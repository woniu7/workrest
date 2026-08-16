#ifndef REST_H
#define REST_H

// Timing constants live in config.h, included only by the files that read them.
// This header is pulled into every translation unit, so keep it to the API alone.

// Runtime options (parsed from the command line)
typedef struct {
    int debug;   // --debug: show logs (enabled by default)
} Options;

// Core state machine (opaque type; implemented once in rest.c, shared by every platform --
// it reaches the OS through platform.h and the UI through view.h, and depends on neither directly)
typedef struct RestCore RestCore;

RestCore *rest_core_new(const Options *opts);
void rest_core_free(RestCore *core);
void rest_core_start(RestCore *core);   // Enter the initial rest, start the state machine
void rest_core_run(RestCore *core);     // Run the main loop, blocks until exit

// Thread-safely feed in a key (the view may call from any thread). 
// Keys are ignored for a short delay after the countdown starts (DELAY_SECONDS in config.h), to prevent accidental presses.
void rest_core_send_key(RestCore *core, char key);

#endif // REST_H
