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
    // fetch from reset vector
    REG_PC = snes_cpu_read16(snes, 0xFFFC);
    // below are taken from bsnes-plus
    REG_SP = 0x01FF;
    FLAG_E = true;
    FLAG_M = true;
    FLAG_X = true;
    FLAG_I = true;

    snes_log("initial pc: 0x%04X\n", REG_PC);

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

    if (FLAG_X)
    {
        REG_X &= 0xFF;
        REG_Y &= 0xFF;
    }
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
    // snes_log("[ABS] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 2;
}

static void absolute_x(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read16(snes, addr(REG_PBR, REG_PC));
    snes->cpu.oprand = addr(REG_DBR, snes->cpu.oprand + REG_X);
    // snes_log("[ABS X] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 2;
}

static void absolute_y(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read16(snes, addr(REG_PBR, REG_PC));
    snes->cpu.oprand = addr(REG_DBR, snes->cpu.oprand + REG_Y);
    // snes_log("[ABS Y] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 2;
}

static void absolute_long(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read24(snes, addr(REG_PBR, REG_PC));
    // snes_log("[ABS LONG] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 3;
}

static void absolute_indirect_long(struct SNES_Core* snes)
{
    const uint16_t base = snes_cpu_read16(snes, addr(REG_PBR, REG_PC));
    snes->cpu.oprand = snes_cpu_read24(snes, base);
    // snes_log("[ABS INDIRECT LONG] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 2;
}

static void absolute_long_x(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read24(snes, addr(REG_PBR, REG_PC)) + REG_X;
    // snes_log("[ABS LONG X] oprand effective address: 0x%06X\n", snes->cpu.oprand);

    REG_PC += 3;
}

static void immediate8(struct SNES_Core* snes)
{
    snes->cpu.oprand = addr(REG_PBR, REG_PC++);
    // snes_log("[IMM8] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

static void immediate16(struct SNES_Core* snes)
{
    snes->cpu.oprand = addr(REG_PBR, REG_PC);
    REG_PC += 2;
    // snes_log("[IMM16] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

// for instructions that use A, such as LDA
static void immediateM(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        immediate8(snes);
    }
    else
    {
        immediate16(snes);
    }

    // snes_log("[IMM8] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

// for instructions that use X/Y, such as LDA
static void immediateX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        immediate8(snes);
    }
    else
    {
        immediate16(snes);
    }

    // snes_log("[IMM8] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

static void relative(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++));

    // snes_log("[REL] value: %d result: 0x%02X\n", (int8_t)snes->cpu.oprand, REG_PC + (int8_t)snes->cpu.oprand);
}

static void direct_page(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++));
    snes->cpu.oprand = (snes->cpu.oprand + REG_D) & 0xFFFF;
    // snes_log("[DP] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

static void direct_page_x(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++));
    snes->cpu.oprand = (snes->cpu.oprand + REG_D + REG_X) & 0xFFFF;
    // snes_log("[DP X] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

static void direct_page_y(struct SNES_Core* snes)
{
    snes->cpu.oprand = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++));
    snes->cpu.oprand = (snes->cpu.oprand + REG_D + REG_Y) & 0xFFFF;
    // snes_log("[DP Y] oprand effective address: 0x%06X\n", snes->cpu.oprand);
}

static void dp_ind_long_y(struct SNES_Core* snes)
{
    // base + REG_D(indirect)
    const uint16_t base = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++)) + REG_D;
    snes->cpu.oprand = (snes_cpu_read24(snes, base) + REG_Y) & 0xFFFFFF;
    // snes_log("[DP IND LONG Y] oprand effective address: 0x%06X 0x%04X M: %u\n", snes->cpu.oprand, snes_cpu_read16(snes, snes->cpu.oprand), FLAG_M);
}

// for debugging, basically crash at pc and log stuff
static void breakpoint(const struct SNES_Core* snes, int pc)
{
    if (((REG_PBR << 16) | REG_PC) == pc)
    {
        snes_log_fatal(
            "[BP] PC: 0x%04X OP: 0x%02X oprand: 0x%06X REG_PBR: 0x%02X REG_A: 0x%04X S: 0x%04X X: 0x%04X Y: 0x%04X P: 0x%02X ticks: %zu\n",
            REG_PC, snes->opcode, snes->cpu.oprand, REG_PBR, REG_A, REG_SP, REG_X, REG_Y, snes_get_status_flags(snes), snes->ticks
        );
    }
}

