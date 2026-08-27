// Mock view (implements view.h) for the core unit tests: every view_* call the core makes is
// appended to an in-memory log the test then inspects. No window, no drawing -- the point is
// only to observe the transitions the state machine drives (rest begins, ticks, work begins).
#include "view.h"
#include "mocks.h"

#define MAX_EVENTS 8192

static ViewEvent g_events[MAX_EVENTS];
static int       g_count;

static void push(EvType type, int value) {
    if (g_count >= MAX_EVENTS) return; // A runaway test would overflow; drop rather than smash memory
    g_events[g_count].type  = type;
    g_events[g_count].value = value;
    g_count++;
}

// --- view interface implementation ---
void view_init(RestCore *core) { (void)core; push(EV_INIT, 0); }
void view_rest_begin(int seconds) { push(EV_REST_BEGIN, seconds); }
void view_tick(int seconds)       { push(EV_TICK, seconds); }
void view_work_begin(void)        { push(EV_WORK_BEGIN, 0); }
void view_destroy(void)           { push(EV_DESTROY, 0); }

// --- test control (mocks.h) ---
void mock_view_reset(void) { g_count = 0; }
int  mock_view_count(void) { return g_count; }

ViewEvent mock_view_at(int i) {
    ViewEvent none = { EV_NONE, 0 };
    if (i < 0 || i >= g_count) return none;
    return g_events[i];
}

ViewEvent mock_view_last(void) {
    return mock_view_at(g_count - 1);
}

int mock_view_count_type(EvType t) {
    int i, n = 0;
    for (i = 0; i < g_count; i++) if (g_events[i].type == t) n++;
    return n;
}
