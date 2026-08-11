# linux 平台配置。可在本目录直接 make。
PLATFORM := linux
VIEW     ?= gui

CC := gcc

ifeq ($(VIEW), gui)
    # GUI 视图需要 GTK4(其已包含 GLib)
    CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags gtk4)
    LDFLAGS := $(shell pkg-config --libs gtk4)
else
    # 终端视图只需 GLib(主循环 + g_unix_fd_add)
    CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags glib-2.0)
    LDFLAGS := $(shell pkg-config --libs glib-2.0)
endif

CFLAGS += $(EXTRA_CFLAGS)

include ../common.mk
