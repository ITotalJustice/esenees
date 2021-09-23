#include "bit.h"
#include <assert.h>

bool is_bit_set(const uint8_t bit, const uint32_t value)
{
    assert(bit < (sizeof(uint32_t) * 8) && "bit value out of bounds!");

    return !!(value & (1U << bit));
}

uint32_t get_bit_range(const uint8_t start, const uint8_t end, const uint32_t value)
{
    assert(end > start && end < 31 && "invalid bit range!");

    return (value & (0xFFFFFFFF >> (31 - end))) >> start;
}
