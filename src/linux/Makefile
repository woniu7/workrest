# linux platform config. Can run `make` directly in this directory.
PLATFORM := linux
VIEW     ?= gui

CC := gcc

# Main loop backend, one platform-<name>.c each:
#   glib  GNOME GLib's GMainLoop. Mandatory for the gui view -- GTK4 is built on it.
#   poll  A poll() loop over deadlines on the monotonic clock. Needs nothing but libc.
# Defaulted from the view so the common path never links GLib for no reason.
BACKENDS := glib poll
ifeq ($(VIEW), gui)
    BACKEND ?= glib
else
    BACKEND ?= poll
endif

ifeq ($(filter $(BACKEND),$(BACKENDS)),)
    $(error BACKEND must be one of: $(BACKENDS))
endif
ifeq ($(VIEW)/$(BACKEND), gui/poll)
    $(error VIEW=gui requires BACKEND=glib: GTK4 runs on the GLib main loop)
endif

# Packages the view needs...
ifeq ($(VIEW), gui)
    PKGS := gtk4              # Already pulls GLib in
else ifeq ($(VIEW), sdl3)
    PKGS := sdl3              # SDL3 >= 3.2, for SDL_RenderDebugText
else
    PKGS :=                   # The cli view is plain stdio
endif

# ...plus what the backend needs. VIEW=cli BACKEND=poll leaves this empty, and then nothing
# below shells out to pkg-config at all.
ifeq ($(BACKEND), glib)
    PKGS += glib-2.0
endif

# STATIC=1 links everything into the binary. Only realistic with BACKEND=poll and a musl
# toolchain: GLib is the one dependency with no usable static build on most distros (nixpkgs
# ships no libglib-2.0.a), which is exactly what the poll backend removes.
#
#   --static      makes pkg-config also emit Libs.private, i.e. the transitive dependencies a
#                 shared library would normally carry itself
#   -static-pie   keeps the default PIE hardening most distros enable (plain -static conflicts
#                 with -pie); fall back to "-static -no-pie" if a dependency is not PIC
#
# BUILD_DIR only carries triple + view, so run `make distclean` when switching between a
# static and a dynamic build -- the objects differ but nothing in the path says so.
STATIC ?= 0
ifeq ($(STATIC), 1)
    PKG_STATIC     := --static
    STATIC_LDFLAGS := -static-pie
endif

# Both are lazily expanded on purpose (`?=` and `=`, never `:=`): pkg-config then runs only
# when something is actually compiled or linked. With `:=` it would run while make merely
# reads this file, so `clean` / `distclean` / `info` would demand the view's dependencies be
# installed just to delete a directory or print a variable.
CFLAGS  ?= -Wall -O2 $(if $(PKGS),$(shell pkg-config $(PKG_STATIC) --cflags $(PKGS)))
LDFLAGS  = $(STATIC_LDFLAGS) $(if $(PKGS),$(shell pkg-config $(PKG_STATIC) --libs $(PKGS)))

include ../common.mk
