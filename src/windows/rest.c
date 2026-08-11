#include "../rest.h"
#include "../view.h"
#include "../keyboard.h"
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define ID_TIMER_REST        1  // 休息倒计时(每秒)
#define ID_TIMER_WORK        2  // 工作计时(单次)
#define ID_TIMER_ENABLE_KEYS 3  // 延迟启用按键(单次)
#define WM_APP_KEY (WM_APP + 1) // 送入按键：wParam = 字符

// 核心状态机(不依赖可见 UI；用隐藏消息窗口承载定时器与消息)
struct RestCore {
    Options opts;
    HWND    hwnd;           // 隐藏消息窗口
    int     remaining_seconds;
    BOOL    is_resting;     // TRUE = 休息中, FALSE = 工作中
    BOOL    keys_enabled;   // FALSE = 延迟期内忽略按键
};

// --- 调试日志(--debug，默认开启)，输出到 stderr ---
static void rest_log(RestCore *c, const char *fmt, ...) {
    va_list ap;
    if (!c->opts.debug) return;
    va_start(ap, fmt);
    fprintf(stderr, "[debug] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
}

// --- 通知视图(直接调用 view_*，链接期已确定是哪一种实现) ---
static void notify_rest_begin(RestCore *c, int seconds) {
    rest_log(c, "进入休息，倒计时 %d 秒", seconds);
    view_rest_begin(seconds);
}
static void notify_tick(RestCore *c, int seconds) {
    view_tick(seconds);
}
static void notify_work_begin(RestCore *c) {
    rest_log(c, "进入工作模式");
    view_work_begin();
}

// --- 状态机(前向声明) ---
static void start_rest(RestCore *c, int seconds);
static void start_work(RestCore *c, int seconds);

// 休息倒计时每秒触发
static void core_tick(RestCore *c) {
    c->remaining_seconds--;
    notify_tick(c, c->remaining_seconds);
    if (c->remaining_seconds <= 0) {
        start_work(c, WORK_SECONDS);
    }
}

// 开始休息(显示界面并倒计时)
static void start_rest(RestCore *c, int seconds) {
    KillTimer(c->hwnd, ID_TIMER_WORK);
    KillTimer(c->hwnd, ID_TIMER_REST);
    KillTimer(c->hwnd, ID_TIMER_ENABLE_KEYS);

    c->is_resting = TRUE;
    c->remaining_seconds = seconds;

    // 倒计时开始，先禁用按键，DELAY_SECONDS 秒后再启用
    c->keys_enabled = FALSE;
    SetTimer(c->hwnd, ID_TIMER_ENABLE_KEYS, DELAY_SECONDS * 1000, NULL);

    notify_rest_begin(c, seconds);
    SetTimer(c->hwnd, ID_TIMER_REST, 1000, NULL);
}

// 开始工作(隐藏界面，seconds 秒后进入休息)
static void start_work(RestCore *c, int seconds) {
    KillTimer(c->hwnd, ID_TIMER_REST);
    KillTimer(c->hwnd, ID_TIMER_WORK);
    KillTimer(c->hwnd, ID_TIMER_ENABLE_KEYS);

    c->is_resting = FALSE;
    notify_work_begin(c);
    SetTimer(c->hwnd, ID_TIMER_WORK, seconds * 1000, NULL);
}

// 按键分发(按键语义以当前 UI 为准：q/r/c/l)
static void dispatch_key(RestCore *c, char key) {
    // 倒计时开始后的 DELAY_SECONDS 秒内忽略按键，防止误触
    if (!c->keys_enabled) return;

    switch (key) {
        case 'q':
        case 'Q':
            rest_log(c, "按键 q：退出");
            PostQuitMessage(0);
            break;
        case 'r':
        case 'R':
            rest_log(c, "按键 r：推迟工作 %d 秒", POSTPONE_SECONDS);
            start_work(c, POSTPONE_SECONDS);
            break;
        case 'c':
        case 'C':
            rest_log(c, "按键 c：立即继续工作");
            start_work(c, WORK_SECONDS);
            break;
        case 'l':
        case 'L':
            rest_log(c, "按键 l：倒计时设为 999999");
            c->remaining_seconds = 999999;
            notify_tick(c, c->remaining_seconds);
            break;
        case 'b':
        case 'B':
            // 立即触发休息倒计时；若已在倒计时中则重置为 BREAK_SECONDS
            rest_log(c, "按键 b：触发/重置倒计时 %d 秒", BREAK_SECONDS);
            start_rest(c, BREAK_SECONDS);
            break;
        default:
            break;
    }
}

// 隐藏消息窗口的窗口过程：承载定时器、按键投递、会话变化
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
                rest_log(c, "按键已启用");
            }
            return 0;

        case WM_APP_KEY:
            if (c) dispatch_key(c, (char)wp);
            return 0;

        case WM_WTSSESSION_CHANGE:
            if (c && wp == WTS_SESSION_UNLOCK) {
                rest_log(c, "会话解锁，重新进入休息");
                start_rest(c, INIT_SECONDS);
            }
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// 注册隐藏窗口类(只需一次)
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
// 公共 API
// =========================================================
RestCore *rest_core_new(const Options *opts) {
    RestCore *c = (RestCore *)calloc(1, sizeof(RestCore));
    c->opts = *opts;

    register_core_class();
    // 创建一个不显示的普通窗口，用于承载定时器与消息(可靠接收 WM_TIMER / 会话通知)
    c->hwnd = CreateWindow(TEXT("RestCoreWindow"), TEXT("rest-core"),
                           WS_OVERLAPPED, 0, 0, 0, 0,
                           NULL, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(c->hwnd, GWLP_USERDATA, (LONG_PTR)c);

    // 会话解锁通知(锁屏解锁后重新休息)
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
    rest_log(c, "启动：初始休息 %d 秒", INIT_SECONDS);
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

// 线程安全：投递到消息窗口，在主循环线程上分发
void rest_core_send_key(RestCore *c, char key) {
    PostMessage(c->hwnd, WM_APP_KEY, (WPARAM)(unsigned char)key, 0);
}

// 解析命令行选项
static void parse_options(int argc, char **argv, Options *opts) {
    int i;
    opts->debug = 1; // 默认开启日志

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            opts->debug = 1;
        } else if (strcmp(argv[i], "--no-debug") == 0) {
            opts->debug = 0;
        } else {
            fprintf(stderr, "未知选项: %s\n", argv[i]);
            fprintf(stderr, "用法: rest [--debug|--no-debug]\n");
        }
    }
}

int app_main(int argc, char **argv) {
    Options opts;
    RestCore *core;

    parse_options(argc, argv, &opts);

    core = rest_core_new(&opts);
    view_init(core);          // 视图(编译期决定：GUI 或终端)
    keyboard_start(core);     // 控制台键盘监听(GUI 与终端模式都启用)
    rest_core_start(core);
    rest_core_run(core);
    view_destroy();
    rest_core_free(core);
    return 0;
}
