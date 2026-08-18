# windows platform config (mingw cross-compile). Can run `make` directly in this directory.
PLATFORM := windows
VIEW     ?= gui

CC       := x86_64-w64-mingw32-gcc
TARGET   := rest.exe

CFLAGS   := -Wall -O2
LDFLAGS  := -lgdi32 -luser32 -lwtsapi32

ifeq ($(VIEW), sdl3)
    PKG_CONFIG ?= x86_64-w64-mingw32-pkg-config
    CFLAGS  += $(shell $(PKG_CONFIG) --cflags sdl3)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs sdl3)
endif

include ../common.mk
