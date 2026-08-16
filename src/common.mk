# =========================
# Common build logic
# Included by src/<platform>/Makefile.
# The caller (platform Makefile) must define CC / CFLAGS / LDFLAGS first, optionally TARGET / VIEW.
# =========================

TARGET     ?= rest
VIEW       ?= gui

# Directory of the common sources (i.e. src/)
COMMON_DIR := ..

# Output layout: build/<target triple>/<view>/
#
# The triple is asked of the compiler itself, so glibc / musl / mingw / other architectures
# separate on their own (x86_64-linux-gnu, x86_64-alpine-linux-musl, x86_64-w64-mingw32,
# aarch64-linux-gnu ...) and a future musl or arm target needs no change here. It is an
# opaque id on purpose: field count varies between distros, so never parse it.
# Falls back to "unknown" rather than erroring out, so clean/info still work with no compiler.
#
# Rooted at the repo top level so `make` from the top and `make` from inside src/<platform>/
# land in the same place. Every configuration gets its own directory: switching VIEW can no
# longer leave you linking the previous view's objects into the binary.
TRIPLE     := $(or $(shell $(CC) -dumpmachine 2>/dev/null),unknown)
BUILD_ROOT ?= $(abspath $(COMMON_DIR)/..)/build
BUILD_DIR  ?= $(BUILD_ROOT)/$(TRIPLE)/$(VIEW)

# View implementations (pick one at compile time). Only gui is genuinely platform-specific
# and lives in the platform dir; cli and sdl3 are portable and live in the common dir.
# A new view must be listed here, otherwise it is not excluded from the other views' builds
# and every link fails on a duplicate view_init.
VIEWS := gui cli sdl3

ifeq ($(filter $(VIEW),$(VIEWS)),)
    $(error VIEW must be one of: $(VIEWS))
endif

# Every view source but the selected one is dropped from the build
VIEW_EXCLUDE := $(addsuffix .c,$(filter-out $(VIEW),$(VIEWS)))

# Sources: common dir (../*.c) + current platform dir (./*.c), minus the unselected view impls
COMMON_SRCS := $(filter-out $(VIEW_EXCLUDE),$(notdir $(wildcard $(COMMON_DIR)/*.c)))
PLAT_SRCS   := $(filter-out $(VIEW_EXCLUDE),$(notdir $(wildcard *.c)))
SRCS        := $(COMMON_SRCS) $(PLAT_SRCS)
OBJS        := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))
DEPS        := $(OBJS:.o=.d)

# Let the %.c rule find sources in both the platform dir and the common dir
vpath %.c . $(COMMON_DIR)

# =========================
# Build rules
# =========================
all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# -MMD writes a $(BUILD_DIR)/<name>.d listing the project headers this .o depends on, so
# editing e.g. rest.h rebuilds everything that includes it (system headers are left out on
# purpose: -MMD, not -MD). -MP adds a phony target per header, so deleting or renaming one
# does not break the build with "No rule to make target".
# Kept in the recipe rather than in CFLAGS: CFLAGS is `?=` in the platform Makefiles and can
# be wiped from the command line, dependency tracking should survive that.
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Pull in the generated dependencies; '-' because they do not exist on the first build
-include $(DEPS)

# =========================
# Clean
# =========================
# clean drops the current configuration only (this triple + this view), matching the
# "a configuration is a property of its directory" rule; distclean drops every build output.
clean:
	rm -rf $(BUILD_DIR)

distclean:
	rm -rf $(BUILD_ROOT)

# =========================
# Print configuration (for debugging)
# =========================
info:
	@echo "Platform:  $(PLATFORM)"
	@echo "Triple:    $(TRIPLE)"
	@echo "View:      $(VIEW)"
	@echo "Compiler:  $(CC)"
	@echo "Build dir: $(BUILD_DIR)"
	@echo "Sources:   $(SRCS)"

.PHONY: all clean distclean info
