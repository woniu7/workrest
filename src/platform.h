#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>

// Everything the core state machine needs from the operating system, implemented once per
// platform in <platform>/platform.c.
//
// This header is the entire surface between rest.c and the OS: rest.c contains no platform
// header, no #ifdef and no conditional code, which is what lets a single copy of the
// work/rest logic serve every platform.

// =========================
// Main loop
// =========================
void plat_init(void);      // Create the main loop (plus whatever carries timers and notifications)
void plat_shutdown(void);  // Tear it down
void plat_run(void);       // Run the loop; blocks until plat_quit
void plat_quit(void);      // Ask the loop to stop

// =========================
// Timers
// =========================
// Handle to a scheduled timer; PLAT_TIMER_NONE means "none".
//
// A repeating timer lives until plat_timer_del. A one-shot timer is destroyed just before
// its callback runs, so its handle is dead from inside the callback onwards: the callback
// must clear whatever variable holds it, and plat_timer_del must not be called on a
// one-shot that already fired.
//
// Deleting a timer from inside its own callback is allowed (the state machine does exactly
// that when a countdown ends), as is scheduling new timers from a callback.
typedef unsigned long PlatTimer;
#define PLAT_TIMER_NONE 0

typedef void (*PlatTimerFunc)(void *user);

PlatTimer plat_timer_add(unsigned ms, int repeat, PlatTimerFunc fn, void *user);

// Whole-second variant. Prefer it for long waits: the OS is allowed to align these to second
// boundaries and coalesce wakeups, which is worth having on a 45-minute timer.
PlatTimer plat_timer_add_seconds(unsigned sec, int repeat, PlatTimerFunc fn, void *user);

void plat_timer_del(PlatTimer t);

// =========================
// Key delivery
// =========================
// Keys can arrive on any thread (the windows console listener runs on its own). plat_post_key
// hands one over to the main loop thread, where the registered handler runs.
typedef void (*PlatKeyFunc)(void *user, char key);

void plat_set_key_handler(PlatKeyFunc fn, void *user);
void plat_post_key(char key);

// =========================
// Session unlock
// =========================
// Called after the desktop is unlocked or resumed, so the core can restart the cycle.
// Only windows implements this today; see the linux stub for what it would take there.
typedef void (*PlatUnlockFunc)(void *user);

void plat_set_unlock_handler(PlatUnlockFunc fn, void *user);

// =========================
// Misc
// =========================
// Local time as "YYYY-MM-DD HH:MM:SS" (localtime_r vs localtime_s).
void plat_format_time(char *buf, size_t size);

#endif // PLATFORM_H
