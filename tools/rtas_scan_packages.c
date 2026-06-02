/*
 * Host-side RTAS package scanner.
 *
 * This tool runs on the development PC before rtas_codegen. It scans
 * rtas_model_packages/<model_id>/generated folders produced by ST Edge AI and
 * writes the RTAS model manifest from the generated artifacts.
 */

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#define RTAS_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#define RTAS_ISREG(mode) (((mode) & _S_IFREG) != 0)
#else
#define RTAS_ISDIR(mode) S_ISDIR(mode)
#define RTAS_ISREG(mode) S_ISREG(mode)
#endif

#define RTAS_MAX_MODELS 16
#define RTAS_MAX_FILES 64
#define RTAS_TEXT_MAX 256
#define RTAS_PATH_MAX 1024
#define RTAS_FLASH_STEP 0x00200000UL

typedef struct
{
  char names[RTAS_MAX_FILES][RTAS_TEXT_MAX];
  size_t count;
} RtasNameList;

typedef struct
{
  unsigned count;
  unsigned width;
  unsigned height;
  unsigned channels;
  unsigned elements;
  unsigned size_bytes;
  char dtype[RTAS_TEXT_MAX];
  char layout[RTAS_TEXT_MAX];
  char color[RTAS_TEXT_MAX];
} RtasTensorInfo;

typedef struct
{
  char id[RTAS_TEXT_MAX];
  char role[RTAS_TEXT_MAX];
  char generated_dir[RTAS_TEXT_MAX];
  char api_prefix[RTAS_TEXT_MAX];
  char stai_header[RTAS_TEXT_MAX];
  char wrapper_header[RTAS_TEXT_MAX];
  char aton_binary[RTAS_TEXT_MAX];
  char origin_model_name[RTAS_TEXT_MAX];
  char c_sources[RTAS_MAX_FILES][RTAS_TEXT_MAX];
  size_t c_source_count;
  unsigned long flash_address;
  RtasTensorInfo input;
  RtasTensorInfo output;
} RtasModel;

typedef struct
{
  RtasModel models[RTAS_MAX_MODELS];
  size_t model_count;
} RtasRegistry;

