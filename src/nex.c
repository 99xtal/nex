#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"

void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options...] <command>\n"
        "  -v     Show version\n"
        "  -h     Show usage\n"
        "\n"
        "Commands\n"
        "  run    Run an ROM in the emulator\n",
        prog
    );
}

static Command commands[] = {
    { .name = "run", .execute = cmd_run },
    { .name = "test", .execute = cmd_test },
};

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    int opt;
    while ((opt = getopt(argc, argv, "vh")) != -1) {
        switch (opt) {
            case 'v':
                printf("0.1.0\n");
                return EXIT_SUCCESS;
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        Command cmd = commands[i];

        if (strcmp(argv[1], cmd.name) == 0) {
            return cmd.execute(argc - 1, argv + 1);
        }
    }

    usage(argv[0]);
    fprintf(stderr, "\nUnknown command: %s\n", argv[1]);
}