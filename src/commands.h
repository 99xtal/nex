#ifndef COMMANDS_H
#define COMMANDS_H

typedef int (exec_fn)(int argc, char **argv);

typedef struct Command {
    const char *name;
    exec_fn *execute;
} Command;

exec_fn cmd_run;

#endif