// helper that only sets the low half of a 16-bit reg
static void set_lo_byte(uint16_t* reg, uint8_t byte)
{
    *reg = (*reg & 0xFF00) | byte;
}

// stack push / pop helpers
static void push8(struct SNES_Core* snes, uint8_t value)
{
    snes_cpu_write8(snes, REG_SP--, value);
}

static uint8_t pop8(struct SNES_Core* snes)
{
    return snes_cpu_read8(snes, ++REG_SP);
}

static void push16(struct SNES_Core* snes, uint16_t value)
{
    push8(snes, (value >> 8) & 0xFF);
    push8(snes, (value >> 0) & 0xFF);
}

static uint16_t pop16(struct SNES_Core* snes)
{
    const uint16_t lo = pop8(snes);
    const uint16_t hi = pop8(snes);

    return (hi << 8) | lo;
}

// helper for setting flags NZ
static void set_nz_8(struct SNES_Core* snes, uint8_t value)
{
    FLAG_N = is_bit_set(7, value);
    FLAG_Z = value == 0;
}

static void set_nz_16(struct SNES_Core* snes, uint16_t value)
{
    FLAG_N = is_bit_set(15, value);
    FLAG_Z = value == 0;
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

// clear carry flag
static void CLC(struct SNES_Core* snes)
{
    FLAG_C = false;
}

// clear decimal flag
static void CLD(struct SNES_Core* snes)
{
    FLAG_D = false;
}

// clear interrupt disable flag
static void CLI(struct SNES_Core* snes)
{
    FLAG_I = false;
}

// clear overflow flag
static void CLV(struct SNES_Core* snes)
{
    FLAG_V = false;
}

// store zero to memory
static void STZ(struct SNES_Core* snes)
{
    snes_cpu_write8(snes, snes->cpu.oprand, 0);
}

// store x to memory
static void STX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        snes_cpu_write8(snes, snes->cpu.oprand, REG_X);
    }
    else
    {
        snes_cpu_write16(snes, snes->cpu.oprand, REG_X);
    }
}

// store x to memory
static void STY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        snes_cpu_write8(snes, snes->cpu.oprand, REG_Y);
    }
    else
    {
        snes_cpu_write16(snes, snes->cpu.oprand, REG_Y);
    }
}

// load x from memory
static void LDX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_X = snes_cpu_read8(snes, snes->cpu.oprand);
        set_nz_8(snes, REG_X);
    }
    else
    {
        REG_X = snes_cpu_read16(snes, snes->cpu.oprand);
        set_nz_16(snes, REG_X);
    }
}

// load y from memory
static void LDY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_Y = snes_cpu_read8(snes, snes->cpu.oprand);
        set_nz_8(snes, REG_Y);
    }
    else
    {
        REG_Y = snes_cpu_read16(snes, snes->cpu.oprand);
        set_nz_16(snes, REG_Y);
    }
}

// load accumulator from memory
static void LDA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        set_lo_byte(&REG_A, value);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = snes_cpu_read16(snes, snes->cpu.oprand);
        set_nz_16(snes, REG_A);
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

// exchange carry and emulation flags
static void XCE(struct SNES_Core* snes)
{
    const bool old_carry = FLAG_C;
    FLAG_C = FLAG_E;
    FLAG_E = old_carry;
    snes_log("[XCE] switch mode to %s\n", FLAG_E ? "EMULATED" : "NATIVE");
    if (FLAG_E)
    {
        snes_log_fatal("[CPU] emulation mode no supported!\n");
    }
}

// exchange lo hi bytes of accumulator
static void XBA(struct SNES_Core* snes)
{
    const uint8_t lo_a = REG_A & 0xFF;
    const uint8_t hi_a = REG_A >> 8;
    REG_A = (lo_a << 8) | hi_a;

    // NOTE: not sure on this, might check 16bit only
    // if (FLAG_M)
    {
        // set_nz_8(snes, REG_A);
    }
    // else
    {
        set_nz_16(snes, REG_A);
    }
}

// clear the bits specified in the oprand of the flags
static void REP(struct SNES_Core* snes)
{
    const uint8_t value = ~snes_cpu_read8(snes, snes->cpu.oprand);
    const uint8_t flags = snes_get_status_flags(snes);
    snes_set_status_flags(snes, flags & value);
}

// set the bits specified in the oprand of the flags
static void SEP(struct SNES_Core* snes)
{
    const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
    const uint8_t flags = snes_get_status_flags(snes);
    snes_set_status_flags(snes, flags | value);
}

