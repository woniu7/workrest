PLATFORM ?= linux

# BUILD_DIR is derived in src/common.mk as build/<target triple>/<view>, which needs to ask
# the platform's compiler for its triple -- something this file cannot know. So it is not set
# here; pass BUILD_DIR=... on the command line to override.
CMD = $(MAKE) -C src PLATFORM=$(PLATFORM) $@

#just for default target
all: 
	$(CMD)
#not working for default without target
%:
	$(CMD)
%.o:
	$(CMD)

.PHONY: % all

