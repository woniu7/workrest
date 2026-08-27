// Mock platform layer (implements platform.h) for the core unit tests.
//
// Time is virtual: nothing sleeps, the test advances the clock explicitly with mock_advance().
// The timer table mirrors the real poll backend (platform-poll.c) exactly so the tests exercise
// the same firing rules the production code relies on:
//   - a one-shot is retired (its slot freed) *before* its callback runs;
//   - a repeating timer is re-armed from the fire time before its callback runs;
//   - deleting a timer from inside its own callback is allowed;
//   - due timers fire earliest-deadline-first, re-scanning after each callback (a callback may
//     add, delete, or replace timers -- start_work does all three when a countdown ends).
// plat_post_key dispatches straight through, matching the linux plat_shared.c (every key source
// there already runs on the loop thread).
#include "platform.h"
#include "mocks.h"
#include <string.h>

#define MAX_TIMERS 16 // The core keeps at most 3 live; headroom so a bug shows as a leak, not a full table

typedef struct {
    int           used;
    int           repeat;
    unsigned      interval_ms;
    long long     deadline_ms;
    PlatTimerFunc fn;
    void         *user;
    PlatTimer     id;
} Timer;

static Timer     g_timers[MAX_TIMERS];
static PlatTimer g_next_id;
static long long g_now_ms;
static int       g_quit_called;

static PlatKeyFunc    g_key_fn;
static void          *g_key_user;
static PlatUnlockFunc g_unlock_fn;
static void          *g_unlock_user;

// =========================
// Main loop
// =========================
// plat_init is the per-test reset point: the core calls it from rest_core_new, so each fresh
// core starts at t=0 with an empty timer table and quit cleared.
void plat_init(void) {
    memset(g_timers, 0, sizeof(g_timers));
    g_next_id     = 0;
    g_now_ms      = 0;
    g_quit_called = 0;
    g_key_fn      = NULL;
    g_key_user    = NULL;
    g_unlock_fn   = NULL;
    g_unlock_user = NULL;
}

void plat_shutdown(void) {
    memset(g_timers, 0, sizeof(g_timers));
}

// The tests drive timers through mock_advance() rather than a blocking loop, so run is a no-op.
void plat_run(void) {
}

void plat_quit(void) {
    g_quit_called = 1;
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
        g_timers[i].deadline_ms = g_now_ms + ms;
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
// Key + unlock delivery
// =========================
void plat_set_key_handler(PlatKeyFunc fn, void *user) {
    g_key_fn   = fn;
    g_key_user = user;
}

void plat_post_key(char key) {
    if (g_key_fn) g_key_fn(g_key_user, key);
}

void plat_set_unlock_handler(PlatUnlockFunc fn, void *user) {
    g_unlock_fn   = fn;
    g_unlock_user = user;
}

// =========================
// Misc
// =========================
// The core only formats time for its debug log, which the tests keep off; a stub is enough.
void plat_format_time(char *buf, size_t size) {
    if (size) buf[0] = '\0';
}

// =========================
// Test control (mocks.h)
// =========================
long long mock_now_ms(void)   { return g_now_ms; }
int       mock_quit_called(void) { return g_quit_called; }

int mock_timer_count(void) {
    int i, n = 0;
    for (i = 0; i < MAX_TIMERS; i++) if (g_timers[i].used) n++;
    return n;
}

void mock_unlock(void) {
    if (g_unlock_fn) g_unlock_fn(g_unlock_user);
}

// Advance the clock to now+ms, firing due timers as it crosses their deadlines. Each iteration
// picks the earliest timer whose deadline is within the target, moves the clock to that deadline
// (so a callback that schedules "now + X" sees the right now), applies the retire/re-arm rule,
// then runs the callback. Re-scanning from scratch each time is deliberate: the callback may
// have freed or reused any slot -- the same reason the real backend re-scans.
void mock_advance(long long ms) {
    long long target = g_now_ms + ms;
    for (;;) {
        Timer        *due = NULL;
        PlatTimerFunc fn;
        void         *user;
        int           i;

        for (i = 0; i < MAX_TIMERS; i++) {
            if (!g_timers[i].used) continue;
            if (g_timers[i].deadline_ms > target) continue;
            if (!due || g_timers[i].deadline_ms < due->deadline_ms) due = &g_timers[i];
        }
        if (!due) break;

        g_now_ms = due->deadline_ms;
        fn   = due->fn;
        user = due->user;

        if (due->repeat) due->deadline_ms = g_now_ms + due->interval_ms;
        else             due->used = 0;

        fn(user);
    }
    g_now_ms = target;
}
