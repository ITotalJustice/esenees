#include "internal.h"
#include "bit.h"
#include "types.h"
#include <stdint.h>
#include <stdlib.h>


// NOTE: the below is for lorom mapping ONLY.
// hirom will be implemented later on, once everything is already working

static uint8_t fast_rand(void)
{
    static uint8_t v = 0;
    return v++;
}

static void io_write_INIDISP(struct SNES_Core* snes, uint8_t value)
{
    snes->mem.INIDISP.forced_blanking = is_bit_set(7, value);
    snes->mem.INIDISP.master_brightness = get_bit_range(0, 3, value);

    if (snes->mem.INIDISP.forced_blanking)
    {
        snes_log("forced blank enabled! screen is black!\n");
    }
}

static void io_write_OAMADDL(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.oam_addr = (snes->ppu.oam_addr & 0xFF00) | value;
}

static void io_write_OAMADDH(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.oam_addr = (snes->ppu.oam_addr & 0xFF) | ((value & 0x1) << 8);
    snes->ppu.obj_priority_actiavtion = is_bit_set(7, value);
}

static void io_write_VMAIN(struct SNES_Core* snes, uint8_t value)
{
    const uint8_t VRAM_STEP[] = { 0x01, 0x20, 0x80, 0x80 };

    snes->ppu.vram_addr_increment_mode = is_bit_set(7, value);
    snes->ppu.vram_addr_step = VRAM_STEP[get_bit_range(0, 1, value)];
    // todo: theres another 2 bits that im not saving!
}

static void io_write_VMADDL(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.vram_addr = (snes->ppu.vram_addr & 0xFF00) | value;
}

static void io_write_VMADDH(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.vram_addr = (snes->ppu.vram_addr & 0xFF) | (value << 8);
}

static void io_write_VMDATAL(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.vram[snes->ppu.vram_addr] = (snes->ppu.vram[snes->ppu.vram_addr] & 0xFF00) | value;

    if (snes->ppu.vram_addr_increment_mode == false)
    {
        snes->ppu.vram_addr += snes->ppu.vram_addr_step;
        snes->ppu.vram_addr &= 0x7FFF;
    }
}

static void io_write_VMDATAH(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.vram[snes->ppu.vram_addr] = (snes->ppu.vram[snes->ppu.vram_addr] & 0xFF) | (value << 8);

    if (snes->ppu.vram_addr_increment_mode == true)
    {
        snes->ppu.vram_addr += snes->ppu.vram_addr_step;
        snes->ppu.vram_addr &= 0x7FFF;
    }
}

static void io_write_CGADD(struct SNES_Core* snes, uint8_t value)
{
    snes->ppu.cgram_addr = value;
}

static void io_write_CGDATA(struct SNES_Core* snes, uint8_t value)
{
    if (snes->ppu.cgram_flipflop)
    {
        snes->ppu.cgram_flipflop = false;
        snes->ppu.cgram[snes->ppu.cgram_addr] = (value << 8) | snes->ppu.cgram_cached_byte;
    }
    else
    {
        snes->ppu.cgram_flipflop = true;
        snes->ppu.cgram_cached_byte = value;
    }
}

static void io_write_NMITIMEN(struct SNES_Core* snes, uint8_t value)
{
    snes->mem.NMITIMEN.vblank_enable = is_bit_set(7, value);
    snes->mem.NMITIMEN.irq = get_bit_range(4, 5, value);
    snes->mem.NMITIMEN.joypad_enable = is_bit_set(0, value);
}

static void io_write_MDMAEN(struct SNES_Core* snes, uint8_t value)
{
    snes->mem.MDMAEN = value;

    if (value)
    {
        snes_log("[IO-MDMAEN] channel enabled! DMA needs to start: 0x%02X\n", value);
    }
}

static void io_write_HDMAEN(struct SNES_Core* snes, uint8_t value)
{
    snes->mem.HDMAEN = value;
    // todo: check if anything happens should a hdma channel
    // be enabled via this write!
}

static void io_write_MEMSEL(struct SNES_Core* snes, uint8_t value)
{
    snes->mem.MEMSEL = value & 0x1;
}

