PLATFORM ?= linux
BUILD_DIR := $(CURDIR)/build/$(PLATFORM)

all: #just for default target
	$(MAKE) -C src/$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@
%: #not working for default without target
	$(MAKE) -C src/$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@
%.o: #match *.o while the % does not match
	$(MAKE) -C src/$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@

.PHONY: all %
