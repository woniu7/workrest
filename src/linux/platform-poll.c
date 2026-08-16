// Main loop backend with no dependencies beyond libc: a poll() loop plus a table of
// deadlines on the monotonic clock. Used by every view except gui (GTK4 is built on GLib and
// must be driven by its main loop, see platform-glib.c).
//
// Timers are deadlines rather than one timerfd each: no Linux-only syscall, no file
// descriptor per timer, and the poll timeout falls out of "how long until the nearest
// deadline". Everything here is POSIX, so this backend would also serve a BSD or macOS port.
#include "../platform.h"
#include "loop.h"
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>

#define MAX_TIMERS  8 // The state machine keeps at most 3 live; sdl3.c adds one more
#define MAX_WATCHES 4 // Only stdin today

typedef struct {
    int           used;
    int           repeat;
    unsigned      interval_ms;
    long long     deadline_ms;
    PlatTimerFunc fn;
    void         *user;
    PlatTimer     id;
} Timer;

typedef struct {
    int        used;
    int        fd;
    LoopFdFunc fn;
    void      *user;
} Watch;

static Timer     g_timers[MAX_TIMERS];
static Watch     g_watches[MAX_WATCHES];
static PlatTimer g_next_id = 0; // Ever-increasing, so a stale handle never names a new timer
static int       g_running = 0;

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// =========================
// Main loop
// =========================
void plat_init(void) {
    int i;
    for (i = 0; i < MAX_TIMERS; i++)  g_timers[i].used = 0;
    for (i = 0; i < MAX_WATCHES; i++) g_watches[i].used = 0;
}

void plat_shutdown(void) {
    plat_init();
}

void plat_quit(void) {
    g_running = 0;
}

// Milliseconds until the nearest deadline, or -1 when no timer is armed (poll blocks)
static int next_timeout_ms(void) {
    long long now  = now_ms();
    long long best = -1;
    int i;

    for (i = 0; i < MAX_TIMERS; i++) {
        long long left;
        if (!g_timers[i].used) continue;
        left = g_timers[i].deadline_ms - now;
        if (left < 0) left = 0;
        if (best < 0 || left < best) best = left;
    }
    if (best < 0) return -1;
    if (best > INT_MAX) best = INT_MAX;
    return (int)best;
}

// Fire everything that is due, earliest first.
//
// Re-scanning the table after every callback is deliberate: a callback may delete this timer,
// add new ones, or delete a timer that was also due (the state machine does all three -- when
// the countdown hits zero it replaces its own timer from inside its own callback). Holding a
// pointer or index across a callback would be reading a slot that has since been reused.
static void fire_due_timers(void) {
    for (;;) {
        long long      now  = now_ms();
        Timer         *due  = NULL;
        PlatTimerFunc  fn;
        void          *user;
        int            i;

        for (i = 0; i < MAX_TIMERS; i++) {
            if (!g_timers[i].used) continue;
            if (g_timers[i].deadline_ms > now) continue;
            if (!due || g_timers[i].deadline_ms < due->deadline_ms) due = &g_timers[i];
        }
        if (!due) return;

        fn   = due->fn;
        user = due->user;

        if (due->repeat) {
            // Re-arm from now rather than from the old deadline: same semantics as GLib's
            // timeouts, and a slow callback can never build up a backlog of instant refires.
            due->deadline_ms = now + due->interval_ms;
        } else {
            due->used = 0; // A one-shot is retired before it runs; its handle is dead now
        }

        fn(user);
    }
}

void plat_run(void) {
    g_running = 1;

    while (g_running) {
        struct pollfd fds[MAX_WATCHES];
        int           slot[MAX_WATCHES];
        int           nfds = 0;
        int           i, r;

        for (i = 0; i < MAX_WATCHES; i++) {
            if (!g_watches[i].used) continue;
            fds[nfds].fd      = g_watches[i].fd;
            fds[nfds].events  = POLLIN;
            fds[nfds].revents = 0;
            slot[nfds]        = i;
            nfds++;
        }

        r = poll(fds, (nfds_t)nfds, next_timeout_ms());
        if (r < 0) {
            if (errno == EINTR) continue; // A signal, not a failure: just go round again
            return;
        }

        fire_due_timers();

        for (i = 0; i < nfds && r > 0; i++) {
            Watch *w = &g_watches[slot[i]];
            if (!fds[i].revents) continue;
            // A timer callback above may have torn this watch down, so re-check the slot
            // still holds the same fd before dispatching
            if (!w->used || w->fd != fds[i].fd) continue;
            if (!w->fn(w->user)) w->used = 0;
        }
    }
}

// =========================
// Timers
// =========================
static PlatTimer timer_add(unsigned ms, int repeat, PlatTimerFunc fn, void *user) {
    int i;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (g_timers[i].used) continue;
        g_timers[i].used        = 1;
        g_timers[i].repeat      = repeat;
        g_timers[i].interval_ms = ms;
        g_timers[i].deadline_ms = now_ms() + ms;
        g_timers[i].fn          = fn;
        g_timers[i].user        = user;
        g_timers[i].id          = ++g_next_id; // Starts at 1, never collides with PLAT_TIMER_NONE
        return g_timers[i].id;
    }
    return PLAT_TIMER_NONE;
}

PlatTimer plat_timer_add(unsigned ms, int repeat, PlatTimerFunc fn, void *user) {
    return timer_add(ms, repeat, fn, user);
}

// No coalescing to arrange here: this loop is the only thing waiting, and poll already sleeps
// exactly until the nearest deadline
PlatTimer plat_timer_add_seconds(unsigned sec, int repeat, PlatTimerFunc fn, void *user) {
    return timer_add(sec * 1000, repeat, fn, user);
}

void plat_timer_del(PlatTimer t) {
    int i;
    if (t == PLAT_TIMER_NONE) return;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (g_timers[i].used && g_timers[i].id == t) {
            g_timers[i].used = 0;
            return;
        }
    }
}

// =========================
// fd watching (loop.h)
// =========================
void loop_watch_fd(int fd, LoopFdFunc fn, void *user) {
    int i;
    for (i = 0; i < MAX_WATCHES; i++) {
        if (g_watches[i].used) continue;
        g_watches[i].used = 1;
        g_watches[i].fd   = fd;
        g_watches[i].fn   = fn;
        g_watches[i].user = user;
        return;
    }
}