static uint8_t snes_io_read(struct SNES_Core* snes, uint16_t addr)
{
    uint8_t value = 0xFF;

    switch (addr)
    {
        case 0x2139: // VMDATALREAD
            value = snes->ppu.vram[snes->ppu.vram_addr] >> 0;
            break;

        case 0x213A: // VMDATAHREAD
            value = snes->ppu.vram[snes->ppu.vram_addr] >> 8;
            break;

        case 0x2140: // APUIO0
            // snes_log("[APUIO0] WARNING - ignoring read\n");
            value = fast_rand();
            break;

        case 0x2141: // APUIO1
            // snes_log("[APUIO1] WARNING - ignoring read\n");
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
            io_write_INIDISP(snes, value);
            break;

        case 0x2101: // OBSEL
            snes_log("[OBSEL] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2102: // OAMADDL
            io_write_OAMADDL(snes, value);
            break;

        case 0x2103: // OAMADDH
            io_write_OAMADDH(snes, value);
            break;

        case 0x2104: // OAMDATA
            snes_log_fatal("[OAMDATA] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2106: // MOSAIC
            snes_log("[MOSAIC] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2107: // BG1SC (BG tilemap addr reg BG1)
            snes_log("[BG1SC] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2108: // BG2SC (BG tilemap addr reg BG2)
            snes_log("[BG2SC] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2109: // BG3SC (BG tilemap addr reg BG3)
            snes_log("[BG3SC] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x210A: // BG4SC (BG tilemap addr reg BG4)
            snes_log("[BG4SC] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x210B: // BG12NBA (BG character addr reg BG1&2)
            snes_log("[BG12NBA] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x210C: // BG34NBA (BG character addr reg BG3&4)
            snes_log("[BG34NBA] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2115: // VMAIN
            io_write_VMAIN(snes, value);
            break;

        case 0x2116: // VMADDL (vram addr low)
            io_write_VMADDL(snes, value);
            break;

        case 0x2117: // VMADDH (vram addr high)
            io_write_VMADDH(snes, value);
            break;

        case 0x2118: // VMDATAL (vram data low)
            io_write_VMDATAL(snes, value);
            break;

        case 0x2119: // VMDATAH (vram data high)
            io_write_VMDATAH(snes, value);
            break;

        case 0x211A: // M7SEL (mode 7 settings reg)
            snes_log("[M7SEL] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2121: // CGADD (CGRAM addr)
            io_write_CGADD(snes, value);
            break;

        case 0x2122: // CGDATA (CGRAM data)
            io_write_CGDATA(snes, value);
            break;

        case 0x212A: // WBGLOG (window mask logic reg BG)
            snes_log("[WBGLOG] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x212B: // WOBJLOG (window mask logic reg OBJ)
            snes_log("[WOBJLOG] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x212C: // TM (screen destination reg)
            snes_log("[TM] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x212D: // TD (screen destination reg)
            snes_log("[TD] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x212E: // TMW (window destination reg)
            snes_log("[TMW] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x212F: // TSW (window destination reg)
            snes_log("[TSW] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2131: // CGADSUB (colour math)
            snes_log("[CGADSUB] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2132: // COLDATA (colour math)
            snes_log("[COLDATA] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2133: // SETINI (screen mode select register)
            snes_log("[SETINI] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2140: // APUIO0
            // snes_log("[APUIO0] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2141: // APUIO1
            // snes_log("[APUIO1] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2142: // APUIO2
            // snes_log("[APUIO2] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x2143: // APUIO3
            // snes_log("[APUIO3] WARNING - ignoring write: 0x%02X\n", value);
            break;

        case 0x4200: // NMITIMEN
            io_write_NMITIMEN(snes, value);
            break;

        case 0x420B: // MDMAEN
            io_write_MDMAEN(snes, value);
            break;

        case 0x420C: // HDMAEN
            io_write_HDMAEN(snes, value);
            break;

        case 0x420D: // MEMSEL
            io_write_MEMSEL(snes, value);
            break;

        case 0x4300 ... 0x437F:
            switch (addr & 0xF)
            {
                case 0x0: // DMAPx
                    snes_log("[DMAPx] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x1: // BBADx
                    snes_log("[BBADx] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x2: // A1TxL
                    snes_log("[A1TxL] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x3: // A1TxH
                    snes_log("[A1TxH] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x4: // A1Bx
                    snes_log("[A1Bx] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x5: // DASxL
                    snes_log("[DASxL] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x6: // DASxH
                    snes_log("[DASxH] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x7: // DASBx
                    snes_log("[DASBx] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x8: // A2AxL
                    snes_log("[A2AxL] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0x9: // A2AxH
                    snes_log("[A2AxH] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0xA: // NTRLx
                    snes_log("[NTRLx] WARNING - ignoring write: 0x%02X channel: %u\n", value, (addr >> 4) & 0xF);
                    break;

                case 0xB: // UNUSED (unused byte r/w)
                    snes_log_fatal("unused byte write - addr: 0x%04X value: 0x%02X\n", addr, value);
                    break;

                case 0xC ... 0xF: // open bus
                    snes_log_fatal("openbus write - addr: 0x%04X value: 0x%02X\n", addr, value);
                    break;
            }
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
                    data = snes->rom[(addr & 0x7FFF) + (0x8000 * (bank & 0x7F))];
                    break;
            }
            break;

        case 0x40 ... 0x7C: // rom
            data = snes->rom[(addr & 0x7FFF) + (0x8000 * (bank & 0x7F))];
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
