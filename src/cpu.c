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
#define FLAG_D snes->cpu.flag_D
#define FLAG_I snes->cpu.flag_I


bool snes_cpu_init(struct SNES_Core* snes)
{
    // todo: setup default values of registers!
    memset(&snes->cpu, 0, sizeof(snes->cpu));
    REG_PC = 0x8000; // im guessing this
    REG_SP = 0x01FF;
    return true;
}

// addressing modes
static void implied(struct SNES_Core* snes)
{
    (void)snes;
}

static void absolute(struct SNES_Core* snes)
{
    // todo:
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
    // todo:
}

void snes_cpu_run(struct SNES_Core* snes)
{
    const uint8_t opcode = snes_cpu_read8(snes, REG_PBR, REG_PC++);

    switch (opcode)
    {
        case 0x38: implied(snes); SEC(snes); break;
        case 0x78: implied(snes); SEI(snes); break;
        case 0xF8: implied(snes); SED(snes); break;

        default:
            snes_log_fatal("UNK opcode: 0x%02X REG_PC: 0x%04X REG_PBR: 0x%02X\n", opcode, REG_PC, REG_PBR);
            break;
    }
}
