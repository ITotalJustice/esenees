#include "internal.h"
#include "bit.h"
#include <stdint.h>
#include <string.h>


// registers
#define REG_PC snes->apu.PC
#define REG_SP snes->apu.SP
#define REG_A snes->apu.A
#define REG_X snes->apu.X
#define REG_Y snes->apu.Y

// flags
#define FLAG_N snes->apu.flag_N
#define FLAG_V snes->apu.flag_V
#define FLAG_P snes->apu.flag_P
#define FLAG_B snes->apu.flag_B
#define FLAG_H snes->apu.flag_H
#define FLAG_I snes->apu.flag_I
#define FLAG_Z snes->apu.flag_Z
#define FLAG_C snes->apu.flag_C


static const uint8_t IPL_ROM[] =
{
    0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0, 0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
    0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4, 0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
    0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB, 0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
    0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD, 0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF,
};

static uint8_t snes_apu_read8(struct SNES_Core* snes, uint16_t addr)
{
    uint8_t value = 0x00;

    switch (addr)
    {
        case 0x0000 ... 0x00EF: // page 0
            value = snes->apu.ram[addr];
            break;

        case 0x00F2: // dsp_addr [R/W]
            value = snes->apu.dsp_addr;
            break;

        case 0x00F3: // dsp_data [R/W]
            value = snes->apu.dsp_data;
            break;

        case 0x00F4 ... 0x00F7: // portx [R/W]
            value = snes->apu.port_out[addr - 0x00F4];
            break;

        case 0x00F8: // rm0 [R/W]
            value = snes->apu.rm0;
            break;

        case 0x00F9: // rm1 [R/W]
            value = snes->apu.rm1;
            break;

        case 0x00FD ... 0x00FF: // counter0 [R]
            value = snes->apu.counter[addr - 0x00FD];
            break;

        case 0x0100 ... 0x01FF: // page 1
            value = snes->apu.ram[addr];
            break;

        case 0x0200 ... 0xFFBF: // memory
            value = snes->apu.ram[addr];
            break;

        case 0xFFC0 ... 0xFFFF: // memory (R/W, IPL ROM R - depending)
            if (snes->apu.ipl_enable)
            {
                value = IPL_ROM[addr - 0xFFC0];
            }
            else
            {
                value = snes->apu.ram[addr];
            }
            break;
    }

    return value;
}

static void snes_apu_write8(struct SNES_Core* snes, uint16_t addr, const uint8_t value)
{
    switch (addr)
    {
        case 0x0000 ... 0x00EF: // page 0
            snes->apu.ram[addr] = value;
            break;

        case 0x00F0: // unk [W]
            snes_log_fatal("[APU] writing to unk register: 0x%02X\n", value);
            break;

        case 0x00F1: // control [W]
            snes->apu.ipl_enable = is_bit_set(7, value);
            snes->apu.timer_enable[2] = is_bit_set(2, value);
            snes->apu.timer_enable[1] = is_bit_set(1, value);
            snes->apu.timer_enable[0] = is_bit_set(0, value);

            // if set, resets the input ports 0,1
            if (is_bit_set(5, value))
            {
                snes->apu.port_in[0] = 0x00;
                snes->apu.port_in[1] = 0x00;
            }

            // if set, resets input ports, 2,3
            if (is_bit_set(4, value))
            {
                snes->apu.port_in[2] = 0x00;
                snes->apu.port_in[3] = 0x00;
            }
            break;

        case 0x00F2: // dsp_addr [R/W]
            snes->apu.dsp_addr = value;
            break;

        case 0x00F3: // dsp_data [R/W]
            snes->apu.dsp_data = value;
            break;

        case 0x00F4 ... 0x00F7: // portx [R/W]
            snes->apu.port_in[addr - 0x00F4] = value;
            break;

        case 0x00F8: // rm0 [R/W]
            snes->apu.rm0 = value;
            break;

        case 0x00F9: // rm1 [R/W]
            snes->apu.rm1 = value;
            break;

        case 0x00FA ... 0x00FC: // timerx [W]
            snes->apu.timer[addr - 0x00FA] = value;
            break;

        case 0x0100 ... 0x01FF: // page 1
            snes->apu.ram[addr] = value;
            break;

        case 0x0200 ... 0xFFFF: // memory
            snes->apu.ram[addr] = value;
            break;
    }
}

uint16_t snes_apu_read16(struct SNES_Core* snes, uint16_t addr)
{
    const uint16_t lo = snes_apu_read8(snes, addr + 0);
    const uint16_t hi = snes_apu_read8(snes, addr + 1);

    return (hi << 8) | lo;
}

void snes_apu_write16(struct SNES_Core* snes, uint16_t addr, uint16_t value)
{
    snes_apu_write8(snes, addr + 0, (value >> 0) & 0xFF);
    snes_apu_write8(snes, addr + 1, (value >> 8) & 0xFF);
}

bool snes_apu_init(struct SNES_Core* snes)
{
    // set control reg inital value (allow reads from ipl)
    snes_apu_write8(snes, 0x00F1, 0xB0);

    // on power up, the values are 0xF, they are 0x0 on reset
    snes_apu_write8(snes, 0x00FD, 0x0F);
    snes_apu_write8(snes, 0x00FE, 0x0F);
    snes_apu_write8(snes, 0x00FF, 0x0F);

    REG_PC = snes_apu_read16(snes, 0xFFFE);
    REG_SP = 0x00;
    REG_A = 0x00;
    REG_X = 0x00;
    REG_Y = 0x00;

    return true;
}
