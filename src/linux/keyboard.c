// 终端(stdin)键盘监听：把终端设为非规范模式，逐字符读取并送入核心。
// 与视图无关，GUI/终端两种构建都会链接。
#include "../keyboard.h"
#include "../rest.h"
#include <glib.h>
#include <glib-unix.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

// 终端原始设置，用于退出时恢复
static struct termios g_orig_termios;
static gboolean g_termios_saved = FALSE;

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

// stdin 有输入时的回调：把字符送入核心
static gboolean on_stdin(gint fd, GIOCondition condition, gpointer user_data) {
    RestCore *core = (RestCore *)user_data;
    char c;
    ssize_t n;

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }
    n = read(fd, &c, 1);
    if (n <= 0) {
        return G_SOURCE_REMOVE;
    }
    rest_core_send_key(core, c);
    return G_SOURCE_CONTINUE;
}

void keyboard_start(RestCore *core) {
    setup_terminal();
    g_unix_fd_add(STDIN_FILENO, G_IO_IN | G_IO_HUP | G_IO_ERR, on_stdin, core);
}
