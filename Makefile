CC = gcc

CFLAGS = -g -O0 -Wall -Wextra -Wpedantic -std=c11 -Isrc -Iexternal/lib6502/include

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

TARGET = $(BUILD_DIR)/nex

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=$(OBJ_DIR)/%.o)

LDFLAGS = -Lexternal/lib6502/build
LDLIBS = -l6502

LIB6502_DIR = external/lib6502
LIB6502 = $(LIB6502_DIR)/build/lib6502.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ) $(LIB6502)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB6502):
	$(MAKE) -C $(LIB6502_DIR)

clean:
	rm -rf $(BUILD_DIR)