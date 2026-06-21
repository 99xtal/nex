#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cartridge.h"
#include "nes.h"

void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] ROM\n"
        "\n"
        "OPTIONS:\n"
        "  -h       Show this help message\n",
        prog
    );
}

int main(int argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: missing ROM file\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *rom_path = argv[optind];

    cartridge cart = {0};
    if (cartridge_load(&cart, rom_path) != 0) {
        fprintf(stderr, "invalid ROM file");
        return EXIT_FAILURE;
    }

    nes nes = {0};
    if ((nes_init(&nes, &cart)) != 0) {
        cartridge_free(&cart);
        fprintf(stderr, "failed to initialize NES");
        return EXIT_FAILURE;
    }

    nes_reset(&nes);

    while (1) {
        nes_step(&nes);
    }

    cartridge_free(&cart);
    return EXIT_SUCCESS;
}