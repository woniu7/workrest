# =========================
# Common build logic
# Included by src/<platform>/Makefile.
# The caller (platform Makefile) must define CC / CFLAGS / LDFLAGS first, optionally TARGET / VIEW.
# =========================

TARGET     ?= rest
VIEW       ?= gui
BUILD_DIR  ?= build

# Directory of the common sources (i.e. src/)
COMMON_DIR := ..

# View implementation (pick one at compile time): cli (default) or gui
ifeq ($(VIEW), gui)
    VIEW_EXCLUDE := cli.c
else ifeq ($(VIEW), cli)
    VIEW_EXCLUDE := gui.c
else
    $(error VIEW must be cli or gui)
endif

# Sources: common dir (../*.c) + current platform dir (./*.c), minus the unselected view impl
COMMON_SRCS := $(notdir $(wildcard $(COMMON_DIR)/*.c))
PLAT_SRCS   := $(filter-out $(VIEW_EXCLUDE),$(wildcard *.c))
SRCS        := $(COMMON_SRCS) $(PLAT_SRCS)
OBJS        := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

# Let the %.c rule find sources in both the platform dir and the common dir
vpath %.c . $(COMMON_DIR)

# =========================
# Build rules
# =========================
all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# Clean
# =========================
clean:
	rm -rf $(BUILD_DIR)

# =========================
# Print configuration (for debugging)
# =========================
info:
	@echo "Platform: $(PLATFORM)"
	@echo "View:     $(VIEW)"
	@echo "Compiler: $(CC)"
	@echo "Sources:  $(SRCS)"

.PHONY: all clean info
