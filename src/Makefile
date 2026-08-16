# =========================
# Top-level entry: only sets the default platform and forwards targets.
# The actual build logic lives in common.mk; per-platform config in <platform>/Makefile.
#
#   make [PLATFORM=linux]        # equivalent to: cd linux && make
#   make PLATFORM=windows        # equivalent to: cd windows && make
#   make PLATFORM=linux VIEW=gui # variables are passed through to the sub-make
#   make clean / make info       # also forwarded to the corresponding platform
# =========================

# Default platform
PLATFORM ?= linux

# Forward targets to the build under src/<platform>/
all clean distclean info:
	$(MAKE) -C $(PLATFORM) $@

.PHONY: all clean distclean info
