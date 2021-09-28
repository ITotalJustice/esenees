#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


#define BIT_MACROS 0

#if BIT_MACROS
    #define is_bit_set(bit, value) !!((value) & (1U << (bit)))
    #define get_bit_range(start, end, value) ((value) & (0xFFFFFFFF >> (31 - (end)))) >> (start);
#else
    bool is_bit_set(const uint8_t bit, const uint32_t value);
    uint32_t get_bit_range(const uint8_t start, const uint8_t end, const uint32_t value);
#endif // #if BIT_MACROS

#ifdef __cplusplus
}
#endif
