#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mapper.h"

typedef enum {
    NT_MIRROR_HORIZONTAL = 0,
    NT_MIRROR_VERTICAL = 1
} NametableMirroring;

typedef struct cartridge {
    uint8_t *prg_rom;
    size_t prg_rom_size;

    uint8_t *chr_rom;
    size_t chr_rom_size;

    uint8_t mapper_num;
    mapper mapper;

    NametableMirroring nt_mirroring;
    bool has_battery_prg_ram;
    bool has_trainer;
    bool uses_chr_ram;
} cartridge;

int cartridge_load(cartridge *c, const char *path);

void cartridge_free(cartridge *c);

#endif // CARTRIDGE_H