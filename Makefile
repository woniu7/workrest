PLATFORM ?= windows
BUILD_DIR ?= $(CURDIR)/build/$(PLATFORM)

all: #just for default target
	$(MAKE) -C src PLATFORM=$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@
%: #not working for default without target
	$(MAKE) -C src PLATFORM=$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@
%.o: 
	$(MAKE) -C src PLATFORM=$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@

.PHONY: % all

