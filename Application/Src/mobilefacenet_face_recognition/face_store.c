

#include "face_store.h"

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "arm_math.h"
#include "stm32n6xx_hal.h"
#include "stm32n6570_discovery_xspi.h"
#include "rtas_generated_config.h"

#ifndef FACE_STORE_USE_HELIUM
#define FACE_STORE_USE_HELIUM 1
#endif

#if (FACE_STORE_USE_HELIUM != 0) && defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
#include "arm_helium_utils.h"
#define FACE_STORE_HELIUM_F32 1
#else
#define FACE_STORE_HELIUM_F32 0
#endif

__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned(32)))
static FaceStore_Blob_t s_blob;

static const char *current_embedding_model(void)
{
#ifdef RTAS_FACE_EMBEDDING_MODEL_ORIGIN_MODEL_NAME
    return RTAS_FACE_EMBEDDING_MODEL_ORIGIN_MODEL_NAME;
#else
    return "unknown";
#endif
}

static void reset_empty_store(void)
{
    memset(&s_blob, 0, sizeof(s_blob));
    s_blob.magic   = FACE_STORE_MAGIC;
    s_blob.version = FACE_STORE_VERSION;
    s_blob.count   = 0;
    strncpy(s_blob.embedding_model,
            current_embedding_model(),
            sizeof(s_blob.embedding_model) - 1U);
}

static bool stored_model_matches_current(const FaceStore_Blob_t *blob)
{
    if (blob == NULL)
    {
        return false;
    }

    return strncmp(blob->embedding_model,
                   current_embedding_model(),
                   FACE_STORE_MODEL_TAG_LEN) == 0;
}

static void cosine_terms_f32(const float *a, const float *b, uint32_t n,
                             float *dot_out, float *na_out, float *nb_out)
{
    float dot = 0.0f;
    float na = 0.0f;
    float nb = 0.0f;

#if FACE_STORE_HELIUM_F32
    f32x4_t vec_dot = vdupq_n_f32(0.0f);
    f32x4_t vec_na = vdupq_n_f32(0.0f);
    f32x4_t vec_nb = vdupq_n_f32(0.0f);
    uint32_t blk_cnt = n >> 2U;

    while (blk_cnt > 0U) {
        f32x4_t vec_a = vld1q(a);
        f32x4_t vec_b = vld1q(b);

        vec_dot = vfmaq(vec_dot, vec_a, vec_b);
        vec_na = vfmaq(vec_na, vec_a, vec_a);
        vec_nb = vfmaq(vec_nb, vec_b, vec_b);
        a += 4U;
        b += 4U;
        blk_cnt--;
    }

    dot = vecAddAcrossF32Mve(vec_dot);
    na = vecAddAcrossF32Mve(vec_na);
    nb = vecAddAcrossF32Mve(vec_nb);
    n &= 3U;
#endif

    while (n > 0U) {
        const float av = *a++;
        const float bv = *b++;
        dot += av * bv;
        na += av * av;
        nb += bv * bv;
        n--;
    }

    *dot_out = dot;
    *na_out = na;
    *nb_out = nb;
}

static float cosine_similarity(const float *a, const float *b, uint32_t n)
{
    float dot;
    float na;
    float nb;
    cosine_terms_f32(a, b, n, &dot, &na, &nb);
    float denom = sqrtf(na) * sqrtf(nb);

    if (denom < 1e-8f) return 0.0f;
    return dot / denom;
}


bool FaceStore_Init(void)
{

    const FaceStore_Blob_t *flash_blob =
        (const FaceStore_Blob_t *)FACE_STORE_FLASH_BASE;

    if (flash_blob->magic   == FACE_STORE_MAGIC &&
        flash_blob->version == FACE_STORE_VERSION &&
        flash_blob->count   <= FACE_STORE_MAX_RECORDS &&
        stored_model_matches_current(flash_blob))
    {
        memcpy(&s_blob, flash_blob, sizeof(s_blob));
        printf("[FaceStore] Loaded %lu enrolled face(s) from flash for %s\r\n",
               (unsigned long)s_blob.count,
               s_blob.embedding_model);
        return true;
    }

    reset_empty_store();
    printf("[FaceStore] No enrollment data in flash — starting empty\r\n");
    return false;
}

uint32_t FaceStore_Count(void)
{
    return s_blob.count;
}

const FaceStore_Record_t* FaceStore_Get(uint32_t index)
{
    if (index >= s_blob.count) return NULL;
    if (!s_blob.records[index].active) return NULL;
    return &s_blob.records[index];
}

bool FaceStore_Add(const char *name,
                   const float *embedding,
                   const uint8_t *thumb_rgb565_48x48,
                   uint32_t *out_index)
{

    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < FACE_STORE_MAX_RECORDS; i++) {
        if (!s_blob.records[i].active) { slot = i; break; }
    }
    if (slot == UINT32_MAX) return false;

    FaceStore_Record_t *rec = &s_blob.records[slot];
    memset(rec, 0, sizeof(*rec));
    strncpy(rec->name, name, FACE_STORE_NAME_LEN - 1);
    rec->active = 1;
    memcpy(rec->embedding, embedding, FACE_STORE_EMB_DIM * sizeof(float));
    rec->is_admin = (slot == 0U) ? 1U : 0U;
    if (thumb_rgb565_48x48) {
        memcpy(rec->thumb, thumb_rgb565_48x48, sizeof(rec->thumb));
    }


    if (slot + 1 > s_blob.count) s_blob.count = slot + 1;
    if (out_index != NULL) *out_index = slot;
    return true;
}

