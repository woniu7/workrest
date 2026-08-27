# Core unit tests. Standalone: links src/rest.c (the portable state machine) against the mock
# platform + view in this directory -- no GTK/SDL, no real clock, so it builds and runs with
# nothing but a C compiler and libc.
#
# Kept out of src/ on purpose: src/common.mk collects sources with `wildcard *.c`, so a test
# file living there would be pulled into the app build. This tree has its own rules instead.
#
#   make            # build and run the tests (default)
#   make build      # build only
#   make clean
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -g -I../src

BIN  := test_core
SRCS := ../src/rest.c mock_platform.c mock_view.c test_core.c

.PHONY: test build clean

# Default goal: build then run.
test: build
	./$(BIN)

build: $(BIN)

$(BIN): $(SRCS) mocks.h ../src/rest.h ../src/config.h ../src/platform.h ../src/view.h
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN)

clean:
	rm -f $(BIN)
