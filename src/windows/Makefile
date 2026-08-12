# windows platform config (mingw cross-compile). Can run `make` directly in this directory.
PLATFORM := windows
VIEW     ?= gui

CC      := x86_64-w64-mingw32-gcc
CFLAGS  ?= -Wall -O2
LDFLAGS := -lgdi32 -luser32 -lwtsapi32
TARGET  := rest.exe

CFLAGS += $(EXTRA_CFLAGS)

include ../common.mk
