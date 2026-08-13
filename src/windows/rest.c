#include "../rest.h"
#include "../view.h"
#include "../keyboard.h"
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ID_TIMER_REST        1  // Rest countdown (per second)
#define ID_TIMER_WORK        2  // Work timer (one-shot)
#define ID_TIMER_ENABLE_KEYS 3  // Delayed key enabling (one-shot)
#define WM_APP_KEY (WM_APP + 1) // Feed in a key: wParam = character

// Core state machine (independent of any visible UI; a hidden message window carries the timers and messages)
struct RestCore {
    Options opts;
    HWND    hwnd;           // Hidden message window
    int     remaining_seconds;
    BOOL    is_resting;     // TRUE = resting, FALSE = working
    BOOL    keys_enabled;   // FALSE = ignore keys during the delay period
};

// --- Debug log (--debug, enabled by default), written to stderr ---
static void rest_log(RestCore *c, const char *fmt, ...) {
    va_list ap;
    if (!c->opts.debug) return;
    va_start(ap, fmt);
    char ts[20];
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_s(&tm_now, &now);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now);
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

// --- State machine (forward declarations) ---
static void start_rest(RestCore *c, int seconds);
static void start_work(RestCore *c, int seconds);

// Fires every second during the rest countdown
static void core_tick(RestCore *c) {
    c->remaining_seconds--;
    notify_tick(c, c->remaining_seconds);
    if (c->remaining_seconds <= 0) {
        start_work(c, WORK_SECONDS);
    }
}

// Start resting (show the UI and count down)
static void start_rest(RestCore *c, int seconds) {
    KillTimer(c->hwnd, ID_TIMER_WORK);
    KillTimer(c->hwnd, ID_TIMER_REST);
    KillTimer(c->hwnd, ID_TIMER_ENABLE_KEYS);

    c->is_resting = TRUE;
    c->remaining_seconds = seconds;

    // Countdown starts: disable keys first, enable them after DELAY_SECONDS seconds
    c->keys_enabled = FALSE;
    SetTimer(c->hwnd, ID_TIMER_ENABLE_KEYS, DELAY_SECONDS * 1000, NULL);

    notify_rest_begin(c, seconds);
    SetTimer(c->hwnd, ID_TIMER_REST, 1000, NULL);
}

// Start working (hide the UI, enter rest after `seconds` seconds)
static void start_work(RestCore *c, int seconds) {
    KillTimer(c->hwnd, ID_TIMER_REST);
    KillTimer(c->hwnd, ID_TIMER_WORK);
    KillTimer(c->hwnd, ID_TIMER_ENABLE_KEYS);

    c->is_resting = FALSE;
    notify_work_begin(c);
    SetTimer(c->hwnd, ID_TIMER_WORK, seconds * 1000, NULL);
}

// Key dispatch (key semantics follow the current UI: q/r/c/l)
static void dispatch_key(RestCore *c, char key) {
    // Ignore keys during the first DELAY_SECONDS seconds after the countdown starts, to prevent accidental presses
    if (!c->keys_enabled) return;

    switch (key) {
        case 'q':
        case 'Q':
            rest_log(c, "Key q: quit");
            PostQuitMessage(0);
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

// Window procedure of the hidden message window: carries timers, key delivery, session changes
static LRESULT CALLBACK core_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    RestCore *c = (RestCore *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_TIMER:
            if (!c) break;
            if (wp == ID_TIMER_REST) {
                core_tick(c);
            } else if (wp == ID_TIMER_WORK) {
                start_rest(c, BREAK_SECONDS);
            } else if (wp == ID_TIMER_ENABLE_KEYS) {
                c->keys_enabled = TRUE;
                KillTimer(hwnd, ID_TIMER_ENABLE_KEYS);
                rest_log(c, "Keys enabled");
            }
            return 0;

        case WM_APP_KEY:
            if (c) dispatch_key(c, (char)wp);
            return 0;

        case WM_WTSSESSION_CHANGE:
            if (c && wp == WTS_SESSION_UNLOCK) {
                rest_log(c, "Session unlocked, re-entering rest");
                start_rest(c, INIT_SECONDS);
            }
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// Register the hidden window class (only needed once)
static void register_core_class(void) {
    static BOOL registered = FALSE;
    WNDCLASS wc;
    if (registered) return;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = core_wndproc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("RestCoreWindow");
    RegisterClass(&wc);
    registered = TRUE;
}

// =========================================================
// Public API
// =========================================================
RestCore *rest_core_new(const Options *opts) {
    RestCore *c = (RestCore *)calloc(1, sizeof(RestCore));
    c->opts = *opts;

    register_core_class();
    // Create a non-visible ordinary window to carry timers and messages (reliably receives WM_TIMER / session notifications)
    c->hwnd = CreateWindow(TEXT("RestCoreWindow"), TEXT("rest-core"),
                           WS_OVERLAPPED, 0, 0, 0, 0,
                           NULL, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(c->hwnd, GWLP_USERDATA, (LONG_PTR)c);

    // Session unlock notification (re-enter rest after unlocking the lock screen)
    WTSRegisterSessionNotification(c->hwnd, NOTIFY_FOR_THIS_SESSION);
    return c;
}

void rest_core_free(RestCore *c) {
    if (!c) return;
    if (c->hwnd) {
        KillTimer(c->hwnd, ID_TIMER_REST);
        KillTimer(c->hwnd, ID_TIMER_WORK);
        KillTimer(c->hwnd, ID_TIMER_ENABLE_KEYS);
        WTSUnRegisterSessionNotification(c->hwnd);
        DestroyWindow(c->hwnd);
    }
    free(c);
}

void rest_core_start(RestCore *c) {
    rest_log(c, "Startup: initial rest %d s", INIT_SECONDS);
    start_rest(c, INIT_SECONDS);
}

void rest_core_run(RestCore *c) {
    MSG msg;
    (void)c;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Thread-safe: post to the message window, dispatch on the main loop thread
void rest_core_send_key(RestCore *c, char key) {
    PostMessage(c->hwnd, WM_APP_KEY, (WPARAM)(unsigned char)key, 0);
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
    keyboard_start(core);     // Console keyboard listener (enabled in both GUI and CLI modes)
    rest_core_start(core);
    rest_core_run(core);
    view_destroy();
    rest_core_free(core);
    return 0;
}
