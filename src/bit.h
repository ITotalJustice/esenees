#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


//
bool is_bit_set(const uint8_t bit, const uint32_t value);

//
uint32_t get_bit_range(const uint8_t start, const uint8_t end, const uint32_t value);


#ifdef __cplusplus
}
#endif
