// Main loop backend built on GLib. Required by the gui view: GTK4 is built on GLib and has to
// be driven by a GMainLoop. Every other view defaults to platform-poll.c, which needs nothing
// beyond libc -- GLib here is the GNOME utility library, not the C library.
#include "../platform.h"
#include "loop.h"
#include <glib.h>
#include <glib-unix.h>

static GMainLoop *g_loop = NULL;

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
// fd watching (loop.h)
// =========================
typedef struct {
    LoopFdFunc fn;
    void      *user;
} WatchCtx;

static gboolean watch_trampoline(gint fd, GIOCondition condition, gpointer data) {
    WatchCtx *w = (WatchCtx *)data;
    (void)fd;
    if (condition & (G_IO_HUP | G_IO_ERR)) return G_SOURCE_REMOVE;
    return w->fn(w->user) ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

void loop_watch_fd(int fd, LoopFdFunc fn, void *user) {
    WatchCtx *w = g_new0(WatchCtx, 1);
    w->fn   = fn;
    w->user = user;
    g_unix_fd_add_full(G_PRIORITY_DEFAULT, fd, G_IO_IN | G_IO_HUP | G_IO_ERR,
                       watch_trampoline, w, g_free);
}
