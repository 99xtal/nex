#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "cartridge.h"
#include "commands.h"
#include "nes.h"

/**
 * "run"
 */
void usage_run(const char *prog) {
    fprintf(stderr,
        "nex %s [options...] [ROM]\n"
        "  -t     Enable tracing logs\n"
        "  -h     Show usage\n"
        "\n",
        prog
    );
}

void print_trace(void *trace_ctx, cpu6502_trace trace);
void render_pixel(void *render_ctx, int x, int y, uint8_t color_index);

int cmd_run(int argc, char **argv) {
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

    const char *rom_path = argv[optind];

    cartridge cart = {0};
    if (cartridge_load(&cart, rom_path) != 0) {
        fprintf(stderr, "Invalid ROM file");
        return EXIT_FAILURE;
    }

    uint8_t framebuffer[256 * 240];

    nes nes = {0};
    if ((nes_init(&nes, &cart, render_pixel, framebuffer)) != 0) {
        cartridge_free(&cart);
        fprintf(stderr, "Failed to initialize NES");
        return EXIT_FAILURE;
    }
    if (tracing) {
        nes.cpu.trace = print_trace;
        nes.cpu.trace_ctx = &nes;
    }

    nes_reset(&nes);

    while (1) {
        nes_step(&nes);
    }

    cartridge_free(&cart);
    return EXIT_SUCCESS;
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

void render_pixel(void *render_ctx, int x, int y, uint8_t color_index) {
    uint8_t *framebuffer = render_ctx;

    framebuffer[(y * SCREEN_WIDTH) + x] = color_index;
}

/**
 * "test"
 */
void usage_test(const char *prog) {
    fprintf(stderr,
        "nex %s [options...] [ROM]\n"
        "Environment:\n"
        "  --start-pc ADDR          Set CPU program counter to ADDR\n"
        "\n"
        "Pass conditions:\n"
        "\n"
        "Failure conditions:\n"
        "\n"
        "Timeouts:\n"
        "  --timeout-cycles N       Fail after N CPU cycles\n"
        "\n",
        prog
    );
}

enum {
    OPT_START_PC,
    OPT_TIMEOUT_CYCLES,
};

uint16_t parse_addr(const char *str);
uint64_t parse_uint64(const char *str);

static struct option test_options[] = {
    { "start-pc",       required_argument, NULL, OPT_START_PC },
    { "timeout-cycles", required_argument, NULL, OPT_START_PC },
};

typedef struct TestConfig {
    uint16_t start_pc;
    uint64_t timeout_cycles;
} TestConfig;

int cmd_test(int argc, char **argv) {
    int opt;
    TestConfig config;
    config.timeout_cycles = 1000000;

    while ((opt = getopt_long(argc, argv, "", test_options, NULL)) != -1) {
        switch (opt) {
            case OPT_START_PC: {
                config.start_pc = parse_addr(optarg);
                break;
            }
            case OPT_TIMEOUT_CYCLES: {
                config.timeout_cycles = parse_uint64(optarg);
            }
            default:
                break;
        }
    }

    if (optind >= argc) {
        usage_test(argv[0]);
        fprintf(stderr, "\nMissing ROM file\n");
        return EXIT_FAILURE;
    }

    // validate params

    // init emulator
    const char *rom_path = argv[optind];

    cartridge cart = {0};
    if (cartridge_load(&cart, rom_path) != 0) {
        fprintf(stderr, "Invalid ROM file");
        return EXIT_FAILURE;
    }

    uint8_t framebuffer[256 * 240];

    nes nes = {0};
    if ((nes_init(&nes, &cart, render_pixel, framebuffer)) != 0) {
        cartridge_free(&cart);
        fprintf(stderr, "Failed to initialize NES");
        return EXIT_FAILURE;
    }

    nes_reset(&nes);

    if (config.start_pc) {
        nes.cpu.PC = config.start_pc;
    }

    while (nes.total_cpu_cycles < config.timeout_cycles) {
        nes_step(&nes);
    }

    printf("FAIL: Timeout after %llu cycles\n", nes.total_cpu_cycles);

    cartridge_free(&cart);
    return EXIT_SUCCESS;
}

uint16_t parse_addr(const char *str) {
    char *end;
    unsigned long value = strtoul(str, &end, 16);

    if (*end != '\0') {
        fprintf(stderr, "invalid address: %s\n", str);
        exit(EXIT_FAILURE);
    }

    if (value > 0xFFFF) {
        fprintf(stderr, "address out of range: %s\n", str);
        exit(EXIT_FAILURE);
    }

    return (uint16_t)value;
}

uint64_t parse_uint64(const char *str) {
    char *end;

    int value = strtoul(str, &end, 10);

    if (*end != '\0') {
        fprintf(stderr, "invalid number: %s\n", str);
        exit(EXIT_FAILURE);
    }

    return (uint64_t)value;
}