#include "../rest.h"
#include "../view.h"
#include "../keyboard.h"
#include <glib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Core state machine (depends on no view, uses only the GLib main loop and timers)
struct RestCore {
    Options    opts;
    GMainLoop *loop;
    int        remaining_seconds;
    gboolean   is_resting;    // TRUE = resting, FALSE = working
    gboolean   keys_enabled;  // FALSE = ignore keys during the delay period
    guint      timer_id;      // Countdown/work timer
    guint      enable_keys_id;// Timer for delayed key enabling
};

// --- Debug log (--debug, enabled by default), written to stderr ---
static void rest_log(RestCore *c, const char *fmt, ...) {
    va_list ap;
    if (!c->opts.debug) return;
    va_start(ap, fmt);
    fprintf(stderr, "[debug] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
}

// --- Notify the view (call view_* directly; which implementation is fixed at link time) ---
static void notify_rest_begin(RestCore *c, int seconds) {
    rest_log(c, "Entering rest, countdown %d s", seconds);
    view_rest_begin(seconds);
}
static void notify_tick(RestCore *c, int seconds) {
    view_tick(seconds);
}
static void notify_work_begin(RestCore *c) {
    rest_log(c, "Entering work mode");
    view_work_begin();
}

// --- Timer management ---
static void stop_timers(RestCore *c) {
    if (c->timer_id > 0) {
        g_source_remove(c->timer_id);
        c->timer_id = 0;
    }
    if (c->enable_keys_id > 0) {
        g_source_remove(c->enable_keys_id);
        c->enable_keys_id = 0;
    }
}

// Enable keys after the delay ends
static gboolean enable_keys_cb(gpointer d) {
    RestCore *c = (RestCore *)d;
    c->keys_enabled = TRUE;
    c->enable_keys_id = 0;
    rest_log(c, "Keys enabled");
    return G_SOURCE_REMOVE; // One-shot
}

static void start_rest(RestCore *c, int seconds);
static void start_work(RestCore *c, int seconds);

// Timer callback
static gboolean on_tick(gpointer d) {
    RestCore *c = (RestCore *)d;

    if (c->is_resting) {
        c->remaining_seconds--;
        notify_tick(c, c->remaining_seconds);
        if (c->remaining_seconds <= 0) {
            start_work(c, WORK_SECONDS);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    } else {
        start_rest(c, BREAK_SECONDS);
        return G_SOURCE_REMOVE;
    }
}

// Start resting (show the UI and count down)
static void start_rest(RestCore *c, int seconds) {
    stop_timers(c);
    c->is_resting = TRUE;
    c->remaining_seconds = seconds;

    // Countdown starts: disable keys first, enable them after DELAY_SECONDS seconds
    c->keys_enabled = FALSE;
    c->enable_keys_id = g_timeout_add_seconds(DELAY_SECONDS, enable_keys_cb, c);

    notify_rest_begin(c, seconds);
    c->timer_id = g_timeout_add(1000, on_tick, c);
}

// Start working (hide the UI, enter rest after `seconds` seconds)
static void start_work(RestCore *c, int seconds) {
    stop_timers(c);
    c->is_resting = FALSE;
    notify_work_begin(c);
    c->timer_id = g_timeout_add_seconds(seconds, on_tick, c);
}

// Key dispatch (key semantics follow the current UI: q/r/c/l)
static void dispatch_key(RestCore *c, char key) {
    // Ignore keys during the first DELAY_SECONDS seconds after the countdown starts, to prevent accidental presses
    if (!c->keys_enabled) return;

    switch (key) {
        case 'q':
        case 'Q':
            rest_log(c, "Key q: quit");
            g_main_loop_quit(c->loop);
            break;
        case 'r':
        case 'R':
            rest_log(c, "Key r: postpone work by %d s", POSTPONE_SECONDS);
            start_work(c, POSTPONE_SECONDS);
            break;
        case 'c':
        case 'C':
            rest_log(c, "Key c: continue working immediately");
            start_work(c, WORK_SECONDS);
            break;
        case 'l':
        case 'L':
            rest_log(c, "Key l: set countdown to 999999");
            c->remaining_seconds = 999999;
            notify_tick(c, c->remaining_seconds);
            break;
        case 'b':
        case 'B':
            // Trigger the rest countdown immediately; if already counting down, reset to BREAK_SECONDS
            rest_log(c, "Key b: trigger/reset countdown to %d s", BREAK_SECONDS);
            start_rest(c, BREAK_SECONDS);
            break;
        default:
            break;
    }
}

// =========================================================
// Public API
// =========================================================
RestCore *rest_core_new(const Options *opts) {
    RestCore *c = g_new0(RestCore, 1);
    c->opts = *opts;
    c->loop = g_main_loop_new(NULL, FALSE);
    return c;
}

void rest_core_free(RestCore *c) {
    if (!c) return;
    stop_timers(c);
    if (c->loop) g_main_loop_unref(c->loop);
    g_free(c);
}

void rest_core_start(RestCore *c) {
    rest_log(c, "Startup: initial rest %d s", INIT_SECONDS);
    start_rest(c, INIT_SECONDS);
}

void rest_core_run(RestCore *c) {
    g_main_loop_run(c->loop);
}

// The stdin callback runs on the main loop thread, so it can dispatch directly
void rest_core_send_key(RestCore *c, char key) {
    dispatch_key(c, key);
}

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

int app_main(int argc, char **argv) {
    Options opts;
    RestCore *core;

    parse_options(argc, argv, &opts);

    core = rest_core_new(&opts);
    view_init(core);          // View (decided at compile time: GUI or CLI)
    keyboard_start(core);     // Terminal keyboard listener (enabled in both GUI and CLI modes)
    rest_core_start(core);
    rest_core_run(core);
    view_destroy();
    rest_core_free(core);
    return 0;
}