// transfer accumulator to REG_X
static void TAX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_X = REG_A & 0xFF;
        set_nz_8(snes, REG_X);
    }
    else
    {
        REG_X = REG_A;
        set_nz_16(snes, REG_X);
    }
}

// transfer accumulator to REG_Y
static void TAY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_Y = REG_A & 0xFF;
        set_nz_8(snes, REG_Y);
    }
    else
    {
        REG_Y = REG_A;
        set_nz_16(snes, REG_Y);
    }
}

// transfer REG_X to accumulator
static void TXA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        set_lo_byte(&REG_A, REG_X);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = REG_X;
        set_nz_16(snes, REG_A);
    }
}

// transfer REG_X to stack pointer
static void TXS(struct SNES_Core* snes)
{
    REG_SP = REG_X;
    set_nz_16(snes, REG_SP);
}

// transfer REG_X to REG_Y
static void TXY(struct SNES_Core* snes)
{
    REG_Y = REG_X;

    if (FLAG_X)
    {
        set_nz_8(snes, REG_Y);
    }
    else
    {
        set_nz_16(snes, REG_Y);
    }
}

// transfer REG_Y to accumulator
static void TYA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        set_lo_byte(&REG_A, REG_Y);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = REG_Y;
        set_nz_16(snes, REG_A);
    }
}

// transfer REG_Y to REG_X
static void TYX(struct SNES_Core* snes)
{
    REG_X = REG_Y;

    if (FLAG_X)
    {
        set_nz_8(snes, REG_X);
    }
    else
    {
        set_nz_16(snes, REG_X);
    }
}

// transfer accumulator to direct page register
static void TCD(struct SNES_Core* snes)
{
    REG_D = REG_A;
    set_nz_16(snes, REG_D);
}

// transfer accumulator to stack pointer
static void TCS(struct SNES_Core* snes)
{
    REG_SP = REG_A;
    set_nz_16(snes, REG_SP);
}

// transfer stack pointer to accumulator
static void TSC(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        set_lo_byte(&REG_A, REG_SP);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = REG_SP;
        set_nz_16(snes, REG_A);
    }
}

// transfer direct page register to accumulator
static void TDC(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        set_lo_byte(&REG_A, REG_D);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = REG_D;
        set_nz_16(snes, REG_A);
    }
}

// add with carry
static void ADC(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand) + FLAG_C;
        const uint8_t result = REG_A + value;

        FLAG_V = ((REG_A ^ result) & (value ^ result) & 0x80) > 0;
        FLAG_C = (REG_A + value) > 0xFFFF;
        set_nz_8(snes, result);

        set_lo_byte(&REG_A, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand) + FLAG_C;
        const uint16_t result = REG_A + value;

        FLAG_V = ((REG_A ^ result) & (value ^ result) & 0x8000) > 0;
        FLAG_C = (REG_A + value) > 0xFFFF;
        set_nz_16(snes, result);

        REG_A = result;
    }
}

// subtract with carry
static void SBC(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        snes_log_fatal("SBC 8-bit REG_A not impl\n");
        // set_lo_byte(&REG_A, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand) + !FLAG_C;
        const uint16_t result = REG_A - value;

        // NOTE: not sure if V is set correct, copied from my nes
        // might need to invert the result as copied from nes ADC()
        FLAG_V = ((REG_A ^ result) & (value ^ result) & 0x8000) > 0;
        FLAG_C = REG_A >= value;
        set_nz_16(snes, result);

        REG_A = result;
    }

    // snes_log("[SBC] V: %u C: %u N: %u Z: %u REG_A: 0x%02X\n", FLAG_V, FLAG_C, FLAG_N, FLAG_Z, REG_A);
}

// compare accumulator with memory
static void CMP(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = REG_A - value;

        FLAG_C = REG_A >= value;
        set_nz_8(snes, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = REG_A - value;

        FLAG_C = REG_A >= value;
        set_nz_16(snes, result);
    }
}

// compare REG_X with memory
static void CPX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = REG_X - value;

        FLAG_C = REG_X >= value;
        set_nz_8(snes, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = REG_X - value;

        FLAG_C = REG_X >= value;
        set_nz_16(snes, result);
    }
}

// compare REG_Y with memory
static void CPY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = REG_Y - value;

        FLAG_C = REG_Y >= value;
        set_nz_8(snes, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = REG_Y - value;

        FLAG_C = REG_Y >= value;
        set_nz_16(snes, result);
    }
}

