// Program entry point: command line parsing and the startup sequence.
// Portable: it only touches the core API, which is why it is here rather than duplicated
// once per platform.
//
// Named app.c rather than main.c on purpose. Sources are collected by basename into one flat
// object dir, so a future src/<platform>/main.c (a WinMain shim, say) would silently shadow a
// shared src/main.c -- vpath searches the platform dir first and nothing would warn.
// Leaving that name free keeps such a file an explicit addition instead of a silent override.
#include "rest.h"
#include "view.h"
#include "keyboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parse command-line options
static void parse_options(int argc, char **argv, Options *opts) {
    int i;
    opts->debug = 1; // Logging enabled by default

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            opts->debug = 1;
        } else if (strcmp(argv[i], "--no-debug") == 0) {
            opts->debug = 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Usage: rest [--debug|--no-debug]\n");
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
    Options opts;
    RestCore *core;

    parse_options(argc, argv, &opts);

    core = rest_core_new(&opts);
    view_init(core);          // View (decided at compile time: gui, cli or sdl3)
    keyboard_start(core);     // Terminal/console keyboard listener (enabled for every view)
    rest_core_start(core);
    rest_core_run(core);
    view_destroy();
    rest_core_free(core);
    return 0;
}
