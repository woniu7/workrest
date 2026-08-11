# windows 平台配置(mingw 交叉编译)。可在本目录直接 make。
PLATFORM := windows
VIEW     ?= terminal

CC      := x86_64-w64-mingw32-gcc
CFLAGS  ?= -Wall -O2
LDFLAGS := -lgdi32 -luser32 -lwtsapi32
TARGET  := rest.exe

CFLAGS += $(EXTRA_CFLAGS)

include ../common.mk
