BUILD_DIR ?= build

CC = x86_64-w64-mingw32-gcc

#CFLAGS = -Wall -O2
CFLAGS ?=

SRC := rest.c
OBJ := $(patsubst %.c,%.o,$(SRC))
TARGET = rest.o

all: $(TARGET)

%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

# 清理
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
