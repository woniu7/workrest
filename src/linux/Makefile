BUILD_DIR ?= build

CC = gcc

GTK_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK_LFLAGS := $(shell pkg-config --libs gtk4)

#CFLAGS = -Wall -O2
CFLAGS ?=

SRC := rest.c
OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
TARGET = $(BUILD_DIR)/rest

all: $(TARGET)

# 链接
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(GTK_LFLAGS) $^ -o $@

# 编译
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# 清理
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
