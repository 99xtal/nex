#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cartridge.h"

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

    printf("ROM: %s\n", rom_path);

    cartridge cart = {0};
    if (load_cartridge(&cart, rom_path) != 0) {
        fprintf(stderr, "invalid ROM file");
        return EXIT_FAILURE;
    }

    uint8_t test = cart.mapper.cpu_read(&cart.mapper, 0x8002);

    int x;

    free_cartridge(&cart);
    return EXIT_SUCCESS;
}