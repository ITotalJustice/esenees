#include "bit.h"
#include "internal.h"
#include "types.h"
#include <stdint.h>
#include <string.h>


// registers
#define REG_PC snes->cpu.PC
#define REG_SP snes->cpu.SP
#define REG_A snes->cpu.A
#define REG_X snes->cpu.X
#define REG_Y snes->cpu.Y
#define REG_D snes->cpu.D
#define REG_PBR snes->cpu.PBR
#define REG_DBR snes->cpu.DBR

// flags
#define FLAG_C snes->cpu.flag_C
#define FLAG_Z snes->cpu.flag_Z
#define FLAG_I snes->cpu.flag_I
#define FLAG_D snes->cpu.flag_D
#define FLAG_X snes->cpu.flag_X
#define FLAG_M snes->cpu.flag_M
#define FLAG_V snes->cpu.flag_V
#define FLAG_N snes->cpu.flag_N
#define FLAG_E snes->cpu.flag_E


bool snes_cpu_init(struct SNES_Core* snes)
{
    // todo: setup default values of registers!
    memset(&snes->cpu, 0, sizeof(snes->cpu));
    REG_PC = 0x8000; // im guessing this
    REG_SP = 0x01FF;
    FLAG_E = true;
    FLAG_M = true;
    FLAG_X = true;
    FLAG_I = true;
    return true;
}

static void snes_set_status_flags(struct SNES_Core* snes, uint8_t value)
{
    FLAG_C = is_bit_set(0, value);
    FLAG_Z = is_bit_set(1, value);
    FLAG_I = is_bit_set(2, value);
    FLAG_D = is_bit_set(3, value);
    FLAG_X = is_bit_set(4, value);
    FLAG_M = is_bit_set(5, value);
    FLAG_V = is_bit_set(6, value);
    FLAG_N = is_bit_set(7, value);
}

static uint8_t snes_get_status_flags(const struct SNES_Core* snes)
{
    uint8_t value = 0;
    value |= FLAG_C << 0;
    value |= FLAG_Z << 1;
    value |= FLAG_I << 2;
    value |= FLAG_D << 3;
    value |= FLAG_X << 4;
    value |= FLAG_M << 5;
    value |= FLAG_V << 6;
    value |= FLAG_N << 7;
    return value;
}

// basically add the bank byte to addr
static uint32_t addr(uint32_t bank_byte, uint16_t addr)
{
    return ((bank_byte << 16) | addr) & 0x00FFFFFF;
}

// addressing modes
static void implied(struct SNES_Core* snes)
{
    (void)snes; // does nothing
}

static void absolute(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read16(snes, addr(REG_PBR, REG_PC));
    snes->cpu.oprand = addr(REG_DBR, snes->cpu.oprand);
    snes_log("[ABS] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 2;
}

static void absolute_long(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read24(snes, addr(REG_PBR, REG_PC));
    // snes->cpu.oprand = addr(REG_DBR, snes->cpu.oprand);
    snes_log("[ABS LONG] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 3;
}

static void immediate(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        snes->cpu.oprand = addr(REG_PBR, REG_PC++);
    }
    else
    {
        snes->cpu.oprand = addr(REG_PBR, REG_PC);
        REG_PC += 2;
    }
    snes_log("[IMM] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}


// set carry flag
static void SEC(struct SNES_Core* snes)
{
    FLAG_C = true;
}

// set decimal flag
static void SED(struct SNES_Core* snes)
{
    FLAG_D = true;
}

// set interrupt disable flag
static void SEI(struct SNES_Core* snes)
{
    FLAG_I = true;
}

// store zero to memory
static void STZ(struct SNES_Core* snes)
{
    snes_cpu_write8(snes, snes->cpu.oprand, 0);
}

// load x from memory
static void LDX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_X = snes_cpu_read8(snes, snes->cpu.oprand);
        FLAG_N = is_bit_set(7, REG_X);
        FLAG_Z = REG_X == 0;
    }
    else
    {
        REG_X = snes_cpu_read16(snes, snes->cpu.oprand);
        FLAG_N = is_bit_set(15, REG_X);
        FLAG_Z = REG_X == 0;
    }
}

// load y from memory
static void LDY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_Y = snes_cpu_read8(snes, snes->cpu.oprand);
        FLAG_N = is_bit_set(7, REG_Y);
        FLAG_Z = REG_Y == 0;
    }
    else
    {
        REG_Y = snes_cpu_read16(snes, snes->cpu.oprand);
        FLAG_N = is_bit_set(15, REG_Y);
        FLAG_Z = REG_Y == 0;
    }
}

