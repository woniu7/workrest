BUILD_DIR ?= build

CC = gcc

GTK_CFLAGS := $(shell pkg-config --cflags gtk4)

#CFLAGS = -Wall -O2
CFLAGS ?=

SRC := rest.c
OBJ := $(patsubst %.c,%.o,$(SRC))
TARGET = rest.o

all: $(TARGET)

## 链接
#$(TARGET): $(OBJ)
#       @mkdir -p $(dir $@)
#       $(CC) $(GTK_LFLAGS) $(addprefix $(BUILD_DIR)/, $^) -o $(BUILD_DIR)/$@

%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $(BUILD_DIR)/$@

# 清理
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
