

#ifndef CROP_IMG
#define CROP_IMG
#include "arm_math.h"


void img_crop(const uint8_t *src_image, uint8_t *dst_img, const uint32_t src_stride,
              const uint16_t dst_width, const uint16_t dst_height,
              const uint16_t bpp);

#endif
