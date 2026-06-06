/*
 * Host-side RTAS generator.
 *
 * This tool runs on the development PC during the build. It reads the
 * RTAS manifest produced by rtas_scan_packages and emits MCU-safe C macros.
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
#include <direct.h>
#ifndef _S_IFDIR
#define _S_IFDIR S_IFDIR
#endif
#ifndef _S_IFREG
#define _S_IFREG S_IFREG
#endif
#define RTAS_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#define RTAS_ISREG(mode) (((mode) & _S_IFREG) != 0)
#else
#include <unistd.h>
#define RTAS_ISDIR(mode) S_ISDIR(mode)
#define RTAS_ISREG(mode) S_ISREG(mode)
#endif

#define RTAS_MAX_MODELS 8
#define RTAS_MAX_SOURCES 16
#define RTAS_TEXT_MAX 192
#define RTAS_PATH_MAX 1024
#define RTAS_STACK_MAX 16

typedef struct
{
  unsigned stable_face_frames;
  unsigned auth_recheck_ms;
  unsigned main_recheck_ms;
  unsigned depth_recheck_ms;
} RtasScheduler;

typedef struct
{
  char id[RTAS_TEXT_MAX];
  char role[RTAS_TEXT_MAX];
  char generated_dir[RTAS_TEXT_MAX];
  char aton_binary[RTAS_TEXT_MAX];
  char flash_address[RTAS_TEXT_MAX];
  char origin_model_name[RTAS_TEXT_MAX];
  char c_sources[RTAS_MAX_SOURCES][RTAS_TEXT_MAX];
  size_t c_source_count;
  RtasScheduler scheduler;
} RtasModel;

typedef struct
{
  bool zero_copy_camera_to_npu;
  unsigned nn_input_buffer_count;
  unsigned telemetry_print_period_frames;
} RtasRuntime;

typedef struct
{
  char schema_version[RTAS_TEXT_MAX];
  char target[RTAS_TEXT_MAX];
  RtasRuntime runtime;
  RtasModel models[RTAS_MAX_MODELS];
  size_t model_count;
} RtasManifest;

static void fail(const char *fmt, ...)
{
  va_list ap;

  fprintf(stderr, "rtas_codegen: error: ");
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

static void strip_comment(char *line)
{
  bool in_string = false;

  for (char *p = line; *p != '\0'; p++)
  {
    if (*p == '"')
    {
      in_string = !in_string;
    }
    else if ((*p == '#') && !in_string)
    {
      *p = '\0';
      return;
    }
  }
}

static void parse_string_value(const char *raw, char *dst, size_t dst_size)
{
  char tmp[RTAS_TEXT_MAX];
  size_t len;

  copy_text(tmp, sizeof(tmp), raw);
  raw = trim(tmp);
  len = strlen(raw);

  if ((len >= 2U) && (raw[0] == '"') && (raw[len - 1U] == '"'))
  {
    if ((len - 2U) >= dst_size)
    {
      fail("quoted text value too long");
    }
    memcpy(dst, raw + 1U, len - 2U);
    dst[len - 2U] = '\0';
    return;
  }

  copy_text(dst, dst_size, raw);
}

static bool parse_bool_value(const char *raw, const char *name)
{
  char tmp[RTAS_TEXT_MAX];

  parse_string_value(raw, tmp, sizeof(tmp));
  if (strcmp(tmp, "true") == 0)
  {
    return true;
  }
  if (strcmp(tmp, "false") == 0)
  {
    return false;
  }

  fail("%s must be true or false", name);
  return false;
}

static unsigned parse_uint_value(const char *raw, const char *name)
{
  char tmp[RTAS_TEXT_MAX];
  char *end = NULL;
  unsigned long value;

  parse_string_value(raw, tmp, sizeof(tmp));
  errno = 0;
  value = strtoul(tmp, &end, 0);
  if ((errno != 0) || (end == tmp) || (*trim(end) != '\0') || (value > 0xFFFFFFFFUL))
  {
    fail("%s must be an unsigned integer", name);
  }

  return (unsigned)value;
}

static unsigned require_range(const char *name, unsigned value, unsigned min_value, unsigned max_value)
{
  if ((value < min_value) || (value > max_value))
  {
    fail("%s=%u outside controlled range %u..%u", name, value, min_value, max_value);
  }
  return value;
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

static void dirname_of(const char *path, char *out, size_t out_size)
{
  const char *slash = strrchr(path, '/');
  const char *backslash = strrchr(path, '\\');
  const char *last = slash;

  if ((backslash != NULL) && ((last == NULL) || (backslash > last)))
  {
    last = backslash;
  }

  if (last == NULL)
  {
    copy_text(out, out_size, ".");
    return;
  }

  if ((size_t)(last - path) >= out_size)
  {
    fail("directory path too long: %s", path);
  }
  memcpy(out, path, (size_t)(last - path));
  out[last - path] = '\0';
}

static void resolve_path(const char *path, char *out, size_t out_size)
{
#if defined(_WIN32)
  if (_fullpath(out, path, out_size) == NULL)
  {
    fail("cannot resolve path: %s", path);
  }
#else
  copy_text(out, out_size, path);
#endif
}

static void join_path(char *out, size_t out_size, const char *base, const char *name)
{
  size_t base_len = strlen(base);
  const char sep = '/';

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
    if (snprintf(out, out_size, "%s%c%s", base, sep, name) >= (int)out_size)
    {
      fail("path too long");
    }
  }
}

static const char *stack_top(char stack[RTAS_STACK_MAX][RTAS_TEXT_MAX], size_t depth)
{
  return (depth > 0U) ? stack[depth - 1U] : "";
}

static RtasModel *find_model_by_role(RtasManifest *manifest, const char *role)
{
  for (size_t i = 0U; i < manifest->model_count; i++)
  {
    if (strcmp(manifest->models[i].role, role) == 0)
    {
      return &manifest->models[i];
    }
  }

  return NULL;
}

static void parse_manifest(const char *path, RtasManifest *manifest)
{
  FILE *f = fopen(path, "r");
  char line_buf[1024];
  char stack[RTAS_STACK_MAX][RTAS_TEXT_MAX];
  size_t depth = 0U;
  int current_model = -1;
  unsigned line_no = 0U;

  if (f == NULL)
  {
    fail("cannot open manifest: %s", path);
  }

  memset(manifest, 0, sizeof(*manifest));
  manifest->runtime.zero_copy_camera_to_npu = true;
  manifest->runtime.nn_input_buffer_count = 2U;
  manifest->runtime.telemetry_print_period_frames = 100U;

  while (fgets(line_buf, sizeof(line_buf), f) != NULL)
  {
    char *line;
    size_t line_len;

    line_no++;
    strip_comment(line_buf);
    line = trim(line_buf);
    if (*line == '\0')
    {
      continue;
    }

    line_len = strlen(line);
    if ((line_len > 0U) && (line[line_len - 1U] == '{'))
    {
      line[line_len - 1U] = '\0';
      line = trim(line);
      if (depth >= RTAS_STACK_MAX)
      {
        fail("%s:%u: too many nested sections", path, line_no);
      }
      copy_text(stack[depth], sizeof(stack[depth]), line);
      depth++;

      if (strcmp(line, "models") == 0)
      {
        RtasModel *model;
        if (manifest->model_count >= RTAS_MAX_MODELS)
        {
          fail("%s:%u: too many models", path, line_no);
        }
        model = &manifest->models[manifest->model_count];
        memset(model, 0, sizeof(*model));
        current_model = (int)manifest->model_count;
        manifest->model_count++;
      }
      continue;
    }

    if (strcmp(line, "}") == 0)
    {
      const char *closed;
      if (depth == 0U)
      {
        fail("%s:%u: unexpected closing brace", path, line_no);
      }
      closed = stack[depth - 1U];
      if (strcmp(closed, "models") == 0)
      {
        current_model = -1;
      }
      depth--;
      continue;
    }

    {
      char *colon = strchr(line, ':');
      char *key;
      char *value;
      const char *section;

      if (colon == NULL)
      {
        fail("%s:%u: expected key: value", path, line_no);
      }

      *colon = '\0';
      key = trim(line);
      value = trim(colon + 1U);
      section = stack_top(stack, depth);

      if (current_model >= 0)
      {
        RtasModel *model = &manifest->models[current_model];
        if (strcmp(section, "scheduler") == 0)
        {
          if (strcmp(key, "stable_face_frames") == 0)
          {
            model->scheduler.stable_face_frames = parse_uint_value(value, key);
          }
          else if (strcmp(key, "auth_recheck_ms") == 0)
          {
            model->scheduler.auth_recheck_ms = parse_uint_value(value, key);
          }
          else if (strcmp(key, "main_recheck_ms") == 0)
          {
            model->scheduler.main_recheck_ms = parse_uint_value(value, key);
          }
          else if (strcmp(key, "depth_recheck_ms") == 0)
          {
            model->scheduler.depth_recheck_ms = parse_uint_value(value, key);
          }
        }
        else if (strcmp(key, "id") == 0)
        {
          parse_string_value(value, model->id, sizeof(model->id));
        }
        else if (strcmp(key, "role") == 0)
        {
          parse_string_value(value, model->role, sizeof(model->role));
        }
        else if (strcmp(key, "generated_dir") == 0)
        {
          parse_string_value(value, model->generated_dir, sizeof(model->generated_dir));
        }
        else if (strcmp(key, "aton_binary") == 0)
        {
          parse_string_value(value, model->aton_binary, sizeof(model->aton_binary));
        }
        else if (strcmp(key, "flash_address") == 0)
        {
          parse_string_value(value, model->flash_address, sizeof(model->flash_address));
        }
        else if (strcmp(key, "origin_model_name") == 0)
        {
          parse_string_value(value, model->origin_model_name, sizeof(model->origin_model_name));
        }
        else if (strcmp(key, "c_sources") == 0)
        {
          if (model->c_source_count >= RTAS_MAX_SOURCES)
          {
            fail("%s:%u: too many c_sources for model %s", path, line_no, model->id);
          }
          parse_string_value(value,
                             model->c_sources[model->c_source_count],
                             sizeof(model->c_sources[model->c_source_count]));
          model->c_source_count++;
        }
      }
      else if (strcmp(section, "runtime") == 0)
      {
        if (strcmp(key, "zero_copy_camera_to_npu") == 0)
        {
          manifest->runtime.zero_copy_camera_to_npu = parse_bool_value(value, key);
        }
        else if (strcmp(key, "nn_input_buffer_count") == 0)
        {
          manifest->runtime.nn_input_buffer_count = parse_uint_value(value, key);
        }
        else if (strcmp(key, "telemetry_print_period_frames") == 0)
        {
          manifest->runtime.telemetry_print_period_frames = parse_uint_value(value, key);
        }
      }
      else if (strcmp(key, "schema_version") == 0)
      {
        parse_string_value(value, manifest->schema_version, sizeof(manifest->schema_version));
      }
      else if (strcmp(key, "target") == 0)
      {
        parse_string_value(value, manifest->target, sizeof(manifest->target));
      }
    }
  }

  fclose(f);

  if (depth != 0U)
  {
    fail("%s: unclosed section", path);
  }
}

static void validate_artifacts(const char *manifest_path, const RtasManifest *manifest)
{
  char package_root[RTAS_PATH_MAX];

  dirname_of(manifest_path, package_root, sizeof(package_root));

  for (size_t i = 0U; i < manifest->model_count; i++)
  {
    const RtasModel *model = &manifest->models[i];
    char model_dir[RTAS_PATH_MAX];
    char artifact_path[RTAS_PATH_MAX];

    if (model->id[0] == '\0')
    {
      fail("model %u missing id", (unsigned)i);
    }
    if (model->generated_dir[0] == '\0')
    {
      fail("%s: generated_dir is required", model->id);
    }
    join_path(model_dir, sizeof(model_dir), package_root, model->generated_dir);
    if (!path_is_dir(model_dir))
    {
      fail("%s: generated_dir not found: %s", model->id, model_dir);
    }

    for (size_t src = 0U; src < model->c_source_count; src++)
    {
      join_path(artifact_path, sizeof(artifact_path), model_dir, model->c_sources[src]);
      if (!path_is_file(artifact_path))
      {
        fail("%s: C source not found: %s", model->id, artifact_path);
      }
    }

    if (model->aton_binary[0] != '\0')
    {
      join_path(artifact_path, sizeof(artifact_path), model_dir, model->aton_binary);
      if (!path_is_file(artifact_path))
      {
        fail("%s: ATON binary not found: %s", model->id, artifact_path);
      }
    }
  }
}

static void macro_name(char *out, size_t out_size, const char *model_id, const char *suffix)
{
  size_t pos;

  if (snprintf(out, out_size, "RTAS_MODEL_") >= (int)out_size)
  {
    fail("macro name too long");
  }
  pos = strlen(out);

  for (const char *p = model_id; *p != '\0'; p++)
  {
    if (pos + 1U >= out_size)
    {
      fail("macro name too long for model id: %s", model_id);
    }
    out[pos++] = isalnum((unsigned char)*p) ? (char)toupper((unsigned char)*p) : '_';
  }

  if (snprintf(out + pos, out_size - pos, "_%s", suffix) >= (int)(out_size - pos))
  {
    fail("macro suffix too long");
  }
}

static unsigned parse_flash_address(const char *text)
{
  char *end = NULL;
  unsigned long value;

  errno = 0;
  value = strtoul(text, &end, 0);
  if ((errno != 0) || (end == text) || (*trim(end) != '\0') || (value > 0xFFFFFFFFUL))
  {
    fail("invalid flash address: %s", text);
  }
  return (unsigned)value;
}

static void generate_header(const char *manifest_path, RtasManifest *manifest, const char *out_path)
{
  FILE *out;
  RtasModel *embed = find_model_by_role(manifest, "face_embedding");
  RtasModel *depth = find_model_by_role(manifest, "depth_liveness");
  unsigned nn_input_buffer_count;
  unsigned telemetry_period;
  unsigned stable_face_frames;
  unsigned auth_recheck_ms;
  unsigned main_recheck_ms;
  unsigned depth_recheck_ms;

  nn_input_buffer_count = require_range("runtime.nn_input_buffer_count",
                                        manifest->runtime.nn_input_buffer_count,
                                        2U,
                                        3U);
  telemetry_period = require_range("runtime.telemetry_print_period_frames",
                                   manifest->runtime.telemetry_print_period_frames,
                                   0U,
                                   10000U);
  stable_face_frames = require_range("face_embedding.scheduler.stable_face_frames",
                                     (embed != NULL) && (embed->scheduler.stable_face_frames != 0U) ?
                                       embed->scheduler.stable_face_frames : 2U,
                                     1U,
                                     10U);
  auth_recheck_ms = require_range("face_embedding.scheduler.auth_recheck_ms",
                                  (embed != NULL) && (embed->scheduler.auth_recheck_ms != 0U) ?
                                    embed->scheduler.auth_recheck_ms : 250U,
                                  50U,
                                  5000U);
  main_recheck_ms = require_range("face_embedding.scheduler.main_recheck_ms",
                                  (embed != NULL) && (embed->scheduler.main_recheck_ms != 0U) ?
                                    embed->scheduler.main_recheck_ms : 750U,
                                  50U,
                                  10000U);
  depth_recheck_ms = require_range("depth_liveness.scheduler.depth_recheck_ms",
                                   (depth != NULL) && (depth->scheduler.depth_recheck_ms != 0U) ?
                                     depth->scheduler.depth_recheck_ms : 1000U,
                                   100U,
                                   10000U);

  out = fopen(out_path, "w");
  if (out == NULL)
  {
    fail("cannot write output: %s", out_path);
  }

  fprintf(out, "/* Auto-generated by tools/rtas_codegen.c. Do not edit by hand. */\n");
  fprintf(out, "/* Source: %s */\n", manifest_path);
  fprintf(out, "#ifndef RTAS_GENERATED_CONFIG_H\n");
  fprintf(out, "#define RTAS_GENERATED_CONFIG_H\n\n");
  fprintf(out, "#define RTAS_MANIFEST_SCHEMA_VERSION \"%s\"\n", manifest->schema_version);
  fprintf(out, "#define RTAS_TARGET \"%s\"\n", manifest->target);
  fprintf(out, "#define RTAS_MODEL_COUNT %uU\n\n", (unsigned)manifest->model_count);
  fprintf(out, "#define RTAS_NN_INPUT_ZERO_COPY_ENABLE %u\n", manifest->runtime.zero_copy_camera_to_npu ? 1U : 0U);
  fprintf(out, "#define RTAS_NN_INPUT_BUFFER_COUNT %uU\n", nn_input_buffer_count);
  fprintf(out, "#define RTAS_TELEMETRY_PRINT_PERIOD_FRAMES %uU\n\n", telemetry_period);
  fprintf(out, "#define RTAS_MODEL_SCHED_STABLE_FACE_FRAMES %uU\n", stable_face_frames);
  fprintf(out, "#define RTAS_MODEL_SCHED_AUTH_RECHECK_MS %uU\n", auth_recheck_ms);
  fprintf(out, "#define RTAS_MODEL_SCHED_MAIN_RECHECK_MS %uU\n", main_recheck_ms);
  fprintf(out, "#define RTAS_MODEL_SCHED_DEPTH_RECHECK_MS %uU\n\n", depth_recheck_ms);

  for (size_t i = 0U; i < manifest->model_count; i++)
  {
    const RtasModel *model = &manifest->models[i];
    char macro[RTAS_TEXT_MAX];

    macro_name(macro, sizeof(macro), model->id, "ID");
    fprintf(out, "#define %s \"%s\"\n", macro, model->id);
    macro_name(macro, sizeof(macro), model->id, "ROLE");
    fprintf(out, "#define %s \"%s\"\n", macro, model->role);
    macro_name(macro, sizeof(macro), model->id, "GENERATED_DIR");
    fprintf(out, "#define %s \"%s\"\n", macro, model->generated_dir);
    macro_name(macro, sizeof(macro), model->id, "ORIGIN_MODEL_NAME");
    fprintf(out, "#define %s \"%s\"\n",
            macro,
            model->origin_model_name[0] != '\0' ? model->origin_model_name : "unknown");
    if (model->flash_address[0] != '\0')
    {
      macro_name(macro, sizeof(macro), model->id, "FLASH_ADDRESS");
      fprintf(out, "#define %s 0x%08XUL\n", macro, parse_flash_address(model->flash_address));
    }
    fprintf(out, "\n");

    if (strcmp(model->role, "face_embedding") == 0)
    {
      fprintf(out, "#define RTAS_FACE_EMBEDDING_MODEL_ID \"%s\"\n", model->id);
      fprintf(out, "#define RTAS_FACE_EMBEDDING_MODEL_ORIGIN_MODEL_NAME \"%s\"\n\n",
              model->origin_model_name[0] != '\0' ? model->origin_model_name : "unknown");
    }
  }

  fprintf(out, "#endif /* RTAS_GENERATED_CONFIG_H */\n");

  if (fclose(out) != 0)
  {
    fail("failed to close output: %s", out_path);
  }
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
  const char *manifest_path = arg_value(argc, argv, "--manifest");
  const char *out_path = arg_value(argc, argv, "--out");
  char manifest_resolved[RTAS_PATH_MAX];
  RtasManifest manifest;

  if ((manifest_path == NULL) || (out_path == NULL))
  {
    fprintf(stderr, "usage: rtas_codegen --manifest <rtas_models.pbtxt> --out <rtas_generated_config.h>\n");
    return 2;
  }

  resolve_path(manifest_path, manifest_resolved, sizeof(manifest_resolved));
  parse_manifest(manifest_path, &manifest);
  validate_artifacts(manifest_resolved, &manifest);
  generate_header(manifest_path, &manifest, out_path);
  printf("RTAS generated %s\n", out_path);
  return 0;
}
