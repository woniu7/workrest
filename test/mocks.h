#ifndef MOCKS_H
#define MOCKS_H

// Test doubles for the two interfaces the core (rest.c) talks to: platform.h and view.h.
//
// The core reaches the OS only through platform.h and the UI only through view.h, and holds
// no #ifdef of its own, so replacing both with mocks links a pure, deterministic copy of the
// state machine -- no GTK/SDL, no real time, no threads. mock_platform.c drives a virtual
// clock the test advances by hand; mock_view.c records every view_* call for inspection.

// =========================
// View event recording (mock_view.c)
// =========================
typedef enum {
    EV_NONE = 0,
    EV_INIT,        // view_init
    EV_REST_BEGIN,  // view_rest_begin(seconds)  -> value = seconds
    EV_TICK,        // view_tick(seconds)        -> value = seconds
    EV_WORK_BEGIN,  // view_work_begin
    EV_DESTROY      // view_destroy
} EvType;

typedef struct {
    EvType type;
    int    value; // seconds for REST_BEGIN / TICK, 0 otherwise
} ViewEvent;

void      mock_view_reset(void);           // Drop all recorded events
int       mock_view_count(void);           // Number of events recorded since the last reset
ViewEvent mock_view_at(int i);             // Event i (0-based); {EV_NONE,0} if out of range
ViewEvent mock_view_last(void);            // Most recent event; {EV_NONE,0} if none
int       mock_view_count_type(EvType t);  // How many recorded events have this type

// =========================
// Platform clock + control (mock_platform.c)
// =========================
// Advance the virtual clock by `ms`, firing every timer that comes due along the way in
// deadline order -- the same firing semantics as the real poll backend, so a repeating 1s
// timer advanced by 5000ms fires exactly five times.
void      mock_advance(long long ms);
long long mock_now_ms(void);      // Current virtual time
int       mock_quit_called(void); // Whether plat_quit has been called (reset by plat_init)
int       mock_timer_count(void); // Live timers right now (leak / cleanup checks)
void      mock_unlock(void);      // Fire the registered unlock handler (as a session unlock would)

#endif // MOCKS_H