static void fail(const char *fmt, ...)
{
  va_list ap;

  fprintf(stderr, "rtas_scan_packages: error: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(2);
}

static void copy_text(char *dst, size_t dst_size, const char *src)
{
  size_t len = strlen(src);

  if (len >= dst_size)
  {
    fail("text value too long: %s", src);
  }

  memcpy(dst, src, len + 1U);
}

static char *trim(char *s)
{
  char *end;

  while (isspace((unsigned char)*s))
  {
    s++;
  }

  end = s + strlen(s);
  while ((end > s) && isspace((unsigned char)end[-1]))
  {
    end--;
  }
  *end = '\0';
  return s;
}

static int str_casecmp(const char *a, const char *b)
{
  while ((*a != '\0') && (*b != '\0'))
  {
    int ca = tolower((unsigned char)*a);
    int cb = tolower((unsigned char)*b);
    if (ca != cb)
    {
      return ca - cb;
    }
    a++;
    b++;
  }

  return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static bool starts_with(const char *text, const char *prefix)
{
  return strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool ends_with(const char *text, const char *suffix)
{
  size_t text_len = strlen(text);
  size_t suffix_len = strlen(suffix);

  return (text_len >= suffix_len) &&
         (strcmp(text + text_len - suffix_len, suffix) == 0);
}

static void lower_copy(char *dst, size_t dst_size, const char *src)
{
  size_t i;

  for (i = 0U; (src[i] != '\0') && (i + 1U < dst_size); i++)
  {
    dst[i] = (char)tolower((unsigned char)src[i]);
  }
  dst[i] = '\0';
}

static bool contains_case(const char *haystack, const char *needle)
{
  char haystack_lower[RTAS_TEXT_MAX * 2U];
  char needle_lower[RTAS_TEXT_MAX];

  lower_copy(haystack_lower, sizeof(haystack_lower), haystack);
  lower_copy(needle_lower, sizeof(needle_lower), needle);
  return strstr(haystack_lower, needle_lower) != NULL;
}

static void uppercase_macro(char *dst, size_t dst_size, const char *src)
{
  size_t pos = 0U;

  for (const char *p = src; *p != '\0'; p++)
  {
    if (pos + 1U >= dst_size)
    {
      fail("macro prefix too long: %s", src);
    }
    dst[pos++] = isalnum((unsigned char)*p) ? (char)toupper((unsigned char)*p) : '_';
  }
  dst[pos] = '\0';
}

static bool path_is_dir(const char *path)
{
  struct stat st;
  return (stat(path, &st) == 0) && RTAS_ISDIR(st.st_mode);
}

static bool path_is_file(const char *path)
{
  struct stat st;
  return (stat(path, &st) == 0) && RTAS_ISREG(st.st_mode);
}

static void join_path(char *out, size_t out_size, const char *base, const char *name)
{
  size_t base_len = strlen(base);

  if (base_len == 0U)
  {
    copy_text(out, out_size, name);
    return;
  }

  if ((base[base_len - 1U] == '/') || (base[base_len - 1U] == '\\'))
  {
    if (snprintf(out, out_size, "%s%s", base, name) >= (int)out_size)
    {
      fail("path too long");
    }
  }
  else
  {
    if (snprintf(out, out_size, "%s/%s", base, name) >= (int)out_size)
    {
      fail("path too long");
    }
  }
}

static void basename_no_ext(char *out, size_t out_size, const char *name)
{
  const char *slash = strrchr(name, '/');
  const char *backslash = strrchr(name, '\\');
  const char *base = name;
  char tmp[RTAS_TEXT_MAX];
  char *dot;

  if ((slash != NULL) && (slash + 1U > base))
  {
    base = slash + 1U;
  }
  if ((backslash != NULL) && (backslash + 1U > base))
  {
    base = backslash + 1U;
  }

  copy_text(tmp, sizeof(tmp), base);
  dot = strrchr(tmp, '.');
  if (dot != NULL)
  {
    *dot = '\0';
  }
  copy_text(out, out_size, tmp);
}

static void add_name(RtasNameList *list, const char *name)
{
  if (list->count >= RTAS_MAX_FILES)
  {
    fail("too many files in generated folder");
  }
  copy_text(list->names[list->count], sizeof(list->names[list->count]), name);
  list->count++;
}

static int compare_text_items(const void *a, const void *b)
{
  const char *sa = (const char *)a;
  const char *sb = (const char *)b;
  return str_casecmp(sa, sb);
}

static int compare_c_sources(const void *a, const void *b)
{
  const char *sa = (const char *)a;
  const char *sb = (const char *)b;
  bool a_is_stai = starts_with(sa, "stai_");
  bool b_is_stai = starts_with(sb, "stai_");

  if (a_is_stai != b_is_stai)
  {
    return a_is_stai ? 1 : -1;
  }
  return str_casecmp(sa, sb);
}

static void sort_names(RtasNameList *list)
{
  qsort(list->names, list->count, sizeof(list->names[0]), compare_text_items);
}

static void list_directory(const char *path, RtasNameList *dirs, RtasNameList *files)
{
  memset(dirs, 0, sizeof(*dirs));
  memset(files, 0, sizeof(*files));

#if defined(_WIN32)
  {
    char pattern[RTAS_PATH_MAX];
    WIN32_FIND_DATAA data;
    HANDLE handle;

    join_path(pattern, sizeof(pattern), path, "*");
    handle = FindFirstFileA(pattern, &data);
    if (handle == INVALID_HANDLE_VALUE)
    {
      fail("cannot list directory: %s", path);
    }

    do
    {
      const bool is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U;

      if ((strcmp(data.cFileName, ".") == 0) || (strcmp(data.cFileName, "..") == 0))
      {
        continue;
      }

      if (is_dir)
      {
        add_name(dirs, data.cFileName);
      }
      else
      {
        add_name(files, data.cFileName);
      }
    } while (FindNextFileA(handle, &data) != 0);

    FindClose(handle);
  }
#else
  {
    DIR *dir = opendir(path);
    struct dirent *entry;

    if (dir == NULL)
    {
      fail("cannot list directory: %s", path);
    }

    while ((entry = readdir(dir)) != NULL)
    {
      char child[RTAS_PATH_MAX];

      if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
      {
        continue;
      }

      join_path(child, sizeof(child), path, entry->d_name);
      if (path_is_dir(child))
      {
        add_name(dirs, entry->d_name);
      }
      else
      {
        add_name(files, entry->d_name);
      }
    }

    closedir(dir);
  }
#endif

  sort_names(dirs);
  sort_names(files);
}

static void strip_cpp_comment(char *line)
{
  bool in_string = false;

  for (char *p = line; *p != '\0'; p++)
  {
    if (*p == '"')
    {
      in_string = !in_string;
    }
    else if ((p[0] == '/') && (p[1] == '/') && !in_string)
    {
      *p = '\0';
      return;
    }
  }
}

static bool parse_define_line(char *line, char *macro, size_t macro_size, char *value, size_t value_size)
{
  char *p;
  size_t len = 0U;

  strip_cpp_comment(line);
  p = trim(line);

  if (!starts_with(p, "#define"))
  {
    return false;
  }

  p += strlen("#define");
  while (isspace((unsigned char)*p))
  {
    p++;
  }

  while ((p[len] != '\0') && !isspace((unsigned char)p[len]))
  {
    len++;
  }

  if ((len == 0U) || (len >= macro_size))
  {
    return false;
  }

  memcpy(macro, p, len);
  macro[len] = '\0';
  p += len;

  while (isspace((unsigned char)*p))
  {
    p++;
  }
  copy_text(value, value_size, trim(p));
  return true;
}

static bool macro_equals(const char *macro, const char *prefix, const char *suffix)
{
  char expected[RTAS_TEXT_MAX];

  if (snprintf(expected, sizeof(expected), "%s_%s", prefix, suffix) >= (int)sizeof(expected))
  {
    fail("macro name too long");
  }
  return strcmp(macro, expected) == 0;
}

static unsigned parse_first_uint(const char *text)
{
  const char *p = text;
  char *end = NULL;
  unsigned long value;

  while ((*p != '\0') && !isdigit((unsigned char)*p))
  {
    p++;
  }

  if (*p == '\0')
  {
    return 0U;
  }

  errno = 0;
  value = strtoul(p, &end, 10);
  if ((errno != 0) || (end == p) || (value > 0xFFFFFFFFUL))
  {
    return 0U;
  }

  return (unsigned)value;
}

static void parse_quoted_string(char *out, size_t out_size, const char *text)
{
  const char *start = strchr(text, '"');
  const char *end;

  if (start == NULL)
  {
    return;
  }

  start++;
  end = strchr(start, '"');
  if (end == NULL)
  {
    return;
  }

  if ((size_t)(end - start) >= out_size)
  {
    fail("quoted string too long");
  }
  memcpy(out, start, (size_t)(end - start));
  out[end - start] = '\0';
}

static void dtype_from_format(char *out, size_t out_size, const char *format)
{
  if (contains_case(format, "STAI_FORMAT_FLOAT32"))
  {
    copy_text(out, out_size, "float32");
  }
  else if (contains_case(format, "STAI_FORMAT_U8"))
  {
    copy_text(out, out_size, "uint8");
  }
  else if (contains_case(format, "STAI_FORMAT_S8"))
  {
    copy_text(out, out_size, "int8");
  }
  else if (contains_case(format, "STAI_FORMAT_U16"))
  {
    copy_text(out, out_size, "uint16");
  }
  else if (contains_case(format, "STAI_FORMAT_S16"))
  {
    copy_text(out, out_size, "int16");
  }
  else
  {
    copy_text(out, out_size, "unknown");
  }
}

static void layout_from_flags(char *out, size_t out_size, const char *flags)
{
  if (contains_case(flags, "CHANNEL_FIRST"))
  {
    copy_text(out, out_size, "CHW");
  }
  else if (contains_case(flags, "CHANNEL_LAST"))
  {
    copy_text(out, out_size, "NHWC");
  }
  else
  {
    copy_text(out, out_size, "unknown");
  }
}

static void parse_stai_header(const char *header_path, const char *api_prefix, RtasModel *model)
{
  FILE *f = fopen(header_path, "r");
  char line[4096];
  char macro[RTAS_TEXT_MAX];
  char value[RTAS_TEXT_MAX * 2U];
  char prefix_macro[RTAS_TEXT_MAX];

  if (f == NULL)
  {
    fail("%s: cannot open STAI header", header_path);
  }

  uppercase_macro(prefix_macro, sizeof(prefix_macro), api_prefix);

  while (fgets(line, sizeof(line), f) != NULL)
  {
    if (!parse_define_line(line, macro, sizeof(macro), value, sizeof(value)))
    {
      continue;
    }

    if (macro_equals(macro, prefix_macro, "ORIGIN_MODEL_NAME"))
    {
      parse_quoted_string(model->origin_model_name, sizeof(model->origin_model_name), value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_NUM"))
    {
      model->input.count = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_WIDTH"))
    {
      model->input.width = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_HEIGHT"))
    {
      model->input.height = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_CHANNEL"))
    {
      model->input.channels = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_SIZE"))
    {
      model->input.elements = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_SIZE_BYTES"))
    {
      model->input.size_bytes = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_FORMAT"))
    {
      dtype_from_format(model->input.dtype, sizeof(model->input.dtype), value);
    }
    else if (macro_equals(macro, prefix_macro, "IN_1_FLAGS"))
    {
      layout_from_flags(model->input.layout, sizeof(model->input.layout), value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_NUM"))
    {
      model->output.count = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_1_WIDTH"))
    {
      model->output.width = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_1_HEIGHT"))
    {
      model->output.height = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_1_CHANNEL"))
    {
      model->output.channels = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_1_SIZE"))
    {
      model->output.elements = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_1_SIZE_BYTES"))
    {
      model->output.size_bytes = parse_first_uint(value);
    }
    else if (macro_equals(macro, prefix_macro, "OUT_1_FORMAT"))
    {
      dtype_from_format(model->output.dtype, sizeof(model->output.dtype), value);
    }
  }

  fclose(f);

  if (model->input.count == 0U)
  {
    model->input.count = 1U;
  }
  if (model->output.count == 0U)
  {
    model->output.count = 1U;
  }
  if (model->input.dtype[0] == '\0')
  {
    copy_text(model->input.dtype, sizeof(model->input.dtype), "unknown");
  }
  if (model->output.dtype[0] == '\0')
  {
    copy_text(model->output.dtype, sizeof(model->output.dtype), "unknown");
  }
  if (model->input.layout[0] == '\0')
  {
    copy_text(model->input.layout, sizeof(model->input.layout), "unknown");
  }
  if (model->input.channels == 3U)
  {
    copy_text(model->input.color, sizeof(model->input.color), "RGB");
  }
  else
  {
    copy_text(model->input.color, sizeof(model->input.color), "unknown");
  }
}

static void parse_origin_from_ll_header(const char *header_path, RtasModel *model)
{
  FILE *f;
  char line[1024];
  char macro[RTAS_TEXT_MAX];
  char value[RTAS_TEXT_MAX * 2U];

  if (model->origin_model_name[0] != '\0')
  {
    return;
  }

  f = fopen(header_path, "r");
  if (f == NULL)
  {
    return;
  }

  while (fgets(line, sizeof(line), f) != NULL)
  {
    if (!parse_define_line(line, macro, sizeof(macro), value, sizeof(value)))
    {
      continue;
    }
    if (strstr(macro, "_ORIGIN_MODEL_NAME") != NULL)
    {
      parse_quoted_string(model->origin_model_name, sizeof(model->origin_model_name), value);
      break;
    }
  }

  fclose(f);
}

static void infer_role(RtasModel *model)
{
  char combined[RTAS_TEXT_MAX * 3U];

  if (snprintf(combined, sizeof(combined), "%s %s %s",
               model->id,
               model->api_prefix,
               model->origin_model_name) >= (int)sizeof(combined))
  {
    fail("%s: model identity text too long", model->id);
  }

  if (contains_case(combined, "embed") || contains_case(combined, "facenet"))
  {
    copy_text(model->role, sizeof(model->role), "face_embedding");
  }
  else if (contains_case(combined, "depth") || contains_case(combined, "liveness"))
  {
    copy_text(model->role, sizeof(model->role), "depth_liveness");
  }
  else if (contains_case(combined, "pose") || contains_case(combined, "keypoint"))
  {
    copy_text(model->role, sizeof(model->role), "pose_estimation");
  }
  else if (contains_case(combined, "blazeface") ||
           (contains_case(combined, "face") && (model->output.count > 1U)))
  {
    copy_text(model->role, sizeof(model->role), "face_detection");
  }
  else if (contains_case(combined, "detect") || contains_case(combined, "yolo"))
  {
    copy_text(model->role, sizeof(model->role), "object_detection");
  }
  else
  {
    copy_text(model->role, sizeof(model->role), "generic_tensor");
  }
}

static int role_priority(const char *role)
{
  if (strcmp(role, "face_detection") == 0)
  {
    return 10;
  }
  if (strcmp(role, "face_embedding") == 0)
  {
    return 20;
  }
  if (strcmp(role, "depth_liveness") == 0)
  {
    return 30;
  }
  if (strcmp(role, "pose_estimation") == 0)
  {
    return 40;
  }
  if (strcmp(role, "object_detection") == 0)
  {
    return 50;
  }
  return 90;
}

static int compare_models(const void *a, const void *b)
{
  const RtasModel *ma = (const RtasModel *)a;
  const RtasModel *mb = (const RtasModel *)b;
  int pa = role_priority(ma->role);
  int pb = role_priority(mb->role);

  if (pa != pb)
  {
    return pa - pb;
  }
  return str_casecmp(ma->id, mb->id);
}

static unsigned long default_flash_address(const char *role)
{
  if (strcmp(role, "face_detection") == 0)
  {
    return 0x71000000UL;
  }
  if (strcmp(role, "face_embedding") == 0)
  {
    return 0x71200000UL;
  }
  if (strcmp(role, "depth_liveness") == 0)
  {
    return 0x71400000UL;
  }
  if (strcmp(role, "pose_estimation") == 0)
  {
    return 0x71600000UL;
  }
  if (strcmp(role, "object_detection") == 0)
  {
    return 0x71800000UL;
  }
  return 0x71A00000UL;
}

static bool flash_address_used(const RtasRegistry *registry, size_t limit, unsigned long address)
{
  for (size_t i = 0U; i < limit; i++)
  {
    if (registry->models[i].flash_address == address)
    {
      return true;
    }
  }
  return false;
}

static void assign_flash_addresses(RtasRegistry *registry)
{
  for (size_t i = 0U; i < registry->model_count; i++)
  {
    unsigned long address = default_flash_address(registry->models[i].role);

    while (flash_address_used(registry, i, address))
    {
      address += RTAS_FLASH_STEP;
    }
    registry->models[i].flash_address = address;
  }
}

static void scan_model_folder(const char *package_root, const char *model_id, RtasModel *model)
{
  char model_root[RTAS_PATH_MAX];
  char generated_path[RTAS_PATH_MAX];
  char header_path[RTAS_PATH_MAX];
  RtasNameList dirs;
  RtasNameList files;
  char ll_header[RTAS_TEXT_MAX] = "";

  memset(model, 0, sizeof(*model));
  copy_text(model->id, sizeof(model->id), model_id);
  if (snprintf(model->generated_dir, sizeof(model->generated_dir), "%s/generated", model_id) >=
      (int)sizeof(model->generated_dir))
  {
    fail("%s: generated_dir too long", model_id);
  }

  join_path(model_root, sizeof(model_root), package_root, model_id);
  join_path(generated_path, sizeof(generated_path), model_root, "generated");
  if (!path_is_dir(generated_path))
  {
    fail("%s: generated folder not found: %s", model_id, generated_path);
  }

  list_directory(generated_path, &dirs, &files);

  for (size_t i = 0U; i < files.count; i++)
  {
    const char *name = files.names[i];

    if (ends_with(name, ".c"))
    {
      if (model->c_source_count >= RTAS_MAX_FILES)
      {
        fail("%s: too many generated C sources", model_id);
      }
      copy_text(model->c_sources[model->c_source_count],
                sizeof(model->c_sources[model->c_source_count]),
                name);
      model->c_source_count++;
    }
    else if (starts_with(name, "stai_") && ends_with(name, ".h"))
    {
      if ((strcmp(name, "stai_network.h") != 0) || (model->stai_header[0] == '\0'))
      {
        copy_text(model->stai_header, sizeof(model->stai_header), name);
      }
      if (strcmp(name, "stai_network.h") == 0)
      {
        copy_text(model->wrapper_header, sizeof(model->wrapper_header), name);
      }
    }
    else if (ends_with(name, ".h") && !contains_case(name, "ecblobs"))
    {
      if (ll_header[0] == '\0')
      {
        copy_text(ll_header, sizeof(ll_header), name);
      }
    }
    else if (ends_with(name, ".bin") && contains_case(name, "atonbuf"))
    {
      copy_text(model->aton_binary, sizeof(model->aton_binary), name);
    }
  }

  if (model->c_source_count == 0U)
  {
    fail("%s: no generated C sources found", model_id);
  }
  qsort(model->c_sources, model->c_source_count, sizeof(model->c_sources[0]), compare_c_sources);

  if (model->stai_header[0] == '\0')
  {
    fail("%s: no stai_*.h header found in %s", model_id, generated_path);
  }

  basename_no_ext(model->api_prefix, sizeof(model->api_prefix), model->stai_header);

  join_path(header_path, sizeof(header_path), generated_path, model->stai_header);
  parse_stai_header(header_path, model->api_prefix, model);

  if (ll_header[0] != '\0')
  {
    join_path(header_path, sizeof(header_path), generated_path, ll_header);
    parse_origin_from_ll_header(header_path, model);
  }

  if (model->aton_binary[0] == '\0')
  {
    fail("%s: no *_atonbuf*.bin file found", model_id);
  }
  if (model->origin_model_name[0] == '\0')
  {
    copy_text(model->origin_model_name, sizeof(model->origin_model_name), "unknown");
  }

  infer_role(model);
}

static void scan_packages(const char *package_root, RtasRegistry *registry)
{
  RtasNameList dirs;
  RtasNameList files;

  memset(registry, 0, sizeof(*registry));

  if (!path_is_dir(package_root))
  {
    fail("package root not found: %s", package_root);
  }

  list_directory(package_root, &dirs, &files);

  for (size_t i = 0U; i < dirs.count; i++)
  {
    char generated_path[RTAS_PATH_MAX];
    char model_root[RTAS_PATH_MAX];

    if (dirs.names[i][0] == '.')
    {
      continue;
    }

    join_path(model_root, sizeof(model_root), package_root, dirs.names[i]);
    join_path(generated_path, sizeof(generated_path), model_root, "generated");
    if (!path_is_dir(generated_path))
    {
      continue;
    }

    if (registry->model_count >= RTAS_MAX_MODELS)
    {
      fail("too many RTAS model packages");
    }

    scan_model_folder(package_root, dirs.names[i], &registry->models[registry->model_count]);
    registry->model_count++;
  }

  if (registry->model_count == 0U)
  {
    fail("no model packages found under %s", package_root);
  }

  qsort(registry->models, registry->model_count, sizeof(registry->models[0]), compare_models);
  assign_flash_addresses(registry);
}

static void write_cache_policy(FILE *out, const RtasModel *model)
{
  fprintf(out, "  cache_policy {\n");
  if (strcmp(model->role, "face_detection") == 0)
  {
    fprintf(out, "    before_sensor_dma_write: \"invalidate_input_buffer\"\n");
    fprintf(out, "    after_sensor_dma_write: \"invalidate_input_buffer\"\n");
  }
  fprintf(out, "    before_npu_read: \"clean_invalidate_input\"\n");
  fprintf(out, "    after_npu_write: \"invalidate_outputs\"\n");
  fprintf(out, "  }\n\n");
}

static void write_scheduler(FILE *out, const RtasModel *model)
{
  fprintf(out, "  scheduler {\n");

  if (strcmp(model->role, "face_detection") == 0)
  {
    fprintf(out, "    policy: \"run_every_frame\"\n");
    fprintf(out, "    priority: 1\n");
  }
  else if (strcmp(model->role, "face_embedding") == 0)
  {
    fprintf(out, "    policy: \"stable_single_face\"\n");
    fprintf(out, "    priority: 2\n");
    fprintf(out, "    stable_face_frames: 2\n");
    fprintf(out, "    auth_recheck_ms: 250\n");
    fprintf(out, "    main_recheck_ms: 750\n");
  }
  else if (strcmp(model->role, "depth_liveness") == 0)
  {
    fprintf(out, "    policy: \"known_or_enrolling_stable_face\"\n");
    fprintf(out, "    priority: 3\n");
    fprintf(out, "    stable_face_frames: 2\n");
    fprintf(out, "    depth_recheck_ms: 1000\n");
  }
  else if (strcmp(model->role, "pose_estimation") == 0)
  {
    fprintf(out, "    policy: \"tracked_subject\"\n");
    fprintf(out, "    priority: 3\n");
    fprintf(out, "    stable_face_frames: 1\n");
  }
  else if (strcmp(model->role, "object_detection") == 0)
  {
    fprintf(out, "    policy: \"run_every_frame\"\n");
    fprintf(out, "    priority: 2\n");
  }
  else
  {
    fprintf(out, "    policy: \"adapter_required\"\n");
    fprintf(out, "    priority: 9\n");
  }

  fprintf(out, "  }\n");
}

static void write_manifest_file(const char *path, const RtasRegistry *registry)
{
  FILE *out = fopen(path, "w");

  if (out == NULL)
  {
    fail("cannot write manifest: %s", path);
  }

  fprintf(out, "# Auto-generated by tools/rtas_scan_packages.c. Do not edit by hand.\n");
  fprintf(out, "# Drop ST Edge AI output into rtas_model_packages/<model_id>/generated and rebuild.\n");
  fprintf(out, "# RTAS infers artifact paths and tensor metadata; semantic roles are controlled by naming rules.\n\n");
  fprintf(out, "schema_version: \"0.2\"\n");
  fprintf(out, "target: \"STM32N6570-DK\"\n");
  fprintf(out, "description: \"Auto-scanned RTAS package registry for generated STM32N6 AI model artifacts.\"\n\n");
  fprintf(out, "runtime {\n");
  fprintf(out, "  control_mode: \"auto_scan_codegen\"\n");
  fprintf(out, "  zero_copy_camera_to_npu: true\n");
  fprintf(out, "  nn_input_buffer_count: 2\n");
  fprintf(out, "  telemetry_print_period_frames: 100\n");
  fprintf(out, "}\n\n");

  for (size_t i = 0U; i < registry->model_count; i++)
  {
    const RtasModel *model = &registry->models[i];

    fprintf(out, "models {\n");
    fprintf(out, "  id: \"%s\"\n", model->id);
    fprintf(out, "  role: \"%s\"\n", model->role);
    fprintf(out, "  generated_dir: \"%s\"\n", model->generated_dir);
    fprintf(out, "  api_prefix: \"%s\"\n", model->api_prefix);
    if (model->wrapper_header[0] != '\0')
    {
      fprintf(out, "  wrapper_header: \"%s\"\n", model->wrapper_header);
    }
    for (size_t src = 0U; src < model->c_source_count; src++)
    {
      fprintf(out, "  c_sources: \"%s\"\n", model->c_sources[src]);
    }
    fprintf(out, "  aton_binary: \"%s\"\n", model->aton_binary);
    fprintf(out, "  flash_address: \"0x%08lX\"\n", model->flash_address);
    fprintf(out, "  origin_model_name: \"%s\"\n\n", model->origin_model_name);

    fprintf(out, "  input {\n");
    fprintf(out, "    width: %u\n", model->input.width);
    fprintf(out, "    height: %u\n", model->input.height);
    fprintf(out, "    channels: %u\n", model->input.channels);
    fprintf(out, "    dtype: \"%s\"\n", model->input.dtype);
    fprintf(out, "    layout: \"%s\"\n", model->input.layout);
    fprintf(out, "    color: \"%s\"\n", model->input.color);
    fprintf(out, "    elements: %u\n", model->input.elements);
    fprintf(out, "    size_bytes: %u\n", model->input.size_bytes);
    fprintf(out, "  }\n\n");

    fprintf(out, "  outputs {\n");
    fprintf(out, "    count: %u\n", model->output.count);
    fprintf(out, "    dtype: \"%s\"\n", model->output.dtype);
    fprintf(out, "    elements: %u\n", model->output.elements);
    fprintf(out, "    size_bytes: %u\n", model->output.size_bytes);
    if (model->output.width != 0U)
    {
      fprintf(out, "    width: %u\n", model->output.width);
    }
    if (model->output.height != 0U)
    {
      fprintf(out, "    height: %u\n", model->output.height);
    }
    if (model->output.channels != 0U)
    {
      fprintf(out, "    channels: %u\n", model->output.channels);
    }
    fprintf(out, "  }\n\n");

    write_cache_policy(out, model);
    write_scheduler(out, model);
    fprintf(out, "}\n");
    if (i + 1U < registry->model_count)
    {
      fprintf(out, "\n");
    }
  }

  if (fclose(out) != 0)
  {
    fail("failed to close manifest: %s", path);
  }
}

static bool files_equal(const char *a_path, const char *b_path)
{
  FILE *a = fopen(a_path, "rb");
  FILE *b = fopen(b_path, "rb");
  bool equal = true;

  if ((a == NULL) || (b == NULL))
  {
    if (a != NULL)
    {
      fclose(a);
    }
    if (b != NULL)
    {
      fclose(b);
    }
    return false;
  }

  for (;;)
  {
    unsigned char a_buf[4096];
    unsigned char b_buf[4096];
    size_t a_count = fread(a_buf, 1U, sizeof(a_buf), a);
    size_t b_count = fread(b_buf, 1U, sizeof(b_buf), b);

    if ((a_count != b_count) || (memcmp(a_buf, b_buf, a_count) != 0))
    {
      equal = false;
      break;
    }
    if (a_count < sizeof(a_buf))
    {
      break;
    }
  }

  fclose(a);
  fclose(b);
  return equal;
}

static void write_manifest_if_changed(const char *out_path, const RtasRegistry *registry)
{
  char tmp_path[RTAS_PATH_MAX];

  if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", out_path) >= (int)sizeof(tmp_path))
  {
    fail("temporary manifest path too long");
  }

  write_manifest_file(tmp_path, registry);

  if (path_is_file(out_path) && files_equal(out_path, tmp_path))
  {
    remove(tmp_path);
    printf("RTAS manifest unchanged %s\n", out_path);
    return;
  }

  remove(out_path);
  if (rename(tmp_path, out_path) != 0)
  {
    fail("cannot replace manifest: %s", out_path);
  }
  printf("RTAS manifest updated %s\n", out_path);
}

static const char *arg_value(int argc, char **argv, const char *name)
{
  for (int i = 1; i < argc - 1; i++)
  {
    if (strcmp(argv[i], name) == 0)
    {
      return argv[i + 1];
    }
  }
  return NULL;
}

int main(int argc, char **argv)
{
  const char *package_root = arg_value(argc, argv, "--packages");
  const char *out_path = arg_value(argc, argv, "--out");
  RtasRegistry registry;

  if ((package_root == NULL) || (out_path == NULL))
  {
    fprintf(stderr, "usage: rtas_scan_packages --packages <rtas_model_packages> --out <rtas_models.pbtxt>\n");
    return 2;
  }

  scan_packages(package_root, &registry);
  write_manifest_if_changed(out_path, &registry);
  return 0;
}
