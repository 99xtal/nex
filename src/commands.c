#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "cartridge.h"
#include "commands.h"
#include "nes.h"
#include "test_config.h"

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

    char mnemonic[5];

    if (trace.is_undocumented_inst)
        snprintf(mnemonic, sizeof mnemonic, "*%s", trace.mnemonic);
    else
        snprintf(mnemonic, sizeof mnemonic, " %s", trace.mnemonic);

    printf(
        "%04X  %-9s%-4s %-27s A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu\n",
        trace.PC,
        byte_str,
        mnemonic,
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

#define RED_TEXT "\033[1;31m"
#define GREEN_TEXT "\033[1;32m"
#define RESET_TEXT "\033[0m"

void usage_test(const char *prog) {
    fprintf(stderr,
        "nex %s [options...] [ROM]\n"
        "Environment:\n"
        "  --start-pc ADDR          Set initial CPU program counter\n"
        "\n"
        "Test control:\n"
        "  --stop-pc ADDR           Stop when CPU reaches ADDR\n"
        "  --assert-mem             Assert memory when stopped, e.g. 0002=00\n"
        "  --timeout-cycles N       Fail after N CPU cycles\n"
        "\n"
        "Other options:\n"
        "  --trace                  Enable instruction tracing\n"
        "\n",
        prog
    );
}

enum {
    OPT_START_PC,
    OPT_STOP_PC,
    OPT_ASSERT_MEM,
    OPT_TIMEOUT_CYCLES,
    OPT_TRACE,
};

void evaluate_assertions(nes *nes, TestConfig *config);
int parse_addr(const char *str, uint16_t *addr);
int parse_uint64(const char *str, uint64_t *value);
int parse_assert_mem_exp(const char *str, uint16_t *addr, uint8_t *value);

static struct option test_options[] = {
    { "start-pc",       required_argument, NULL, OPT_START_PC },
    { "stop-pc",        required_argument, NULL, OPT_STOP_PC },
    { "assert-mem",     required_argument, NULL, OPT_ASSERT_MEM },
    { "timeout-cycles", required_argument, NULL, OPT_START_PC },
    { "trace",          no_argument,       NULL, OPT_TRACE },
};

int cmd_test(int argc, char **argv) {
    int opt;
    bool tracing = false;
    TestConfig config;
    test_config_init(&config);

    while ((opt = getopt_long(argc, argv, "", test_options, NULL)) != -1) {
        switch (opt) {
            case OPT_START_PC: {
                parse_addr(optarg, &config.start_pc);
                break;
            }
            case OPT_STOP_PC: {
                parse_addr(optarg, &config.stop_pc);
                break;
            }
            case OPT_ASSERT_MEM: {
                uint16_t addr;
                uint8_t value;

                parse_assert_mem_exp(optarg, &addr, &value);

                Assertion assert_mem = {
                    .type = ASSERT_MEM_EQ,
                    .mem = {
                        .addr = addr,
                        .value = value,
                    }
                };

                if (test_config_add_assertion(&config, assert_mem) != 0) {
                    fprintf(stderr, "Failed to add assertion");
                    return EXIT_FAILURE;
                };
                break;
            }
            case OPT_TIMEOUT_CYCLES: {
                parse_uint64(optarg, &config.timeout_cycles);
                break;
            }
            case OPT_TRACE: {
                tracing = true;
                break;
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
    if (tracing) {
        nes.cpu.trace = print_trace;
        nes.cpu.trace_ctx = &nes;
    }

    nes_reset(&nes);

    if (config.start_pc) {
        nes.cpu.PC = config.start_pc;
    }

    while (nes.total_cpu_cycles < config.timeout_cycles) {
        nes_step(&nes);

        if (nes.cpu.PC == config.stop_pc) {
            nes_step(&nes);
            evaluate_assertions(&nes, &config);
            return EXIT_SUCCESS;
        }
    }

    printf(RED_TEXT "FAIL" RESET_TEXT ": Timeout after %llu cycles\n", nes.total_cpu_cycles);

    cartridge_free(&cart);
    test_config_free(&config);
    return EXIT_SUCCESS;
}

void evaluate_assertions(nes *nes, TestConfig *config) {
    for (size_t i = 0; i < config->assertion_cnt; i++) {
        Assertion assert = config->assertions[i];

        switch (assert.type) {
            case ASSERT_MEM_EQ: {
                if (nes->wram[assert.mem.addr] != assert.mem.value) {
                    printf(RED_TEXT "FAIL" RESET_TEXT ": expected %04X=%02X, actual %04X=%02X\n",
                        assert.mem.addr,
                        assert.mem.value,
                        assert.mem.addr,
                        nes->wram[assert.mem.addr]
                    );
                } else {
                    printf(GREEN_TEXT "PASS" RESET_TEXT ": expected %04X=%02X, actual %04X=%02X\n",
                        assert.mem.addr,
                        assert.mem.value,
                        assert.mem.addr,
                        nes->wram[assert.mem.addr]
                    );                        
                }
                break;
            }
            default:
                break;
        }
    }
}

int parse_addr(const char *str, uint16_t *addr) {
    char *end;
    unsigned long value = strtoul(str, &end, 16);

    if (*end != '\0') {
        return -1;
    }

    if (value > 0xFFFF) {
        return -1;
    }

    *addr = (uint16_t)value;
    return 0;
}

int parse_uint8(const char *str, uint8_t *value, int base) {
    char *end;

    int parsed_value = strtoul(str, &end, base);

    if (*end != '\0') {
        return -1;
    }

    *value = (uint8_t)parsed_value;

    return 0;
}

int parse_uint64(const char *str, uint64_t *value) {
    char *end;

    int parsed_value = strtoul(str, &end, 10);

    if (*end != '\0') {
        return -1;
    }

    *value = (uint64_t)parsed_value;

    return 0;
}

int parse_assert_mem_exp(const char *arg, uint16_t *addr, uint8_t *value) {
    const char *eq = strchr(arg, '=');
    if (!eq) {
        return -1;
    }

    // Parse address (left side)
    char addr_str[16];
    size_t addr_len = eq - arg;

    if (addr_len >= sizeof(addr_str)) {
        return -1;
    }

    memcpy(addr_str, arg, addr_len);
    addr_str[addr_len] = '\0';

    if (parse_addr(addr_str, addr) != 0) {
        return -1;
    }

    // Parse value (right side)
    if (parse_uint8(eq + 1, value, 16) != 0) {
        return -1;
    }

    return 0;
}

/**
 * "info"
 */

void usage_info(const char *prog) {
    fprintf(stderr,
        "nex %s [ROM]\n"
        "Print metadata about ROM file\n"
        "\n",
        prog
    );
}

static struct option info_options[] = {};

int cmd_info(int argc, char **argv) {
    int opt;

    while ((opt = getopt_long(argc, argv, "", info_options, NULL)) != -1) {
        switch (opt) {
            default:
                break;
        }
    }

    if (optind >= argc) {
        usage_info(argv[0]);
        fprintf(stderr, "\nMissing ROM file\n");
        return EXIT_FAILURE;
    }

    // init cartridge
    const char *rom_path = argv[optind];

    cartridge cart = {0};
    cartridge_load(&cart, rom_path);

    char *prg_ram_info = "";
    if (cart.has_battery_prg_ram) {
        sprintf(prg_ram_info, "PRG RAM: %d KB", 0);
    }

    printf(
        "File Info:\n"
        "  ROM format: iNES 1.0\n"
        "\n"
        "Cartridge Info:\n"
        "  PRG ROM: %d KB\n"
        "%s"
        "  CHR_ROM: %d KB\n"
        "  Mapper:  %d\n"
        "  Nametable arrangement:  %s\n",
        (int)(cart.prg_rom_size / 1000),
        prg_ram_info,
        (int)(cart.chr_rom_size / 1000),
        (int)(cart.mapper_num),
        cart.nt_mirroring == NT_MIRROR_HORIZONTAL
            ? "Vertical (\"Horizontal Mirroring\")"
            : "Horizontal (\"Vertical Mirroring\")"
    );

    cartridge_free(&cart);
    return EXIT_SUCCESS;
}