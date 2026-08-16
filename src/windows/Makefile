# windows platform config (mingw cross-compile). Can run `make` directly in this directory.
PLATFORM := windows
VIEW     ?= gui

CC      := x86_64-w64-mingw32-gcc
CFLAGS  ?= -Wall -O2
LDFLAGS := -lgdi32 -luser32 -lwtsapi32
TARGET  := rest.exe

ifeq ($(VIEW), sdl3)
    # SDL3 (>= 3.2) built for mingw. Override PKG_CONFIG if your cross build needs it, e.g.
    #   make PLATFORM=windows VIEW=sdl3 PKG_CONFIG=x86_64-w64-mingw32-pkg-config
    PKG_CONFIG ?= pkg-config
    CFLAGS  += $(shell $(PKG_CONFIG) --cflags sdl3)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs sdl3)
endif

CFLAGS += $(EXTRA_CFLAGS)

include ../common.mk
