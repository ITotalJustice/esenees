// this file is basically a dumping ground of internal functions that
// shouldn't *really* be exposed to the user.
// it would probably be best to split the component specific stuff into
// their own headers, such as cpu.h.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#if SNES_DEBUG
    #include <stdio.h>
    #include <assert.h>
    #define snes_log(...) fprintf(stdout, __VA_ARGS__)
    #define snes_log_err(...) fprintf(stderr, __VA_ARGS__)
    #define snes_log_fatal(...) do { fprintf(stderr, __VA_ARGS__); assert(0); } while(0)
#else
    #define snes_log(...)
    #define snes_log_err(...)
    #define snes_log_fatal(...)
#endif // SNES_DEBUG

uint8_t snes_cpu_read8(struct SNES_Core* snes, uint32_t addr);
uint16_t snes_cpu_read16(struct SNES_Core* snes, uint32_t addr);

void snes_cpu_write8(struct SNES_Core* snes, uint32_t addr, uint8_t value);
void snes_cpu_write16(struct SNES_Core* snes, uint32_t addr, uint16_t value);

bool snes_cpu_init(struct SNES_Core* snes);
bool snes_ppu_init(struct SNES_Core* snes);
bool snes_apu_init(struct SNES_Core* snes);

void snes_cpu_run(struct SNES_Core* snes);
// void snes_ppu_run(struct SNES_Core* snes);
// void snes_apu_run(struct SNES_Core* snes);

#ifdef __cplusplus
}
#endif
