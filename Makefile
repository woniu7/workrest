PLATFORM ?= linux

# BUILD_DIR is derived in src/common.mk as build/<target triple>/<view>, which needs to ask
# the platform's compiler for its triple -- something this file cannot know. So it is not set
# here; pass BUILD_DIR=... on the command line to override.
CMD = $(MAKE) -C src PLATFORM=$(PLATFORM) $@

#just for default target
all:
	$(CMD)

# Core unit tests live in their own tree (test/), not under src/, and don't go through the
# platform sub-make -- so give them an explicit target ahead of the catch-all below.
test:
	$(MAKE) -C test

#not working for default without target
%:
	$(CMD)
%.o:
	$(CMD)

.PHONY: % all test

