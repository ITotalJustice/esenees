#include "internal.h"
#include "bit.h"
#include <stdint.h>
#include <stdlib.h>


// NOTE: the below is for lorom mapping ONLY.
// hirom will be implemented later on, once everything is already working

static uint8_t snes_io_read(struct SNES_Core* snes, uint16_t addr)
{
    uint8_t value = 0xFF;

    switch (addr)
    {
        case 0x2140: // APUIO0
            snes_log("[APUIO0] WARNING - ignoring read\n");
            value = rand();
            break;

        case 0x2141: // APUIO1
            snes_log("[APUIO1] WARNING - ignoring read\n");
            value = rand();
            break;

        default:
            snes_log_fatal("[IO] unhandled read! addr: 0x%04X\n", addr);
            break;
    }

    return value;
}

static void snes_io_write(struct SNES_Core* snes, uint16_t addr, uint8_t value)
{
    switch (addr)
    {
        case 0x2100: // INIDISP
            snes->mem.INIDISP.forced_blanking = is_bit_set(7, value);
            snes->mem.INIDISP.master_brightness = get_bit_range(0, 3, value);
            if (snes->mem.INIDISP.forced_blanking)
            {
                snes_log("forced blank enabled! screen is black!\n");
            }
            break;

        case 0x2140: // APUIO0
            snes_log("[APUIO0] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2141: // APUIO1
            snes_log("[APUIO1] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2142: // APUIO2
            snes_log("[APUIO2] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2143: // APUIO3
            snes_log("[APUIO3] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x4200: // NMITIMEN
            snes->mem.NMITIMEN.vblank_enable = is_bit_set(7, value);
            snes->mem.NMITIMEN.irq = get_bit_range(4, 5, value);
            snes->mem.NMITIMEN.joypad_enable = is_bit_set(0, value);
            break;

        case 0x420B: // MDMAEN
            snes->mem.MDMAEN = value;
            if (value)
            {
                snes_log_fatal("[IO-MDMAEN] channel enabled! DMA needs to start: 0x%02X\n", value);
            }
            break;

        case 0x420C: // HDMAEN
            snes->mem.HDMAEN = value;
            // todo: check if anything happens should a hdma channel
            // be enabled via this write!
            break;

        default:
            snes_log_fatal("[IO] unhandled write! addr: 0x%04X value: 0x%02X\n", addr, value);
            break;
    }
}

uint8_t snes_cpu_read8(struct SNES_Core* snes, uint32_t addr)
{
    const uint8_t bank = (addr >> 16) & 0xFF;
    addr &= 0xFFFF;
    uint8_t data = snes->mem.open_bus;

    switch (bank & 0x7F)
    {
        case 0x00 ... 0x3F:
            switch (addr)
            {
                case 0x0000 ... 0x1FFF: // shadow ram
                    data = snes->mem.wram[addr];
                    break;

                case 0x2000 ... 0x5FFF: // hardware registers
                    data = snes_io_read(snes, addr);
                    break;

                case 0x6000 ... 0x7FFF: // Expansion ram
                    snes_log_fatal("reading from expansion ram! bank: 0x%02X addr: 0x%04X\n", bank, addr);
                    break;

                case 0x8000 ... 0xFFFF: // 32k rom
                    data = snes->rom[(addr & 0x7FFF) + (0x8000 * bank)];
                    break;
            }
            break;

        case 0x40 ... 0x7C: // rom
            data = snes->rom[(addr & 0x7FFF) + (0x8000 * bank)];
            break;

        case 0x7D: // sram
            snes_log_fatal("reading from sram! bank: 0x%02X addr: 0x%04X\n", bank, addr);
            break;

        case 0x7E: // wram (1st 64K)
            data = snes->mem.wram[addr];
            break;

        case 0x7F: // wram (2nd 64K)
            data = snes->mem.wram[addr | 0x10000];
            break;
    }

    snes->mem.open_bus = data;
    return data;
}

void snes_cpu_write8(struct SNES_Core* snes, uint32_t addr, uint8_t value)
{
    const uint8_t bank = (addr >> 16) & 0xFF;
    addr &= 0xFFFF;
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
                    snes_io_write(snes, addr, value);
                    break;

                case 0x6000 ... 0x7FFF: // Expansion ram
                    snes_log_fatal("writing to expansion ram! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
                    break;

                case 0x8000 ... 0xFFFF: // 32k rom
                    snes_log_fatal("writing to rom! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
                    break;
            }
            break;

        case 0x40 ... 0x7C: // rom
            snes_log_fatal("writing to rom! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
            break;

        case 0x7D: // sram
            snes_log_fatal("writing to sram! bank: 0x%02X addr: 0x%04X value: 0x%02X\n", bank, addr, value);
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
uint16_t snes_cpu_read16(struct SNES_Core* snes, uint32_t addr)
{
    const uint16_t lo = snes_cpu_read8(snes, addr + 0);
    const uint16_t hi = snes_cpu_read8(snes, addr + 1);

    return (hi << 8) | lo;
}

uint32_t snes_cpu_read24(struct SNES_Core* snes, uint32_t addr)
{
    const uint32_t lo = snes_cpu_read16(snes, addr + 0);
    const uint32_t hi = snes_cpu_read16(snes, addr + 2);

    return ((hi << 16) | lo) & 0x00FFFFFF;
}

void snes_cpu_write16(struct SNES_Core* snes, uint32_t addr, uint16_t value)
{
    snes_cpu_write8(snes, addr + 0, (value >> 0) & 0xFF);
    snes_cpu_write8(snes, addr + 1, (value >> 8) & 0xFF);
}

// this won't be needed i don't think
// void snes_cpu_write24(struct SNES_Core* snes, uint32_t addr, uint16_t value)
// {
//     snes_cpu_write16(snes, addr + 0, (value >> 0) & 0xFF);
//     snes_cpu_write16(snes, addr + 1, (value >> 8) & 0xFF);
// }
