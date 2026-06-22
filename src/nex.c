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
        "  -t       Enable tracing\n"
        "  -h       Show this help message\n",
        prog
    );
}

void print_trace(void *trace_ctx __attribute__((unused)), cpu6502_trace trace) {
  char byte_str[10] = {0};
  int pos = 0;

  for (size_t i = 0; i < trace.bytes_count; i++) {
    pos += snprintf(
      byte_str + pos,
      sizeof(byte_str) - pos,
      "%02X ",
      trace.bytes[i]
    );
  }

  printf(
    "%04X  %-9s %-3s %-27s A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n",
    trace.PC,
    byte_str,
    trace.mnemonic,
    trace.operand,
    trace.A,
    trace.X,
    trace.Y,
    trace.status,
    trace.SP,
    trace.cycles
  );
}

int main(int argc, char **argv) {
    int opt;
    int tracing = 0;

    while ((opt = getopt(argc, argv, "ht")) != -1) {
        switch (opt) {
            case 't':
                tracing = 1;
                break;
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
    if (tracing) {
        nes.cpu.trace = print_trace;
    }

    nes_reset(&nes);

    while (1) {
        nes_step(&nes);
    }

    cartridge_free(&cart);
    return EXIT_SUCCESS;
}