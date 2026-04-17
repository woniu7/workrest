PLATFORM ?= linux
BUILD_DIR ?= $(CURDIR)/build/$(PLATFORM)

CMD = $(MAKE) -C src PLATFORM=$(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@

#just for default target
all: 
	$(CMD)
#not working for default without target
%:
	$(CMD)
%.o:
	$(CMD)

.PHONY: % all

