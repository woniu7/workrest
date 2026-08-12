#ifndef REST_H
#define REST_H

// Configuration constants
#define WORK_SECONDS 2700
#define BREAK_SECONDS 180
#define INIT_SECONDS 3
#define DELAY_SECONDS 1
#define POSTPONE_SECONDS 120  // Seconds the 'r' key postpones work by

// Runtime options (parsed from the command line)
typedef struct {
    int debug;   // --debug: show logs (enabled by default)
} Options;

// Core state machine (opaque type; implemented per platform in rest.c, depends on no view)
typedef struct RestCore RestCore;

RestCore *rest_core_new(const Options *opts);
void rest_core_free(RestCore *core);
void rest_core_start(RestCore *core);   // Enter the initial rest, start the state machine
void rest_core_run(RestCore *core);     // Run the main loop, blocks until exit

// Thread-safely feed in a key (the view may call from any thread). 
// Keys are ignored during the first DELAY_SECONDS seconds after the countdown starts, to prevent accidental presses.
void rest_core_send_key(RestCore *core, char key);

int app_main(int argc, char **argv);

#endif // REST_H
