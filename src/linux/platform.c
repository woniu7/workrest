// Platform layer, GLib implementation (see ../platform.h).
#include "../platform.h"
#include <glib.h>
#include <time.h>

static GMainLoop *g_loop = NULL;

static PlatKeyFunc g_key_fn   = NULL;
static void       *g_key_user = NULL;

// =========================
// Main loop
// =========================
void plat_init(void) {
    g_loop = g_main_loop_new(NULL, FALSE);
}

void plat_shutdown(void) {
    if (g_loop) {
        g_main_loop_unref(g_loop);
        g_loop = NULL;
    }
}

void plat_run(void) {
    g_main_loop_run(g_loop);
}

void plat_quit(void) {
    g_main_loop_quit(g_loop);
}

// =========================
// Timers
// =========================
// The GLib source id doubles as our handle. The callback and its user pointer ride along as
// the source's user data, and GLib frees that context when the source goes away.
typedef struct {
    PlatTimerFunc fn;
    void         *user;
    int           repeat;
} TimerCtx;

static gboolean timer_trampoline(gpointer data) {
    TimerCtx *t = (TimerCtx *)data;
    // Read repeat *before* dispatching: the callback is allowed to delete this very timer,
    // and touching the context afterwards would be reading a soon-to-be-freed struct.
    int repeat = t->repeat;
    t->fn(t->user);
    return repeat ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static TimerCtx *timer_ctx_new(PlatTimerFunc fn, void *user, int repeat) {
    TimerCtx *t = g_new0(TimerCtx, 1);
    t->fn     = fn;
    t->user   = user;
    t->repeat = repeat;
    return t;
}

PlatTimer plat_timer_add(unsigned ms, int repeat, PlatTimerFunc fn, void *user) {
    TimerCtx *t = timer_ctx_new(fn, user, repeat);
    return (PlatTimer)g_timeout_add_full(G_PRIORITY_DEFAULT, ms,
                                         timer_trampoline, t, g_free);
}

PlatTimer plat_timer_add_seconds(unsigned sec, int repeat, PlatTimerFunc fn, void *user) {
    TimerCtx *t = timer_ctx_new(fn, user, repeat);
    // g_timeout_add_seconds_full lets GLib align the wakeup to a second boundary
    return (PlatTimer)g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, sec,
                                                 timer_trampoline, t, g_free);
}

void plat_timer_del(PlatTimer t) {
    if (t != PLAT_TIMER_NONE) {
        g_source_remove((guint)t);
    }
}

// =========================
// Key delivery
// =========================
void plat_set_key_handler(PlatKeyFunc fn, void *user) {
    g_key_fn   = fn;
    g_key_user = user;
}

// Every key source on linux already runs on the main loop thread -- the stdin watch
// (g_unix_fd_add), the GTK key controller, and the SDL poll driven by a GLib timer -- so
// this dispatches straight through instead of hopping threads.
void plat_post_key(char key) {
    if (g_key_fn) g_key_fn(g_key_user, key);
}

// =========================
// Session unlock
// =========================
// Not wired up on linux by design: no lock-screen integration here. The hook exists for
// windows, which does get a session notification.
void plat_set_unlock_handler(PlatUnlockFunc fn, void *user) {
    (void)fn;
    (void)user;
}

// =========================
// Misc
// =========================
void plat_format_time(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_now);
}