// load accumulator from memory
static void LDA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        REG_A = snes_cpu_read8(snes, snes->cpu.oprand);
        FLAG_N = is_bit_set(7, REG_A);
        FLAG_Z = REG_A == 0;
    }
    else
    {
        REG_A = snes_cpu_read16(snes, snes->cpu.oprand);
        FLAG_N = is_bit_set(15, REG_A);
        FLAG_Z = REG_A == 0;
    }
}

// store accumulator to memory
static void STA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        snes_cpu_write8(snes, snes->cpu.oprand, REG_A);
    }
    else
    {
        snes_cpu_write16(snes, snes->cpu.oprand, REG_A);
    }
}

// clear carry flag
static void CLC(struct SNES_Core* snes)
{
    FLAG_C = false;
}

// exchange carry and emulation flags
static void XCE(struct SNES_Core* snes)
{
    const bool old_carry = FLAG_C;
    FLAG_C = FLAG_E;
    FLAG_E = old_carry;
    snes_log("[CPU] switch mode to %s\n", FLAG_E ? "EMULATED" : "NATIVE");
    if (FLAG_E)
    {
        snes_log_fatal("[CPU] emulation mode no supported!\n");
    }
}

// clear the bits specified in the oprand of the flags
static void REP(struct SNES_Core* snes)
{
    const uint8_t value = ~snes_cpu_read8(snes, snes->cpu.oprand);
    snes_set_status_flags(snes, value);
}

// transfer accumulator to REG_X
static void TAX(struct SNES_Core* snes)
{
    REG_X = REG_A;
    FLAG_N = is_bit_set(15, REG_X);
    FLAG_Z = REG_X == 0;
}

// transfer accumulator to REG_Y
static void TAY(struct SNES_Core* snes)
{
    REG_Y = REG_A;
    FLAG_N = is_bit_set(15, REG_Y);
    FLAG_Z = REG_Y == 0;
}

// transfer accumulator to direct page register
static void TCD(struct SNES_Core* snes)
{
    REG_D = REG_A;
    FLAG_N = is_bit_set(15, REG_D);
    FLAG_Z = REG_D == 0;
}

// transfer accumulator to stack pointer
static void TCS(struct SNES_Core* snes)
{
    REG_SP = REG_A;
    FLAG_N = is_bit_set(15, REG_SP);
    FLAG_Z = REG_SP == 0;
}

// transfer direct page register to accumulator
static void TDC(struct SNES_Core* snes)
{
    REG_A = REG_D;
    FLAG_N = is_bit_set(15, REG_A);
    FLAG_Z = REG_A == 0;
}

void snes_cpu_run(struct SNES_Core* snes)
{
    const uint8_t opcode = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++));

    switch (opcode)
    {
        case 0x18: implied(snes);           CLC(snes); break;
        case 0x1B: implied(snes);           TCS(snes); break;
        case 0x38: implied(snes);           SEC(snes); break;
        case 0x5B: implied(snes);           TCD(snes); break;
        case 0x78: implied(snes);           SEI(snes); break;
        case 0x7B: implied(snes);           TDC(snes); break;
        case 0x8D: absolute(snes);          STA(snes); break;
        case 0x8F: absolute_long(snes);     STA(snes); break;
        case 0x9C: absolute(snes);          STZ(snes); break;
        case 0xA0: immediate(snes);         LDY(snes); break;
        case 0xA2: immediate(snes);         LDX(snes); break;
        case 0xA8: implied(snes);           TAY(snes); break;
        case 0xA9: immediate(snes);         LDA(snes); break;
        case 0xAA: implied(snes);           TAX(snes); break;
        case 0xC2: immediate(snes);         REP(snes); break;
        case 0xF8: implied(snes);           SED(snes); break;
        case 0xFB: implied(snes);           XCE(snes); break;

        default:
            snes_log_fatal("UNK opcode: 0x%02X REG_PC: 0x%04X REG_PBR: 0x%02X\n", opcode, REG_PC, REG_PBR);
            break;
    }
}
