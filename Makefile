CC = gcc

CFLAGS = -g -O0 -Wall -Wextra -Wpedantic -std=c11 -Isrc

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

TARGET = $(BUILD_DIR)/nex

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=$(OBJ_DIR)/%.o)

LDFLAGS =
LDLIBS =

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(BUILD_DIR)