// decrement REG_A
static void DEA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = REG_A - 1;
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A--;
        set_nz_16(snes, REG_A);
    }
}

// decrement REG_X
static void DEX(struct SNES_Core* snes)
{
    REG_X--;

    if (FLAG_X)
    {
        REG_X &= 0xFF;
        set_nz_8(snes, REG_X);
    }
    else
    {
        set_nz_16(snes, REG_X);
    }
}

// decrement REG_Y
static void DEY(struct SNES_Core* snes)
{
    REG_Y--;

    if (FLAG_X)
    {
        REG_Y &= 0xFF;
        set_nz_8(snes, REG_Y);
    }
    else
    {
        set_nz_16(snes, REG_Y);
    }
}

// increment REG_A
static void INA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = REG_A + 1;
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A++;
        set_nz_16(snes, REG_A);
    }
}

// increment REG_X
static void INX(struct SNES_Core* snes)
{
    REG_X++;

    if (FLAG_X)
    {
        REG_X &= 0xFF;
        set_nz_8(snes, REG_X);
    }
    else
    {
        set_nz_16(snes, REG_X);
    }
}

// increment REG_Y
static void INY(struct SNES_Core* snes)
{
    REG_Y++;

    if (FLAG_X)
    {
        REG_Y &= 0xFF;
        set_nz_8(snes, REG_Y);
    }
    else
    {
        set_nz_16(snes, REG_Y);
    }
}

// rotate left memory
static void ROL(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = (value << 1) | FLAG_C;
        FLAG_C = is_bit_set(7, result);
        set_nz_8(snes, result);
        snes_cpu_write8(snes, snes->cpu.oprand, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = (value << 1) | FLAG_C;
        FLAG_C = is_bit_set(15, result);
        set_nz_16(snes, result);
        snes_cpu_write16(snes, snes->cpu.oprand, result);
    }
}

// rotate left REG_A
static void ROLA(struct SNES_Core* snes)
{
    const bool old_carry = FLAG_C;
    const uint16_t old_a = REG_A;

    if (FLAG_M)
    {
        const uint8_t result = (REG_A << 1) | old_carry;
        set_lo_byte(&REG_A, result);
        FLAG_C = is_bit_set(7, old_a);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A <<= 1;
        REG_A |= old_carry;
        FLAG_C = is_bit_set(15, old_a);
        set_nz_16(snes, REG_A);
    }
}

// rotate right memory
static void ROR(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = (value >> 1) | (FLAG_C << 7);
        FLAG_C = is_bit_set(1, value);
        set_nz_8(snes, result);
        snes_cpu_write8(snes, snes->cpu.oprand, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = (value >> 1) | (FLAG_C << 15);
        FLAG_C = is_bit_set(1, value);
        set_nz_16(snes, result);
        snes_cpu_write16(snes, snes->cpu.oprand, result);
    }
}

// rotate right REG_A
static void RORA(struct SNES_Core* snes)
{
    const bool old_carry = FLAG_C;
    FLAG_C = is_bit_set(1, REG_A);

    if (FLAG_M)
    {
        const uint8_t result = (REG_A >> 1) | (old_carry << 7);
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = (REG_A >> 1) | (old_carry << 15);
        set_nz_16(snes, REG_A);
    }
}

// branch if carry flag clear
static void BCC(struct SNES_Core* snes)
{
    if (!FLAG_C)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BCC] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if carry flag set
static void BCS(struct SNES_Core* snes)
{
    if (FLAG_C)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BCS] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if zero flag clear
static void BEQ(struct SNES_Core* snes)
{
    if (FLAG_Z)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BEQ] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if negative flag set
static void BMI(struct SNES_Core* snes)
{
    if (FLAG_N)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BMI] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if negative flag clear
static void BPL(struct SNES_Core* snes)
{
    if (!FLAG_N)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BPL] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if zero flag clear
static void BNE(struct SNES_Core* snes)
{
    if (!FLAG_Z)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BNE] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if overflow flag clear
static void BVC(struct SNES_Core* snes)
{
    if (!FLAG_V)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BVC] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch if overflow flag set
static void BVS(struct SNES_Core* snes)
{
    if (FLAG_V)
    {
        REG_PC += (int8_t)snes->cpu.oprand;
        // snes_log("[BVS] REG_PC: 0x%04X\n", REG_PC);
    }
}

// branch always
static void BRA(struct SNES_Core* snes)
{
    REG_PC += (int8_t)snes->cpu.oprand;
}

