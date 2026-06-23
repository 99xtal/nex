#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

void print_trace(void *trace_ctx, cpu6502_trace trace) {
    nes *n = trace_ctx;

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
        "%04X  %-9s %-3s %-27s A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu\n",
        trace.PC,
        byte_str,
        trace.mnemonic,
        trace.operand,
        trace.A,
        trace.X,
        trace.Y,
        trace.status,
        trace.SP,
        n->ppu.scanline,
        n->ppu.dot,
        n->total_cpu_cycles
    );
}

int main(int argc, char *argv[]) {
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

    // SDL initialization
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error: failed to initialize SDL");
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow(
        "nex",
        640,
        480,
        0
    );
    if (!window) {
        fprintf(stderr, "Error: window creation failed", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

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
        nes.cpu.trace_ctx = &nes;
    }

    nes_reset(&nes);

    // program loop
    int running = 1;

    while (running) {
        // poll events
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = 0;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        running = 0;
                    }
                    break;
            }
        }
        
        nes_step(&nes);
    }

    cartridge_free(&cart);
    return EXIT_SUCCESS;
}