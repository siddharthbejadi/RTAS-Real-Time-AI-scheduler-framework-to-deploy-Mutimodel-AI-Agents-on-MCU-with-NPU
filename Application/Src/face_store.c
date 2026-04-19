/**
 ******************************************************************************
 * @file    face_store.c
 * @brief   Persistent face-embedding store in external NOR flash.
 *
 * The working copy lives in PSRAM; FaceStore_Commit() writes the whole blob
 * to a single 64-KB sector of MX66UW1G45G via XSPI2 indirect mode, then
 * re-enables memory-mapped mode so the bootloader / model loader keep working.
 ******************************************************************************
 */

#include "face_store.h"

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "stm32n6xx_hal.h"
#include "stm32n6570_discovery_xspi.h"

/* ───── Private state ─────────────────────────────────────────────────────── */

/* Working copy kept in PSRAM so it can be modified freely without erase/write
 * cycles until the user commits. */
__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned(32)))
static FaceStore_Blob_t s_blob;

/* ───── Cosine similarity helper (embeddings assumed NOT pre-normalized) ──── */

static float cosine_similarity(const float *a, const float *b, uint32_t n)
{
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = sqrtf(na) * sqrtf(nb);
    if (denom < 1e-8f) return 0.0f;
    return dot / denom;
}

/* ───── Public API ────────────────────────────────────────────────────────── */

bool FaceStore_Init(void)
{
    /* Read from memory-mapped NOR (always enabled at boot). */
    const FaceStore_Blob_t *flash_blob =
        (const FaceStore_Blob_t *)FACE_STORE_FLASH_BASE;

    if (flash_blob->magic   == FACE_STORE_MAGIC &&
        flash_blob->version == FACE_STORE_VERSION &&
        flash_blob->count   <= FACE_STORE_MAX_RECORDS)
    {
        memcpy(&s_blob, flash_blob, sizeof(s_blob));
        printf("[FaceStore] Loaded %lu enrolled face(s) from flash\r\n",
               (unsigned long)s_blob.count);
        return true;
    }

    /* Empty / corrupt — initialise fresh RAM copy. */
    memset(&s_blob, 0, sizeof(s_blob));
    s_blob.magic   = FACE_STORE_MAGIC;
    s_blob.version = FACE_STORE_VERSION;
    s_blob.count   = 0;
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
                   const uint8_t *thumb_rgb565_48x48)
{
    /* Find first free slot. */
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

    /* Keep count = highest active slot + 1 (linear-packed storage). */
    if (slot + 1 > s_blob.count) s_blob.count = slot + 1;
    return true;
}

bool FaceStore_Remove(uint32_t index)
{
    if (index >= FACE_STORE_MAX_RECORDS) return false;
    memset(&s_blob.records[index], 0, sizeof(s_blob.records[index]));
    /* Recompute count. */
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
    if (index == 0U) {
        return true;
    }
    if ((index >= FACE_STORE_MAX_RECORDS) || !s_blob.records[index].active) {
        return false;
    }
    return s_blob.records[index].is_admin != 0U;
}

bool FaceStore_ClearAll(void)
{
    memset(&s_blob, 0, sizeof(s_blob));
    s_blob.magic   = FACE_STORE_MAGIC;
    s_blob.version = FACE_STORE_VERSION;
    s_blob.count   = 0;
    return FaceStore_Commit();
}

bool FaceStore_Match(const float *probe,
                     uint32_t *out_index,
                     float *out_score,
                     float threshold)
{
    float best = -2.0f;
    uint32_t best_idx = 0;
    bool found = false;
    for (uint32_t i = 0; i < s_blob.count; i++) {
        if (!s_blob.records[i].active) continue;
        float s = cosine_similarity(probe,
                                    s_blob.records[i].embedding,
                                    FACE_STORE_EMB_DIM);
        if (s > best) { best = s; best_idx = i; }
    }
    if (best >= threshold) found = true;
    if (out_index) *out_index = best_idx;
    if (out_score) *out_score = best;
    return found;
}

/* ───── Flash commit ─────────────────────────────────────────────────────── */

bool FaceStore_Commit(void)
{
    /* Leave memory-mapped mode so we can erase/program. */
    BSP_XSPI_NOR_DisableMemoryMappedMode(0);

    /* Erase the 64 KB sector. MX66UW1G45G supports 4KB / 64KB sector erase. */
    if (BSP_XSPI_NOR_Erase_Block(0,
                                 FACE_STORE_FLASH_OFFSET,
                                 BSP_XSPI_NOR_ERASE_64K) != BSP_ERROR_NONE)
    {
        printf("[FaceStore] Erase failed\r\n");
        BSP_XSPI_NOR_EnableMemoryMappedMode(0);
        return false;
    }

    /* Program. BSP write handles page programming internally. */
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
