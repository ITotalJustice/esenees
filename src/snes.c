#include "snes.h"
#include "types.h"
#include "internal.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>


static struct SNES_Header snes_get_header(const uint8_t* rom, size_t size)
{
    // todo: figure out how to detect this...
    enum
    {
        LoROM_OFFSET = 0x7FB0,
        HiROM_OFFSET = 0xFFB0,
    };

    struct SNES_Header header;
    memcpy(&header, rom + LoROM_OFFSET, sizeof(struct SNES_Header));
    return header;
}

static struct SNES_GameTitle snes_get_game_title(const struct SNES_Header* header)
{
    struct SNES_GameTitle title = {0};
    memcpy(title.title, header->game_title_registration, sizeof(header->game_title_registration));
    return title;
}

bool snes_init(struct SNES_Core* snes)
{
    memset(snes, 0, sizeof(struct SNES_Core));
    return true;
}

bool snes_loadrom(struct SNES_Core* snes, const uint8_t* rom, size_t rom_size)
{
    // todo: validate rom
    snes->rom = rom;
    snes->rom_size = rom_size;

    const struct SNES_Header header = snes_get_header(rom, rom_size);
    const struct SNES_GameTitle title = snes_get_game_title(&header);

    // todo: fix this, getting the wrong size...
    // metroid is reported as 4096 KiB, but file size is 3 MiB
    // however it works for SMW ???
    snes->cart.rom_size = 1 << header.rom_size;
    snes->cart.ram_size = 1 << header.ram_size;

    snes_log("SNES header:\n");
    snes_log("\ttitle: %s\n", title.title);
    snes_log("\tmap_mode: 0x%02X\n", header.map_mode);
    snes_log("\tcartridge_type: 0x%02X\n", header.cartridge_type);
    snes_log("\trom_size: 0x%02X [%zu KiB]\n", header.rom_size, snes->cart.rom_size);
    snes_log("\tram_size: 0x%02X [%zu KiB]\n", header.ram_size, snes->cart.ram_size);

    snes_cpu_init(snes);
    snes_ppu_init(snes);
    snes_apu_init(snes);

    return true;
}

bool snes_run(struct SNES_Core* snes)
{
    for (;;)
    {
        snes_cpu_run(snes);
    }

    return true;
}
