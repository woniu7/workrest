# windows platform config (mingw cross-compile). Can run `make` directly in this directory.
PLATFORM := windows
VIEW     ?= gui

CC      := x86_64-w64-mingw32-gcc
TARGET  := rest.exe

# Lazily expanded (`?=` and `=`, never `:=`) so the pkg-config call added below for the sdl3
# view runs only when compiling or linking, not while make reads this file -- otherwise
# clean / distclean / info would need SDL3 installed just to delete a directory.
CFLAGS  ?= -Wall -O2
LDFLAGS  = -lgdi32 -luser32 -lwtsapi32

ifeq ($(VIEW), sdl3)
    # SDL3 (>= 3.2) built for mingw. Override PKG_CONFIG if your cross build needs it, e.g.
    #   make PLATFORM=windows VIEW=sdl3 PKG_CONFIG=x86_64-w64-mingw32-pkg-config
    PKG_CONFIG ?= pkg-config
    CFLAGS  += $(shell $(PKG_CONFIG) --cflags sdl3)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs sdl3)
endif

# Append-only escape hatches; setting CFLAGS/LDFLAGS on the command line would override the
# values above instead of adding to them
CFLAGS  += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

include ../common.mk
