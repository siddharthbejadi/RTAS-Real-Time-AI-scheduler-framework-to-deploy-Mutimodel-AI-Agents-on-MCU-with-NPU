

#ifndef FACE_STORE_H
#define FACE_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


#define FACE_STORE_EMB_DIM          128u
#define FACE_STORE_DEPTH_W          224u
#define FACE_STORE_DEPTH_H          224u
#define FACE_STORE_DEPTH_SIZE       (FACE_STORE_DEPTH_W * FACE_STORE_DEPTH_H)


#define FACE_STORE_MAX_RECORDS      5u
#define FACE_STORE_NAME_LEN         20u
#define FACE_STORE_MODEL_TAG_LEN    64u


#define FACE_STORE_FLASH_BASE       0x71F00000u
#define FACE_STORE_FLASH_SIZE       (512u * 1024u)
#define FACE_STORE_FLASH_OFFSET     (FACE_STORE_FLASH_BASE - 0x70000000u)

#define FACE_STORE_MAGIC            0xF4CEF4CEu
#define FACE_STORE_VERSION          4u

typedef struct {
    char      name[FACE_STORE_NAME_LEN];
    uint8_t   active;
    uint8_t   is_admin;
    uint8_t   depth_valid;
    uint8_t   _pad[1];
    float     embedding[FACE_STORE_EMB_DIM];
    uint8_t   depth_template[FACE_STORE_DEPTH_SIZE];

    uint8_t   thumb[48 * 48 * 2];
} FaceStore_Record_t;

typedef struct {
    uint32_t            magic;
    uint32_t            version;
    uint32_t            count;
    uint32_t            _pad;
    char                embedding_model[FACE_STORE_MODEL_TAG_LEN];
    FaceStore_Record_t  records[FACE_STORE_MAX_RECORDS];
} FaceStore_Blob_t;


bool FaceStore_Init(void);


uint32_t FaceStore_Count(void);


const FaceStore_Record_t* FaceStore_Get(uint32_t index);


bool FaceStore_Add(const char *name,
                   const float *embedding,
                   const uint8_t *thumb_rgb565_48x48,
                   uint32_t *out_index);


bool FaceStore_SetDepthTemplate(uint32_t index,
                                const uint8_t *depth_224x224);


bool FaceStore_DepthScore(uint32_t index,
                          const uint8_t *probe_224x224,
                          float *out_score);


bool FaceStore_Remove(uint32_t index);


bool FaceStore_Rename(uint32_t index, const char *name);


bool FaceStore_SetAdmin(uint32_t index, bool is_admin);


bool FaceStore_IsAdmin(uint32_t index);


bool FaceStore_ClearAll(void);


bool FaceStore_Commit(void);


bool FaceStore_Match(const float *probe,
                     uint32_t *out_index,
                     float *out_score,
                     float threshold);

#ifdef __cplusplus
}
#endif
#endif
