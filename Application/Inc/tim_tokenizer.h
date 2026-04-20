#ifndef TIM_TOKENIZER_H
#define TIM_TOKENIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIM_TOKENIZER_MAX_LEN 32U

void TIM_Tokenizer_Encode(const char *text, int32_t input_ids[TIM_TOKENIZER_MAX_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* TIM_TOKENIZER_H */
