#include "internal.h"
#include <stdint.h>


// NOTE: the below is for lorom mapping ONLY.
// hirom will be implemented later on, once everything is already working

uint8_t snes_cpu_read8(struct SNES_Core* snes, uint8_t bank, uint16_t addr)
{
    uint8_t data = snes->mem.open_bus;

    // NOTE: not sure if there's anything special about bank 0xFF
    // in terms of the reset vectors, so below is a way to have bank 0xFF
    // be seen as case 0x80
    // switch ((bank & 0x7F) + (bank == 0xFF))

    switch (bank & 0x7F)
    {
        case 0x00 ... 0x3F:
            switch (addr)
            {
                case 0x0000 ... 0x1FFF: // shadow ram
                    data = snes->mem.wram[addr];
                    break;

                case 0x2000 ... 0x5FFF: // hardware registers
                    snes_log_err("reading from hw regs! bank: 0x%02X addr: 0x%04X\n", bank, addr);
                    break;

                case 0x6000 ... 0x7FFF: // Expansion ram
                    snes_log_err("reading from expansion ram! bank: 0x%02X addr: 0x%04X\n", bank, addr);
                    break;

                case 0x8000 ... 0xFFFF: // 32k rom
                    data = snes->rom[(addr & 0x3FFF) + (0x4000 * bank)];
                    break;
            }
            break;

        case 0x40 ... 0x7C: // rom
            data = snes->rom[(addr & 0x3FFF) + (0x4000 * bank)];
            break;

        case 0x7D: // sram
            snes_log_err("reading from sram! bank: 0x%02X addr: 0x%04X\n", bank, addr);
            break;

        case 0x7E: // wram (1st 64K)
            data = snes->mem.wram[addr];
            break;

        case 0x7F: // wram (2nd 64K)
            data = snes->mem.wram[addr | 0x10000];
            break;

        // see above note about bank 0xFF
        // case 0x80: // wram bank 0xFF (reset and nmi vectors)
        //     data = snes->rom[(addr & 0x3FFF) * bank];
        //     break;
    }

    snes->mem.open_bus = data;
    return data;
}

void snes_cpu_write8(struct SNES_Core* snes, uint8_t bank, uint16_t addr, uint8_t value)
{
    snes->mem.open_bus = value;

    switch (bank & 0x7F)
    {
        case 0x00 ... 0x3F:
            switch (addr)
            {
                case 0x0000 ... 0x1FFF: // shadow ram
                    snes->mem.wram[addr] = value;
                    break;

                case 0x2000 ... 0x5FFF: // hardware registers
                    snes_log_err("reading from hw regs! bank: 0x%02X addr: 0x%04X\n", bank, addr);
                    break;

                case 0x6000 ... 0x7FFF: // Expansion ram
                    snes_log_err("reading from expansion ram! bank: 0x%02X addr: 0x%04X\n", bank, addr);
                    break;

                case 0x8000 ... 0xFFFF: // 32k rom
                    snes_log_err("writing to rom! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
                    break;
            }
            break;

        case 0x40 ... 0x7C: // rom
            snes_log_err("writing to rom! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
            break;

        case 0x7D: // sram
            snes_log_err("writing to sram! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
            break;

        case 0x7E: // wram (1st 64K)
            snes->mem.wram[addr] = value;
            break;

        case 0x7F: // wram (2nd 64K)
            snes->mem.wram[addr | 0x10000] = value;
            break;
    }
}

// todo: check if not 16-bit aligned addr.
// will 00:FFFF wrap around to 01:0000 or 00:0000?
uint16_t snes_cpu_read16(struct SNES_Core* snes, uint8_t bank, uint16_t addr)
{
    const uint8_t lo = snes_cpu_read8(snes, bank, addr + 0);
    const uint8_t hi = snes_cpu_read8(snes, bank, addr + 1);

    return (hi << 8) | lo;
}

void snes_cpu_write16(struct SNES_Core* snes, uint8_t bank, uint16_t addr, uint16_t value)
{
    snes_cpu_write8(snes, bank, addr + 0, value & 0xFF);
    snes_cpu_write8(snes, bank, addr + 1, value >> 8);
}

#if 0
// SOURCE: https://wiki.superfamicom.org/memory-mapping
uint8_t snes_cpu_read8(struct SNES_Core* snes, uint16_t addr)
{
    const uint8_t bank = addr >> 16;

    switch (bank)
    {
        case 0x00 ... 0x3F:
        case 0x80 ... 0xBF:
            switch (addr & 0xFF)
            {
                case 0x0000 ... 0x1FFF:
                    // [SLOW] addr bus A + /WRAM (mirror 0x7E:0000-0x1FFF)
                    break;

                case 0x2000 ... 0x20FF:
                    // [SLOW] addr bus A
                    break;

                case 0x2100 ... 0x21FF:
                    // [FAST] addr bus B
                    break;

                case 0x2200 ... 0x3FFF:
                    // [FAST] addr bus A
                    break;

                case 0x4000 ... 0x41FF:
                    // [XSLOW] internal cpu registers
                    break;

                case 0x4200 ... 0x43FF:
                    // [FAST] internal cpu registers
                    break;

                case 0x4400 ... 0x5FFF:
                    // [FAST] addr bus A
                    break;

                case 0x6000 ... 0x7FFF:
                    // [SLOW] addr bus A
                    break;

                case 0x8000 ... 0xFFFF:
                    // [SLOW] addr bus A + /CART
                    break;
            }
            break;

        case 0x40 ... 0x7D:
        case 0xC0 ... 0xFF:
            // [SLOW] addr bus A + /CART
            break;

        case 0x7E ... 0x7F:
            // [SLOW] addr bus A + /WRAM
            break;
    }

    return 0;
}

void snes_cpu_write8(struct SNES_Core* snes, uint16_t addr, uint8_t value)
{
    const uint8_t bank = addr >> 16;
    snes->mem.open_bus = value;

    switch (bank)
    {
        case 0x00 ... 0x3F:
        case 0x80 ... 0xBF:
            switch (addr & 0xFF)
            {
                case 0x0000 ... 0x1FFF:
                    // [SLOW] addr bus A + /WRAM (mirror 0x7E:0000-0x1FFF)
                    break;

                case 0x2000 ... 0x20FF:
                    // [SLOW] addr bus A
                    break;

                case 0x2100 ... 0x21FF:
                    // [FAST] addr bus B
                    break;

                case 0x2200 ... 0x3FFF:
                    // [FAST] addr bus A
                    break;

                case 0x4000 ... 0x41FF:
                    // [XSLOW] internal cpu registers
                    break;

                case 0x4200 ... 0x43FF:
                    // [FAST] internal cpu registers
                    break;

                case 0x4400 ... 0x5FFF:
                    // [FAST] addr bus A
                    break;

                case 0x6000 ... 0x7FFF:
                    // [SLOW] addr bus A
                    break;

                case 0x8000 ... 0xFFFF:
                    // [SLOW] addr bus A + /CART
                    break;
            }
            break;

        case 0x40 ... 0x7D:
        case 0xC0 ... 0xFF:
            // [SLOW] addr bus A + /CART
            break;

        case 0x7E ... 0x7F:
            // [SLOW] addr bus A + /WRAM
            break;
    }
}
#endif