// push pc to stack then set pc (jmp)
static void JSR(struct SNES_Core* snes)
{
    push16(snes, REG_PC - 1);
    REG_PC = snes->cpu.oprand;
    snes_log("[JSR] jump to 0x%04X\n", REG_PC);
}

// push pc to stack then set pc (jmp)
static void JSL(struct SNES_Core* snes)
{
    push8(snes, REG_PBR);
    push16(snes, REG_PC - 1);
    REG_PC = snes->cpu.oprand;
    REG_PBR = snes->cpu.oprand >> 16;
    snes_log("[JSL] jump to %02X:%04X\n", REG_PBR, REG_PC);
}

// jump long
static void JML(struct SNES_Core* snes)
{
    REG_PBR = snes->cpu.oprand >> 16;
    REG_PC = snes->cpu.oprand & 0xFFFF;
    snes_log("[JML] jump to %02X:%04X\n", REG_PBR, REG_PC);
}

// pull status flags by byte from stack
static void PLP(struct SNES_Core* snes)
{
    const uint8_t value = pop8(snes);
    snes_set_status_flags(snes, value);
}

// pull data bank register from stack
static void PLA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = pop8(snes);
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A = pop16(snes);
        set_nz_16(snes, REG_A);
    }
}

// pull data bank register from stack
static void PLB(struct SNES_Core* snes)
{
    REG_DBR = pop8(snes);
    set_nz_8(snes, REG_DBR);
}

// pull direct page register from stack
static void PLD(struct SNES_Core* snes)
{
    REG_D = pop16(snes);
    set_nz_16(snes, REG_D);
}

// pull REG_X from stack
static void PLX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_X = pop8(snes);
        set_nz_8(snes, REG_X);
    }
    else
    {
        REG_X = pop16(snes);
        set_nz_16(snes, REG_X);
    }
}

// pull REG_Y from stack
static void PLY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        REG_Y = pop8(snes);
        set_nz_8(snes, REG_Y);
    }
    else
    {
        REG_Y = pop16(snes);
        set_nz_16(snes, REG_Y);
    }
}

// push status flags to stack
static void PHP(struct SNES_Core* snes)
{
    const uint8_t value = snes_get_status_flags(snes);
    push8(snes, value);
}

// push accumulator to stack
static void PHA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        push8(snes, REG_A);
    }
    else
    {
        push16(snes, REG_A);
    }
}

// push REG_X to stack
static void PHX(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        push8(snes, REG_X);
    }
    else
    {
        push16(snes, REG_X);
    }
}

// push REG_Y to stack
static void PHY(struct SNES_Core* snes)
{
    if (FLAG_X)
    {
        push8(snes, REG_Y);
    }
    else
    {
        push16(snes, REG_Y);
    }
}

// push direct page register to stack
static void PHD(struct SNES_Core* snes)
{
    push16(snes, REG_D);
}

// push program bank to stack
static void PHK(struct SNES_Core* snes)
{
    push8(snes, REG_PBR);
}

// return from subroutine
static void RTS(struct SNES_Core* snes)
{
    REG_PC = pop16(snes) + 1;
    snes_log("[RTS] REG_PC: 0x%04X op: 0x%02X\n", REG_PC, snes_cpu_read8(snes, REG_PC));
}

// increment memory
static void INC(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = snes_cpu_read8(snes, snes->cpu.oprand) + 1;
        set_nz_8(snes, result);
        snes_cpu_write8(snes, snes->cpu.oprand, result);
    }
    else
    {
        const uint16_t result = snes_cpu_read16(snes, snes->cpu.oprand) + 1;
        set_nz_16(snes, result);
        snes_cpu_write16(snes, snes->cpu.oprand, result);
    }
}

// decrement memory
static void DEC(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = snes_cpu_read8(snes, snes->cpu.oprand) - 1;
        set_nz_8(snes, result);
        snes_cpu_write8(snes, snes->cpu.oprand, result);
    }
    else
    {
        const uint16_t result = snes_cpu_read16(snes, snes->cpu.oprand) - 1;
        set_nz_16(snes, result);
        snes_cpu_write16(snes, snes->cpu.oprand, result);
    }
}

// and accumulator with memory
static void AND(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = REG_A & snes_cpu_read8(snes, snes->cpu.oprand);
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, result);
    }
    else
    {
        REG_A &= snes_cpu_read16(snes, snes->cpu.oprand);
        set_nz_16(snes, REG_A);
    }
}

