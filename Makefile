PLATFORM ?= linux
BUILD_DIR := $(CURDIR)/build/$(PLATFORM)

%:
	$(MAKE) -C src/$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@

.PHONY: %
