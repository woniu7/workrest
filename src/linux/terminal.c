#include "terminal.h"
#include <glib-unix.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

// 终端原始设置，用于退出时恢复
static struct termios g_orig_termios;
static gboolean g_termios_saved = FALSE;

// 按键回调及其用户数据
static TerminalKeyCallback g_callback = NULL;
static gpointer g_user_data = NULL;

// 退出时恢复终端设置
static void restore_terminal(void) {
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_termios_saved = FALSE;
    }
}

// 把终端设为非规范模式：无需回车、不回显，即时读取按键
static void setup_terminal(void) {
    struct termios raw;
    if (!isatty(STDIN_FILENO)) return; // 非终端(如管道)则不处理
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0) return;
    g_termios_saved = TRUE;
    atexit(restore_terminal);

    raw = g_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // 关闭行缓冲与回显，保留 ISIG(Ctrl-C 仍有效)
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// stdin 有输入时的回调
static gboolean on_stdin(gint fd, GIOCondition condition, gpointer user_data) {
    char c;
    ssize_t n;

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE; // 终端关闭，停止监听
    }

    n = read(fd, &c, 1);
    if (n <= 0) {
        return G_SOURCE_REMOVE;
    }

    if (g_callback) {
        g_callback(c, user_data);
    }
    return G_SOURCE_CONTINUE;
}

void terminal_start_listen(TerminalKeyCallback callback, gpointer user_data) {
    g_callback = callback;
    g_user_data = user_data;

    setup_terminal();
    g_unix_fd_add(STDIN_FILENO, G_IO_IN | G_IO_HUP | G_IO_ERR, on_stdin, user_data);
}