bool FaceStore_SetDepthTemplate(uint32_t index,
                                const uint8_t *depth_224x224)
{
    if ((index >= FACE_STORE_MAX_RECORDS) ||
        !s_blob.records[index].active ||
        (depth_224x224 == NULL))
    {
        return false;
    }

    memcpy(s_blob.records[index].depth_template,
           depth_224x224,
           FACE_STORE_DEPTH_SIZE);
    s_blob.records[index].depth_valid = 1U;
    return true;
}

bool FaceStore_DepthScore(uint32_t index,
                          const uint8_t *probe_224x224,
                          float *out_score)
{
    if ((index >= FACE_STORE_MAX_RECORDS) ||
        !s_blob.records[index].active ||
        (s_blob.records[index].depth_valid == 0U) ||
        (probe_224x224 == NULL))
    {
        if (out_score != NULL) *out_score = 0.0f;
        return false;
    }

    const uint8_t *probe = probe_224x224;
    const uint8_t *probe_end = probe_224x224 + FACE_STORE_DEPTH_SIZE;
    const uint8_t *stored = s_blob.records[index].depth_template;
    uint32_t sad = 0U;

    while (probe < probe_end)
    {
        uint32_t a = *probe;
        uint32_t b = *stored;
        sad += (a > b) ? (a - b) : (b - a);
        probe += 4U;
        stored += 4U;
    }

    float mean_abs_diff = (float)sad / (float)(FACE_STORE_DEPTH_SIZE / 4U);
    float score = 1.0f - (mean_abs_diff / 96.0f);
    if (score < 0.0f) score = 0.0f;
    if (score > 1.0f) score = 1.0f;

    if (out_score != NULL) *out_score = score;
    return true;
}

bool FaceStore_Remove(uint32_t index)
{
    if (index >= FACE_STORE_MAX_RECORDS) return false;
    memset(&s_blob.records[index], 0, sizeof(s_blob.records[index]));

    uint32_t new_count = 0;
    for (uint32_t i = 0; i < FACE_STORE_MAX_RECORDS; i++) {
        if (s_blob.records[i].active) new_count = i + 1;
    }
    s_blob.count = new_count;
    return true;
}

bool FaceStore_Rename(uint32_t index, const char *name)
{
    if ((index >= FACE_STORE_MAX_RECORDS) || !s_blob.records[index].active || (name == NULL)) {
        return false;
    }

    memset(s_blob.records[index].name, 0, sizeof(s_blob.records[index].name));
    strncpy(s_blob.records[index].name, name, FACE_STORE_NAME_LEN - 1);
    return FaceStore_Commit();
}

bool FaceStore_SetAdmin(uint32_t index, bool is_admin)
{
    if ((index >= FACE_STORE_MAX_RECORDS) || !s_blob.records[index].active) {
        return false;
    }

    s_blob.records[index].is_admin = ((index == 0U) || is_admin) ? 1U : 0U;
    return FaceStore_Commit();
}

bool FaceStore_IsAdmin(uint32_t index)
{
    if ((index >= FACE_STORE_MAX_RECORDS) || !s_blob.records[index].active) {
        return false;
    }
    if (index == 0U) {
        return true;
    }
    return s_blob.records[index].is_admin != 0U;
}

bool FaceStore_ClearAll(void)
{
    reset_empty_store();
    return FaceStore_Commit();
}

bool FaceStore_Match(const float *probe,
                     uint32_t *out_index,
                     float *out_score,
                     float threshold)
{
    float best = -2.0f;
    uint32_t best_idx = UINT32_MAX;
    uint32_t active_count = 0U;
    bool found = false;
    for (uint32_t i = 0; i < s_blob.count; i++) {
        if (!s_blob.records[i].active) continue;
        active_count++;
        float s = cosine_similarity(probe,
                                    s_blob.records[i].embedding,
                                    FACE_STORE_EMB_DIM);
        if (s > best) { best = s; best_idx = i; }
    }
    if (active_count == 0U) {
        if (out_index) *out_index = UINT32_MAX;
        if (out_score) *out_score = 0.0f;
        return false;
    }
    if (best >= threshold) found = true;
    if (out_index) *out_index = best_idx;
    if (out_score) *out_score = best;
    return found;
}


bool FaceStore_Commit(void)
{
    s_blob.magic = FACE_STORE_MAGIC;
    s_blob.version = FACE_STORE_VERSION;
    memset(s_blob.embedding_model, 0, sizeof(s_blob.embedding_model));
    strncpy(s_blob.embedding_model,
            current_embedding_model(),
            sizeof(s_blob.embedding_model) - 1U);

    /* NOR must leave memory-mapped mode before erase or program commands are accepted. */
    BSP_XSPI_NOR_DisableMemoryMappedMode(0);


    for (uint32_t off = 0U; off < FACE_STORE_FLASH_SIZE; off += (64U * 1024U))
    {
        if (BSP_XSPI_NOR_Erase_Block(0,
                                     FACE_STORE_FLASH_OFFSET + off,
                                     BSP_XSPI_NOR_ERASE_64K) != BSP_ERROR_NONE)
        {
            printf("[FaceStore] Erase failed @ +0x%08lX\r\n",
                   (unsigned long)off);
            BSP_XSPI_NOR_EnableMemoryMappedMode(0);
            return false;
        }
    }


    if (BSP_XSPI_NOR_Write(0,
                           (uint8_t *)&s_blob,
                           FACE_STORE_FLASH_OFFSET,
                           sizeof(s_blob)) != BSP_ERROR_NONE)
    {
        printf("[FaceStore] Write failed\r\n");
        BSP_XSPI_NOR_EnableMemoryMappedMode(0);
        return false;
    }

    BSP_XSPI_NOR_EnableMemoryMappedMode(0);
    printf("[FaceStore] Committed %lu records to flash @ 0x%08lX\r\n",
           (unsigned long)s_blob.count,
           (unsigned long)FACE_STORE_FLASH_BASE);
    return true;
}
