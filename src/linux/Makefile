# linux platform config. Can run `make` directly in this directory.
PLATFORM := linux
VIEW     ?= gui

CC := gcc

ifeq ($(VIEW), gui)
    # The GUI view needs GTK4 (which already includes GLib)
    CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags gtk4)
    LDFLAGS := $(shell pkg-config --libs gtk4)
else
    # The CLI view only needs GLib (main loop + g_unix_fd_add)
    CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags glib-2.0)
    LDFLAGS := $(shell pkg-config --libs glib-2.0)
endif

CFLAGS += $(EXTRA_CFLAGS)

include ../common.mk
