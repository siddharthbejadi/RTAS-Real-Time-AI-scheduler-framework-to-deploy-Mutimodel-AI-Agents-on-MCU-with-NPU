/**
 ******************************************************************************
 * @file    face_store.h
 * @brief   Persistent storage for enrolled face embeddings in external NOR
 *          flash (MX66UW1G45G, XSPI2, memory-mapped base 0x70000000).
 *
 *  Flash layout
 *  ────────────
 *   0x70000000               Start of external NOR (128 MB)
 *   0x70380000               BlazeFace network data (existing)
 *   0x71000000 / 0x71200000  User model .bin blobs (existing)
 *   0x71F00000 ─┐
 *              │  FACE_STORE_BASE — 64 KB reserved for enrollment records
 *   0x71F10000 ─┘
 *
 *  Record format (packed, aligned to 4 KB sector):
 *   ┌───────────────────────────────────────────────────────────┐
 *   │ magic   uint32   0xF4CEF4CE ("face" in hex-speak)         │
 *   │ version uint32   1                                        │
 *   │ count   uint32   number of enrolled persons (<= 8)        │
 *   │ pad     uint32                                            │
 *   │ records FaceStore_Record_t[count]                         │
 *   └───────────────────────────────────────────────────────────┘
 ******************************************************************************
 */
#ifndef FACE_STORE_H
#define FACE_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* The MobileFaceNet standard embedding dimension is 128 floats.
 * If your specific model outputs a different size, adjust here. */
#define FACE_STORE_EMB_DIM          128u

/* Must match UI_MAX_PERSONS in app_ui.h */
#define FACE_STORE_MAX_RECORDS      8u
#define FACE_STORE_NAME_LEN         20u

/* External NOR flash addresses (memory-mapped read) */
#define FACE_STORE_FLASH_BASE       0x71F00000u
#define FACE_STORE_FLASH_SIZE       (64u * 1024u)      /* 64 KB */
#define FACE_STORE_FLASH_OFFSET     (FACE_STORE_FLASH_BASE - 0x70000000u)

#define FACE_STORE_MAGIC            0xF4CEF4CEu
#define FACE_STORE_VERSION          1u

typedef struct {
    char      name[FACE_STORE_NAME_LEN];
    uint8_t   active;                              /* 1 = slot used */
    uint8_t   _pad[3];
    float     embedding[FACE_STORE_EMB_DIM];
    /* optional small thumbnail stored for later visual confirmation */
    uint8_t   thumb[48 * 48 * 2];                  /* 48x48 RGB565 = 4.5 KB */
} FaceStore_Record_t;

typedef struct {
    uint32_t            magic;
    uint32_t            version;
    uint32_t            count;
    uint32_t            _pad;
    FaceStore_Record_t  records[FACE_STORE_MAX_RECORDS];
} FaceStore_Blob_t;

/* ────────────── Public API ───────────────────────────────────────────────── */

/**
 * @brief  Initialise the face store. Reads the blob from NOR flash; if magic
 *         is absent, initialises an empty RAM copy.
 * @return true if flash contained a valid blob; false if RAM is empty.
 */
bool FaceStore_Init(void);

/** @brief  Number of enrolled persons currently in the store. */
uint32_t FaceStore_Count(void);

/** @brief  Pointer to the i-th record, or NULL if out of range / inactive. */
const FaceStore_Record_t* FaceStore_Get(uint32_t index);

/**
 * @brief  Add a new enrollment (embedding + name).
 * @param  name       display name (truncated to FACE_STORE_NAME_LEN-1)
 * @param  embedding  pointer to FACE_STORE_EMB_DIM floats
 * @param  thumb_rgb565_48x48  optional 48x48 RGB565 thumbnail (may be NULL)
 * @return true if added, false if store is full.
 */
bool FaceStore_Add(const char *name,
                   const float *embedding,
                   const uint8_t *thumb_rgb565_48x48);

/** @brief  Remove record at index. Returns true on success. */
bool FaceStore_Remove(uint32_t index);

/** @brief  Clear all enrollments (erases flash sector). */
bool FaceStore_ClearAll(void);

/**
 * @brief  Commit the current RAM state to NOR flash.
 *         Temporarily disables XSPI memory-mapped mode, erases the sector,
 *         writes the blob, then re-enables memory-mapped mode.
 * @return true on success.
 */
bool FaceStore_Commit(void);

/**
 * @brief  Find the best-matching enrolled embedding.
 * @param  probe      pointer to FACE_STORE_EMB_DIM floats (the query)
 * @param  out_index  [out] matched index (undefined if no match)
 * @param  out_score  [out] cosine similarity of the best match
 * @param  threshold  minimum cosine similarity to accept (e.g., 0.6 for MobileFaceNet)
 * @return true if a match above threshold was found.
 */
bool FaceStore_Match(const float *probe,
                     uint32_t *out_index,
                     float *out_score,
                     float threshold);

#ifdef __cplusplus
}
#endif
#endif /* FACE_STORE_H */
