#include <snes.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

static struct SNES_Core snes = {0};
static uint8_t rom[0x80000] = {0};
static size_t rom_size = 0;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("missing args\n");
        return -1;
    }

    FILE* file = fopen(argv[1], "rb");
    rom_size = fread(rom, 1, sizeof(rom), file);
    printf("rom size: %zu\n", rom_size);
    fclose(file);

    snes_init(&snes);
    snes_loadrom(&snes, rom, rom_size);
    snes_run(&snes);

    return 0;
}
