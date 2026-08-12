// Terminal (stdin) keyboard listener: puts the terminal into non-canonical mode, reads char by char and feeds the core.
// View-independent; linked by both the GUI and CLI builds.
#include "../keyboard.h"
#include "../rest.h"
#include <glib.h>
#include <glib-unix.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

// Original terminal settings, used to restore on exit
static struct termios g_orig_termios;
static gboolean g_termios_saved = FALSE;

// Restore terminal settings on exit
static void restore_terminal(void) {
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_termios_saved = FALSE;
    }
}

// Put the terminal into non-canonical mode: no Enter needed, no echo, read keys instantly
static void setup_terminal(void) {
    struct termios raw;
    if (!isatty(STDIN_FILENO)) return; // Skip if not a terminal (e.g. a pipe)
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0) return;
    g_termios_saved = TRUE;
    atexit(restore_terminal);

    raw = g_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable line buffering and echo, keep ISIG (Ctrl-C still works)
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// Callback when stdin has input: feed the character into the core
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
