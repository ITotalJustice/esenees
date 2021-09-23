#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include <stdint.h>


bool snes_init(struct SNES_Core* snes);
bool snes_loadrom(struct SNES_Core* snes, const uint8_t* rom, size_t rom_size);
bool snes_run(struct SNES_Core* snes);

#ifdef __cplusplus
}
#endif
