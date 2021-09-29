#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// SOURCE: https://sneslab.net/wiki/SNES_ROM_Header#CPU_Exception_Vectors
enum SNES_Vector
{
    SNES_Vector_COP = 0xFFE4, // 4-5
    SNES_Vector_BRK = 0xFFE6, // 6-7
    SNES_Vector_ABORT = 0xFFE8, // 8-9
    SNES_Vector_NMI = 0xFFEA, // A-B
    SNES_Vector_IRQ = 0xFFEE, // E-F
};

enum SNES_MapMode
{
    SNES_MapMode_LoROM = 0x20, // 32K banks
    SNES_MapMode_HiROM = 0x21, // 64K banks
    // theres more but will only support above for now
};

enum SNES_CartridgeType
{
    SNES_CartridgeType_ROM = 0x00,
    SNES_CartridgeType_ROM_RAM = 0x01,
    SNES_CartridgeType_ROM_RAM_BATTERY = 0x02,
    // the rest involves coprocessor, won't be supported for now
};

// SOURCE: https://problemkaputt.de/fullsnes.htm#snescartridgeromheader
enum SNES_DestinationCode
{
    SNES_DestinationCode_JAPAN = 0x00,
    SNES_DestinationCode_USA = 0x01,
    SNES_DestinationCode_EUROPE = 0x02, // PAL 50fps
    SNES_DestinationCode_SWEDEN = 0x03,
    SNES_DestinationCode_FINLAND = 0x04,
    SNES_DestinationCode_DENMARK = 0x05,
    SNES_DestinationCode_FRANCE = 0x06,
    SNES_DestinationCode_HOLLAND = 0x07,
    SNES_DestinationCode_SPAIN = 0x08,
    SNES_DestinationCode_GERMANY = 0x09,
    SNES_DestinationCode_ITALY = 0x0A,
    SNES_DestinationCode_CHINA = 0x0B,
    SNES_DestinationCode_INDONESIA = 0x0C,
    SNES_DestinationCode_SOUTH_KOREA = 0x0D,
    SNES_DestinationCode_COMMON = 0x0E,
    SNES_DestinationCode_CANADA = 0x0F,
    SNES_DestinationCode_BRAZIL = 0x10,
    SNES_DestinationCode_AUSTRALIA = 0x11,
};

// SOURCE: https://sneslab.net/wiki/SNES_ROM_Header#ROM_Registration_Data
struct SNES_Header
{
    uint8_t maker_code[2];
    uint8_t game_code[4];
    uint8_t fixed_value_1[7]; // should be 0x00
    uint8_t expansion_ram_size; // should be 0x00
    uint8_t special_version; // should be 0x00
    uint8_t cartridge_type_sub_number;
    uint8_t game_title_registration[21];
    uint8_t map_mode;
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t fixed_value_2; // should be 0x33
    uint8_t mask_rom_version;
    uint8_t complement_check[2];
    uint8_t check_sum[2];
};

struct SNES_GameTitle
{
    char title[22]; // NULL terminated
};

struct SNES_NMITIMEN
{
    bool vblank_enable;
    uint8_t irq; // not sure on this
    bool joypad_enable;
};

struct SNES_INIDISP
{
    bool forced_blanking; // 0=normal, 1=screen black
    uint8_t master_brightness; // 0=screen black 1=dim etc
};

struct SNES_Cart
{
    size_t rom_size;
    size_t ram_size;

    uint8_t map_mode;
    uint8_t cart_type;
};

struct SNES_Cpu
{
    uint32_t oprand;
    uint16_t cycles;

    uint16_t PC;
    uint16_t SP;
    uint16_t X;
    uint16_t Y;
    uint16_t A;
    uint16_t D;
    uint8_t PBR; // program bank register (bit 16-24 of addr)
    uint8_t DBR; // data bank register (bit 16-24 of addr)

    // flags
    bool flag_B; // break (emulation mode only)
    bool flag_N; // negative
    bool flag_V; // overflow
    bool flag_M; // accumulator width
    bool flag_X; // x/y width
    bool flag_D; // decimal
    bool flag_I; // irq disable
    bool flag_Z; // zero
    bool flag_C; // carry
    bool flag_E; // emulation
};

struct SNES_Ppu
{
    uint16_t vram[1024 * 32];
    uint16_t vram_addr; // todo: mask 0x7FFF
    uint8_t vram_addr_step; // used as an index into step array
    bool vram_addr_increment_mode; // 0=lo, 1=hi

    uint16_t cgram[256];
    uint8_t cgram_addr;
    uint8_t cgram_cached_byte;
    bool cgram_flipflop;

    // each slot is 4 bytes, also takes 2-bits at the end of oam
    uint8_t oam[544];
    uint16_t oam_addr;

    uint8_t obj_size;
    // name select
    // name base select
    bool obj_priority_actiavtion;

};

struct SNES_Apu
{
    // cpu register set
    uint16_t oprand;
    uint16_t PC;
    uint8_t SP;
    uint8_t A;
    uint8_t X;
    uint8_t Y;

    // flags (PSW)
    bool flag_N; // negative
    bool flag_V; // overflow
    bool flag_P; // direct page
    bool flag_B; // break (unused - apart from BRK)
    bool flag_H; // half carry
    bool flag_I; // interrupt enabled (unused)
    bool flag_Z; // zero
    bool flag_C; // carry

    // registers
    uint8_t unk; // [W] undocumented
    uint8_t dsp_addr; // [R/W]
    uint8_t dsp_data; // [R/W]
    uint8_t rm0; // [R/W]
    uint8_t rm1; // [R/W]
    uint8_t port_in[4]; // [W]
    uint8_t port_out[4]; // [R]
    uint8_t timer[3]; // [W]
    uint8_t counter[3]; // [R]
    bool timer_enable[3];
    bool ipl_enable; // enabled reading from ipl at 0xFFC0+

    // 64KiB ram
    uint8_t ram[1024 * 64];
};

struct SNES_Mem
{
    uint8_t wram[1024 * 128]; // 128KiB

    struct SNES_NMITIMEN NMITIMEN;
    struct SNES_INIDISP INIDISP;
    uint8_t HDMAEN; // hdma channel(s) enable
    uint8_t MDMAEN; // general dma channel(s)
    uint8_t MEMSEL; // memory-2 waitstate control

    // last value placed on the data bus
    uint8_t open_bus;
};

struct SNES_Core
{
    struct SNES_Cpu cpu;
    struct SNES_Ppu ppu;
    struct SNES_Apu apu;
    struct SNES_Mem mem;
    struct SNES_Cart cart;

    const uint8_t* rom;
    size_t rom_size;

    // for testing
    size_t ticks;
    uint8_t opcode;
};

#ifdef __cplusplus
}
#endif
