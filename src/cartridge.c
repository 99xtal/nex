#include <stdio.h>
#include <string.h>
#include "cartridge.h"

typedef enum {
    FILE_INES_V1,
    FILE_INES_V2,
    FILE_UNKNOWN,
} RomFileFormat;

int cartridge_load_ines_v1(cartridge *c, uint8_t *header, FILE *f);
RomFileFormat get_file_format(uint8_t *header);

int cartridge_load(cartridge *c, const char *path) {
    if (!c || !path) {
        return -1;
    }

    memset(c, 0, sizeof(*c));

    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }

    uint8_t header[16];
    if (fread(header, 1, 16, f) != 16) {
        fclose(f);
        return -1;
    }

    RomFileFormat file_format = get_file_format(header);

    switch (file_format) {
        case FILE_INES_V1:
            if (cartridge_load_ines_v1(c, header, f) != 0) {
                fclose(f);
                return -1;
            }
            break;
        case FILE_INES_V2:
        case FILE_UNKNOWN:
        default: {
            fclose(f);
            return -1;
        }
    }

    return 0;
}

int cartridge_load_ines_v1(cartridge *c, uint8_t *header, FILE *f) {
    c->prg_rom_size = header[4] * 16 * 1024;
    c->chr_rom_size = header[5] * 8 * 1024;
    c->mapper_num = (header[6] >> 4) | (header[7] & 0xF0);
    c->nt_mirroring = (header[6] & 0x01) == 0 ? NT_MIRROR_HORIZONTAL : NT_MIRROR_VERTICAL;
    c->has_battery_prg_ram = (header[6] & 0x02) != 0;
    c->has_trainer = (header[6] & 0x04) != 0;

    if (c->has_trainer) {
        if (fseek(f, 512, SEEK_CUR) != 0) {
            fclose(f);
            return -1;
        }
    }

    c->prg_rom = malloc(c->prg_rom_size);
    if (!c->prg_rom) {
        fclose(f);
        return -1;
    }

    if (fread(c->prg_rom, 1, c->prg_rom_size, f) != c->prg_rom_size) {
        fclose(f);
        return -1;
    }

    if (c->chr_rom_size > 0) {
        c->uses_chr_ram = 0;
        c->chr_rom = malloc(c->chr_rom_size);
        if (!c->chr_rom) {
            fclose(f);
            return -1;
        }

        if (fread(c->chr_rom, 1, c->chr_rom_size, f) != c->chr_rom_size) {
            fclose(f);
            return -1;
        }
    } else {
        c->uses_chr_ram = 1;
        c->chr_rom_size = 8 * 1024;
        c->chr_rom = calloc(1, c->chr_rom_size);
    }

    // initialize mapper struct
    switch (c->mapper_num) {
        case 0: {
            mapper_nrom_init(&c->mapper, c);
            break;
        }
        default: {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

void cartridge_free(cartridge *c) {
    if (!c) {
        return;
    }

    free(c->prg_rom);
    free(c->chr_rom);
    memset(c, 0, sizeof(*c));
}

RomFileFormat get_file_format(uint8_t *header) {
    bool is_ines_v1 = memcmp(header, "NES\x1a", 4) == 0;

    if (is_ines_v1) {
        if ((header[7] & 0x0C) == 0x08) {
            return FILE_INES_V2;
        } else {
            return FILE_INES_V1;
        }
    }

    return FILE_UNKNOWN;
}

