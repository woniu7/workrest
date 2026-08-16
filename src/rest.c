// Core state machine: the work/rest cycle, the countdown, and the key semantics.
//
// Shared by every platform. It reaches the OS only through platform.h and the UI only
// through view.h, so there is no platform header, no #ifdef and no conditional code below.
#include "rest.h"
#include "view.h"
#include "platform.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

struct RestCore {
    Options   opts;
    int       remaining_seconds;
    int       is_resting;   // 1 = resting, 0 = working
    int       keys_enabled; // 0 = ignore keys during the delay period
    PlatTimer timer;        // Rest countdown (repeating) or work timer (one-shot)
    PlatTimer enable_keys;  // Delayed key enabling (one-shot)
};

// --- Debug log (--debug, enabled by default), written to stderr ---
static void rest_log(RestCore *c, const char *fmt, ...) {
    va_list ap;
    char ts[20];
    if (!c->opts.debug) return;
    va_start(ap, fmt);
    plat_format_time(ts, sizeof(ts));
    fprintf(stderr, "[%s] [debug] ", ts);
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

static void start_rest(RestCore *c, int seconds);
static void start_work(RestCore *c, int seconds);

// --- Timer management ---
static void stop_timers(RestCore *c) {
    if (c->timer != PLAT_TIMER_NONE) {
        plat_timer_del(c->timer);
        c->timer = PLAT_TIMER_NONE;
    }
    if (c->enable_keys != PLAT_TIMER_NONE) {
        plat_timer_del(c->enable_keys);
        c->enable_keys = PLAT_TIMER_NONE;
    }
}

// One-shot: the delay after a countdown starts is over, keys count again.
// It clears its own handle first -- a fired one-shot no longer exists.
static void on_enable_keys(void *user) {
    RestCore *c = (RestCore *)user;
    c->enable_keys = PLAT_TIMER_NONE;
    c->keys_enabled = 1;
    rest_log(c, "Keys enabled");
}

// Repeating, once a second while resting
static void on_countdown(void *user) {
    RestCore *c = (RestCore *)user;
    c->remaining_seconds--;
    notify_tick(c, c->remaining_seconds);
    if (c->remaining_seconds <= 0) {
        start_work(c, WORK_SECONDS); // Replaces this timer, see stop_timers
    }
}

// One-shot: the work stretch is over, time for a break
static void on_work_done(void *user) {
    RestCore *c = (RestCore *)user;
    c->timer = PLAT_TIMER_NONE;
    start_rest(c, BREAK_SECONDS);
}

// Start resting (show the UI and count down)
static void start_rest(RestCore *c, int seconds) {
    stop_timers(c);
    c->is_resting = 1;
    c->remaining_seconds = seconds;

    // Countdown starts: disable keys first, enable them after DELAY_SECONDS seconds
    c->keys_enabled = 0;
    c->enable_keys = plat_timer_add_seconds(DELAY_SECONDS, 0, on_enable_keys, c);

    notify_rest_begin(c, seconds);
    c->timer = plat_timer_add(1000, 1, on_countdown, c);
}

// Start working (hide the UI, enter rest after `seconds` seconds)
static void start_work(RestCore *c, int seconds) {
    stop_timers(c);
    c->is_resting = 0;
    notify_work_begin(c);
    c->timer = plat_timer_add_seconds(seconds, 0, on_work_done, c);
}

// Key dispatch (key semantics follow the current UI: q/r/c/l/b)
static void dispatch_key(RestCore *c, char key) {
    // Ignore keys during the first DELAY_SECONDS seconds after the countdown starts, to prevent accidental presses
    if (!c->keys_enabled) return;

    switch (key) {
        case 'q':
        case 'Q':
            rest_log(c, "Key q: quit");
            plat_quit();
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

// --- Callbacks handed to the platform layer ---
static void on_key(void *user, char key) {
    dispatch_key((RestCore *)user, key);
}

// Only fires where the platform implements it (windows); linux has no lock-screen hook
static void on_unlock(void *user) {
    RestCore *c = (RestCore *)user;
    rest_log(c, "Session unlocked, re-entering rest");
    start_rest(c, INIT_SECONDS);
}

// =========================================================
// Public API
// =========================================================
RestCore *rest_core_new(const Options *opts) {
    RestCore *c = (RestCore *)calloc(1, sizeof(RestCore));
    if (!c) return NULL;

    c->opts        = *opts;
    c->timer       = PLAT_TIMER_NONE;
    c->enable_keys = PLAT_TIMER_NONE;

    plat_init();
    plat_set_key_handler(on_key, c);
    plat_set_unlock_handler(on_unlock, c);
    return c;
}

void rest_core_free(RestCore *c) {
    if (!c) return;
    stop_timers(c);
    plat_shutdown();
    free(c);
}

void rest_core_start(RestCore *c) {
    rest_log(c, "Startup: initial rest %d s", INIT_SECONDS);
    start_rest(c, INIT_SECONDS);
}

void rest_core_run(RestCore *c) {
    (void)c;
    plat_run();
}

// The platform layer routes this to the main loop thread and back into on_key above.
// `c` is unused: there is one core per process and the platform layer already holds it.
void rest_core_send_key(RestCore *c, char key) {
    (void)c;
    plat_post_key(key);
}