// or accumulator with memory
static void ORA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = REG_A | snes_cpu_read8(snes, snes->cpu.oprand);
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, result);
    }
    else
    {
        REG_A |= snes_cpu_read16(snes, snes->cpu.oprand);
        set_nz_16(snes, REG_A);
    }
}

// xor accumulator with memory
static void EOR(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t result = REG_A ^ snes_cpu_read8(snes, snes->cpu.oprand);
        set_lo_byte(&REG_A, result);
        set_nz_8(snes, result);
    }
    else
    {
        REG_A ^= snes_cpu_read16(snes, snes->cpu.oprand);
        set_nz_16(snes, REG_A);
    }
}

// arithmetic shift left memory
static void ASL(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = value << 1;
        FLAG_C = is_bit_set(7, value);
        set_nz_8(snes, result);
        snes_cpu_write8(snes, snes->cpu.oprand, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = value << 1;
        FLAG_C = is_bit_set(15, value);
        set_nz_16(snes, result);
        snes_cpu_write16(snes, snes->cpu.oprand, result);
    }
}

// arithmetic shift left accumulator
static void ASLA(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        FLAG_C = is_bit_set(7, REG_A);
        set_lo_byte(&REG_A, REG_A << 1);
        set_nz_8(snes, REG_A);
    }
    else
    {
        FLAG_C = is_bit_set(15, REG_A);
        REG_A <<= 1;
        set_nz_16(snes, REG_A);
    }
}


// logical shift right memory
static void LSR(struct SNES_Core* snes)
{
    if (FLAG_M)
    {
        const uint8_t value = snes_cpu_read8(snes, snes->cpu.oprand);
        const uint8_t result = value >> 1;
        FLAG_C = is_bit_set(1, value);
        set_nz_8(snes, result);
        snes_cpu_write8(snes, snes->cpu.oprand, result);
    }
    else
    {
        const uint16_t value = snes_cpu_read16(snes, snes->cpu.oprand);
        const uint16_t result = value >> 1;
        FLAG_C = is_bit_set(1, value);
        set_nz_16(snes, result);
        snes_cpu_write16(snes, snes->cpu.oprand, result);
    }
}

// logical shift right accumulator
static void LSRA(struct SNES_Core* snes)
{
    FLAG_C = is_bit_set(1, REG_A);

    if (FLAG_M)
    {
        set_lo_byte(&REG_A, REG_A >> 1);
        set_nz_8(snes, REG_A);
    }
    else
    {
        REG_A >>= 1;
        set_nz_16(snes, REG_A);
    }
}

