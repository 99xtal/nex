#include <getopt.h>
#include <lib6502/6502.h>
#include <nex/nes.h>
#include <stdio.h>

#include "commands.h"

void usage_run(const char* prog) {
  fprintf(stderr,
          "nex %s [options...] [ROM]\n"
          "  -t     Enable tracing logs\n"
          "  -h     Show usage\n"
          "\n",
          prog);
}

int cmd_run(int argc, char** argv) {
  int opt;
  int tracing = 0;

  while ((opt = getopt(argc, argv, "ht")) != -1) {
    switch (opt) {
      case 't':
        tracing = 1;
        break;
      case 'h':
        usage_run(argv[0]);
        return EXIT_SUCCESS;
      default:
        usage_run(argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (optind >= argc) {
    usage_run(argv[0]);
    fprintf(stderr, "\nMissing ROM file\n");
    return EXIT_FAILURE;
  }

  const char* rom_path = argv[optind];

  NES* nes = nex_create();
  if (!nes) {
    fprintf(stderr, "Failed to initialize NES");
    return EXIT_FAILURE;
  }

  if (nex_load_rom(nes, rom_path) != 0) {
    fprintf(stderr, "Invalid ROM file");
    return EXIT_FAILURE;
  }
  // if (tracing) {
  //     nes.cpu.trace = print_trace;
  //     nes.cpu.trace_ctx = &nes;
  // }

  nex_reset(nes);

  while (1) {
    nex_step(nes);
  }

  return EXIT_SUCCESS;
}