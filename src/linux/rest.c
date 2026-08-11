#include "../rest.h"
#include "../view.h"
#include "../keyboard.h"
#include <glib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// 核心状态机(不依赖任何视图，仅用 GLib 主循环与定时器)
struct RestCore {
    Options    opts;
    GMainLoop *loop;
    int        remaining_seconds;
    gboolean   is_resting;    // TRUE = 休息中, FALSE = 工作中
    gboolean   keys_enabled;  // FALSE = 延迟期内忽略按键
    guint      timer_id;      // 倒计时/工作定时器
    guint      enable_keys_id;// 延迟启用按键的定时器
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

// --- 定时器管理 ---
static void stop_timers(RestCore *c) {
    if (c->timer_id > 0) {
        g_source_remove(c->timer_id);
        c->timer_id = 0;
    }
    if (c->enable_keys_id > 0) {
        g_source_remove(c->enable_keys_id);
        c->enable_keys_id = 0;
    }
}

// 延迟结束后启用按键
static gboolean enable_keys_cb(gpointer d) {
    RestCore *c = (RestCore *)d;
    c->keys_enabled = TRUE;
    c->enable_keys_id = 0;
    rest_log(c, "按键已启用");
    return G_SOURCE_REMOVE; // 单次触发
}

static void start_rest(RestCore *c, int seconds);
static void start_work(RestCore *c, int seconds);

// 定时器回调
static gboolean on_tick(gpointer d) {
    RestCore *c = (RestCore *)d;

    if (c->is_resting) {
        c->remaining_seconds--;
        notify_tick(c, c->remaining_seconds);
        if (c->remaining_seconds <= 0) {
            start_work(c, WORK_SECONDS);
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    } else {
        start_rest(c, BREAK_SECONDS);
        return G_SOURCE_REMOVE;
    }
}

// 开始休息(显示界面并倒计时)
static void start_rest(RestCore *c, int seconds) {
    stop_timers(c);
    c->is_resting = TRUE;
    c->remaining_seconds = seconds;

    // 倒计时开始，先禁用按键，DELAY_SECONDS 秒后再启用
    c->keys_enabled = FALSE;
    c->enable_keys_id = g_timeout_add_seconds(DELAY_SECONDS, enable_keys_cb, c);

    notify_rest_begin(c, seconds);
    c->timer_id = g_timeout_add(1000, on_tick, c);
}

// 开始工作(隐藏界面，seconds 秒后进入休息)
static void start_work(RestCore *c, int seconds) {
    stop_timers(c);
    c->is_resting = FALSE;
    notify_work_begin(c);
    c->timer_id = g_timeout_add_seconds(seconds, on_tick, c);
}

// 按键分发(按键语义以当前 UI 为准：q/r/c/l)
static void dispatch_key(RestCore *c, char key) {
    // 倒计时开始后的 DELAY_SECONDS 秒内忽略按键，防止误触
    if (!c->keys_enabled) return;

    switch (key) {
        case 'q':
        case 'Q':
            rest_log(c, "按键 q：退出");
            g_main_loop_quit(c->loop);
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

// =========================================================
// 公共 API
// =========================================================
RestCore *rest_core_new(const Options *opts) {
    RestCore *c = g_new0(RestCore, 1);
    c->opts = *opts;
    c->loop = g_main_loop_new(NULL, FALSE);
    return c;
}

void rest_core_free(RestCore *c) {
    if (!c) return;
    stop_timers(c);
    if (c->loop) g_main_loop_unref(c->loop);
    g_free(c);
}

void rest_core_start(RestCore *c) {
    rest_log(c, "启动：初始休息 %d 秒", INIT_SECONDS);
    start_rest(c, INIT_SECONDS);
}

void rest_core_run(RestCore *c) {
    g_main_loop_run(c->loop);
}

// stdin 回调在主循环线程上执行，可直接分发
void rest_core_send_key(RestCore *c, char key) {
    dispatch_key(c, key);
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
    keyboard_start(core);     // 终端键盘监听(GUI 与终端模式都启用)
    rest_core_start(core);
    rest_core_run(core);
    view_destroy();
    rest_core_free(core);
    return 0;
}