void snes_cpu_run(struct SNES_Core* snes)
{
    breakpoint(snes, 0x01ad10);

    const uint8_t opcode = snes_cpu_read8(snes, addr(REG_PBR, REG_PC++));
    snes->opcode = opcode;

    switch (opcode)
    {
        case 0x01: direct_page_x(snes);     ORA(snes); break;
        case 0x05: direct_page(snes);       ORA(snes); break;
        case 0x06: direct_page(snes);       ASL(snes); break;
        case 0x08: implied(snes);           PHP(snes); break;
        case 0x09: immediateM(snes);        ORA(snes); break;
        case 0x0A: implied(snes);           ASLA(snes); break;
        case 0x0B: implied(snes);           PHD(snes); break;
        case 0x0D: absolute(snes);          ORA(snes); break;
        case 0x0E: absolute(snes);          ASL(snes); break;
        case 0x0F: absolute_long(snes);     ORA(snes); break;
        case 0x10: relative(snes);          BPL(snes); break;
        case 0x15: direct_page_x(snes);     ORA(snes); break;
        case 0x16: direct_page_x(snes);     ASL(snes); break;
        case 0x18: implied(snes);           CLC(snes); break;
        case 0x19: absolute_y(snes);        ORA(snes); break;
        case 0x1A: implied(snes);           INA(snes); break;
        case 0x1B: implied(snes);           TCS(snes); break;
        case 0x1D: absolute_x(snes);        ORA(snes); break;
        case 0x1E: absolute_x(snes);        ASL(snes); break;
        case 0x20: absolute(snes);          JSR(snes); break;
        // case 0x21: direct_page_x(snes);     AND(snes); break;
        case 0x22: absolute_long(snes);     JSL(snes); break;
        case 0x25: direct_page(snes);       AND(snes); break;
        case 0x26: direct_page(snes);       ROL(snes); break;
        case 0x28: implied(snes);           PLP(snes); break;
        case 0x29: immediateM(snes);        AND(snes); break;
        case 0x2A: implied(snes);           ROLA(snes); break;
        case 0x2B: implied(snes);           PLD(snes); break;
        case 0x2D: absolute(snes);          AND(snes); break;
        case 0x2E: absolute(snes);          ROL(snes); break;
        case 0x2F: absolute_long(snes);     AND(snes); break;
        case 0x30: relative(snes);          BMI(snes); break;
        case 0x35: direct_page_x(snes);     AND(snes); break;
        case 0x36: direct_page_x(snes);     ROL(snes); break;
        case 0x38: implied(snes);           SEC(snes); break;
        case 0x39: absolute_y(snes);        AND(snes); break;
        case 0x3A: implied(snes);           DEA(snes); break;
        case 0x3B: implied(snes);           TSC(snes); break;
        case 0x3D: absolute_x(snes);        AND(snes); break;
        case 0x3E: absolute_x(snes);        ROL(snes); break;
        // case 0x41: direct_page_x(snes);     EOR(snes); break;
        case 0x45: direct_page(snes);       EOR(snes); break;
        case 0x46: direct_page(snes);       LSR(snes); break;
        case 0x48: implied(snes);           PHA(snes); break;
        case 0x49: immediateM(snes);        EOR(snes); break;
        case 0x4A: implied(snes);           LSRA(snes); break;
        case 0x4B: implied(snes);           PHK(snes); break;
        case 0x4D: absolute(snes);          EOR(snes); break;
        case 0x4E: direct_page_x(snes);     LSR(snes); break;
        case 0x4F: absolute_long(snes);     EOR(snes); break;
        case 0x50: relative(snes);          BVC(snes); break;
        case 0x55: direct_page_x(snes);     EOR(snes); break;
        case 0x58: implied(snes);           CLI(snes); break;
        case 0x59: absolute_y(snes);        EOR(snes); break;
        case 0x5A: implied(snes);           PHY(snes); break;
        case 0x5B: implied(snes);           TCD(snes); break;
        case 0x5C: absolute_long(snes);     JML(snes); break;
        case 0x5D: absolute_x(snes);        EOR(snes); break;
        case 0x5E: absolute_x(snes);        LSR(snes); break;
        case 0x60: implied(snes);           RTS(snes); break;
        case 0x64: direct_page(snes);       STZ(snes); break;
        case 0x65: direct_page(snes);       ADC(snes); break;
        case 0x66: direct_page(snes);       ROR(snes); break;
        case 0x68: implied(snes);           PLA(snes); break;
        case 0x69: immediateM(snes);        ADC(snes); break;
        case 0x6A: implied(snes);           RORA(snes); break;
        case 0x6D: absolute(snes);          ADC(snes); break;
        case 0x6E: absolute(snes);          ROR(snes); break;
        case 0x6F: absolute_long(snes);     ADC(snes); break;
        case 0x70: relative(snes);          BVS(snes); break;
        case 0x74: direct_page_x(snes);     STZ(snes); break;
        case 0x76: direct_page_x(snes);     ROR(snes); break;
        case 0x77: dp_ind_long_y(snes);     ADC(snes); break;
        case 0x78: implied(snes);           SEI(snes); break;
        case 0x7A: implied(snes);           PLY(snes); break;
        case 0x7B: implied(snes);           TDC(snes); break;
        case 0x7E: absolute_x(snes);        ROR(snes); break;
        case 0x80: relative(snes);          BRA(snes); break;
        // case 0x81: direct_page_x(snes);     STA(snes); break;
        case 0x84: direct_page(snes);       STY(snes); break;
        case 0x85: direct_page(snes);       STA(snes); break;
        case 0x86: direct_page(snes);       STX(snes); break;
        // case 0x87: direct_page_long(snes);  STA(snes); break;
        case 0x88: implied(snes);           DEY(snes); break;
        case 0x8A: implied(snes);           TXA(snes); break;
        case 0x8C: absolute(snes);          STY(snes); break;
        case 0x8D: absolute(snes);          STA(snes); break;
        case 0x8E: absolute(snes);          STX(snes); break;
        case 0x8F: absolute_long(snes);     STA(snes); break;
        case 0x90: relative(snes);          BCC(snes); break;
        case 0x94: direct_page_x(snes);     STY(snes); break;
        case 0x95: direct_page_x(snes);     STA(snes); break;
        case 0x96: direct_page_y(snes);     STX(snes); break;
        case 0x98: implied(snes);           TYA(snes); break;
        case 0x99: absolute_y(snes);        STA(snes); break;
        case 0x9A: implied(snes);           TXS(snes); break;
        case 0x9B: implied(snes);           TXY(snes); break;
        case 0x9C: absolute(snes);          STZ(snes); break;
        case 0x9D: absolute_x(snes);        STA(snes); break;
        case 0x9E: absolute_x(snes);        STZ(snes); break;
        case 0x9F: absolute_long_x(snes);   STA(snes); break;
        case 0xA0: immediateX(snes);        LDY(snes); break;
        case 0xA2: immediateX(snes);        LDX(snes); break;
        case 0xA4: direct_page(snes);       LDY(snes); break;
        case 0xA5: direct_page(snes);       LDA(snes); break;
        case 0xA6: direct_page(snes);       LDX(snes); break;
        case 0xA8: implied(snes);           TAY(snes); break;
        case 0xA9: immediateM(snes);        LDA(snes); break;
        case 0xAA: implied(snes);           TAX(snes); break;
        case 0xAB: implied(snes);           PLB(snes); break;
        case 0xAC: absolute(snes);          LDY(snes); break;
        case 0xAD: absolute(snes);          LDA(snes); break;
        case 0xAE: absolute(snes);          LDX(snes); break;
        case 0xAF: absolute_long(snes);     LDA(snes); break;
        case 0xB0: relative(snes);          BCS(snes); break;
        case 0xB4: direct_page_x(snes);     LDY(snes); break;
        case 0xB5: direct_page_x(snes);     LDA(snes); break;
        case 0xB6: direct_page_y(snes);     LDX(snes); break;
        case 0xB7: dp_ind_long_y(snes);     LDA(snes); break;
        case 0xB8: implied(snes);           CLV(snes); break;
        case 0xB9: absolute_y(snes);        LDA(snes); break;
        case 0xBB: implied(snes);           TYX(snes); break;
        case 0xBC: absolute_x(snes);        LDY(snes); break;
        case 0xBD: absolute_x(snes);        LDA(snes); break;
        case 0xBE: absolute_y(snes);        LDX(snes); break;
        case 0xC0: immediateX(snes);        CPY(snes); break;
        case 0xC2: immediate8(snes);        REP(snes); break;
        case 0xC4: direct_page(snes);       CPY(snes); break;
        case 0xC6: direct_page(snes);       DEC(snes); break;
        case 0xC8: implied(snes);           INY(snes); break;
        case 0xCA: implied(snes);           DEX(snes); break;
        case 0xCC: absolute(snes);          CPY(snes); break;
        case 0xCD: absolute(snes);          CMP(snes); break;
        case 0xCE: absolute(snes);          DEC(snes); break;
        case 0xD0: relative(snes);          BNE(snes); break;
        case 0xD6: direct_page_x(snes);     DEC(snes); break;
        case 0xD8: implied(snes);           CLD(snes); break;
        case 0xDA: implied(snes);           PHX(snes); break;
        case 0xDC: absolute_indirect_long(snes);    JML(snes); break;
        case 0xDE: absolute_x(snes);        DEC(snes); break;
        case 0xE0: immediateX(snes);        CPX(snes); break;
        case 0xE2: immediate8(snes);        SEP(snes); break;
        case 0xE4: direct_page(snes);       CPX(snes); break;
        case 0xE6: direct_page(snes);       INC(snes); break;
        case 0xE8: implied(snes);           INX(snes); break;
        case 0xE9: immediateM(snes);        SBC(snes); break;
        case 0xEB: implied(snes);           XBA(snes); break;
        case 0xEC: absolute(snes);          CPX(snes); break;
        case 0xEE: absolute(snes);          INC(snes); break;
        case 0xF0: relative(snes);          BEQ(snes); break;
        case 0xF6: direct_page_x(snes);     INC(snes); break;
        case 0xF8: implied(snes);           SED(snes); break;
        case 0xFA: implied(snes);           PLX(snes); break;
        case 0xFB: implied(snes);           XCE(snes); break;
        case 0xFE: absolute_x(snes);        INC(snes); break;

        default:
            snes_log_fatal("UNK opcode: 0x%02X REG_PC: 0x%04X REG_PBR: 0x%02X REG_A: 0x%04X S: 0x%04X X: 0x%04X Y: 0x%04X P: 0x%02X ticks: %zu\n", opcode, REG_PC, REG_PBR, REG_A, REG_SP, REG_X, REG_Y, snes_get_status_flags(snes), snes->ticks);
            break;
    }

    snes->ticks++;
}
