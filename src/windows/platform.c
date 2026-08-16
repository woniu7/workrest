// Platform layer, Win32 implementation (see ../platform.h).
#include "../platform.h"
#include <windows.h>
#include <wtsapi32.h>
#include <time.h>

#define WM_APP_KEY (WM_APP + 1) // Deliver a key on the loop thread: wParam = character

static HWND g_hwnd = NULL;      // Hidden window: carries key posts and session notifications

static PlatKeyFunc    g_key_fn      = NULL;
static void          *g_key_user    = NULL;
static PlatUnlockFunc g_unlock_fn   = NULL;
static void          *g_unlock_user = NULL;

// =========================
// Timers
// =========================
// SetTimer with a NULL window posts WM_TIMER to the thread queue, and DispatchMessage then
// calls the TIMERPROC below. That proc only receives the timer id, so the callback and its
// user pointer are parked in this small table and looked up by id.
#define MAX_TIMERS 8

typedef struct {
    UINT_PTR      id; // 0 = free slot
    PlatTimerFunc fn;
    void         *user;
    int           repeat;
} TimerSlot;

static TimerSlot g_timers[MAX_TIMERS];

static TimerSlot *find_slot(UINT_PTR id) {
    int i;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (g_timers[i].id == id) return &g_timers[i];
    }
    return NULL;
}

static void CALLBACK timer_proc(HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) {
    TimerSlot    *s;
    PlatTimerFunc fn;
    void         *user;

    (void)hwnd; (void)msg; (void)tick;

    s = find_slot(id);
    if (!s) return;
    fn   = s->fn;
    user = s->user;

    // Retire a one-shot before running its callback: the callback may schedule new timers
    // (start_work does), and it must not find this slot still occupied.
    if (!s->repeat) {
        KillTimer(NULL, id);
        s->id = 0;
    }

    // Nothing touches `s` past this point -- the callback is free to delete this very timer.
    fn(user);
}

PlatTimer plat_timer_add(unsigned ms, int repeat, PlatTimerFunc fn, void *user) {
    int i;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (g_timers[i].id == 0) {
            UINT_PTR id = SetTimer(NULL, 0, ms, timer_proc);
            if (id == 0) return PLAT_TIMER_NONE;
            g_timers[i].id     = id;
            g_timers[i].fn     = fn;
            g_timers[i].user   = user;
            g_timers[i].repeat = repeat;
            return (PlatTimer)id;
        }
    }
    return PLAT_TIMER_NONE; // At most three timers are ever live, so the table cannot fill up
}

PlatTimer plat_timer_add_seconds(unsigned sec, int repeat, PlatTimerFunc fn, void *user) {
    // No coalescing story on Win32 the way GLib has one; seconds are just milliseconds here
    return plat_timer_add(sec * 1000, repeat, fn, user);
}

void plat_timer_del(PlatTimer t) {
    TimerSlot *s;
    if (t == PLAT_TIMER_NONE) return;
    KillTimer(NULL, (UINT_PTR)t);
    s = find_slot((UINT_PTR)t);
    if (s) s->id = 0;
}

// =========================
// Main loop
// =========================
static LRESULT CALLBACK plat_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_APP_KEY:
            if (g_key_fn) g_key_fn(g_key_user, (char)wp);
            return 0;

        case WM_WTSSESSION_CHANGE:
            if (wp == WTS_SESSION_UNLOCK && g_unlock_fn) g_unlock_fn(g_unlock_user);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void plat_init(void) {
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = plat_wndproc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("RestPlatformWindow");
    RegisterClass(&wc);

    // Never shown; it exists so the thread has a window to receive posted keys and session
    // notifications on
    g_hwnd = CreateWindow(TEXT("RestPlatformWindow"), TEXT("rest-platform"),
                          WS_OVERLAPPED, 0, 0, 0, 0,
                          NULL, NULL, GetModuleHandle(NULL), NULL);

    WTSRegisterSessionNotification(g_hwnd, NOTIFY_FOR_THIS_SESSION);
}

void plat_shutdown(void) {
    if (g_hwnd) {
        WTSUnRegisterSessionNotification(g_hwnd);
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }
}

void plat_run(void) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void plat_quit(void) {
    PostQuitMessage(0);
}

// =========================
// Key delivery
// =========================
void plat_set_key_handler(PlatKeyFunc fn, void *user) {
    g_key_fn   = fn;
    g_key_user = user;
}

// Thread-safe: the console listener runs on its own thread, so the key is posted to the
// hidden window and dispatched on the loop thread.
void plat_post_key(char key) {
    if (g_hwnd) PostMessage(g_hwnd, WM_APP_KEY, (WPARAM)(unsigned char)key, 0);
}

// =========================
// Session unlock
// =========================
void plat_set_unlock_handler(PlatUnlockFunc fn, void *user) {
    g_unlock_fn   = fn;
    g_unlock_user = user;
}

// =========================
// Misc
// =========================
void plat_format_time(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_s(&tm_now, &now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_now);
}
