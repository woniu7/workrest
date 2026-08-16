// Terminal (stdin) keyboard listener: puts the terminal into non-canonical mode, reads char by
// char and feeds the core. View-independent; linked by every build.
// Uses only libc plus loop.h, so it adds no dependency of its own to whichever backend is in use.
#include "../keyboard.h"
#include "../rest.h"
#include "loop.h"
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

// Original terminal settings, used to restore on exit
static struct termios g_orig_termios;
static int            g_termios_saved = 0;

// Restore terminal settings on exit
static void restore_terminal(void) {
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_termios_saved = 0;
    }
}

// Put the terminal into non-canonical mode: no Enter needed, no echo, read keys instantly
static void setup_terminal(void) {
    struct termios raw;
    if (!isatty(STDIN_FILENO)) return; // Skip if not a terminal (e.g. a pipe)
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0) return;
    g_termios_saved = 1;
    atexit(restore_terminal);

    raw = g_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable line buffering and echo, keep ISIG (Ctrl-C still works)
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// stdin has input: feed the character into the core. Returning 0 drops the watch.
static int on_stdin(void *user) {
    RestCore *core = (RestCore *)user;
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);

    if (n <= 0) return 0; // EOF or error
    rest_core_send_key(core, c);
    return 1;
}

void keyboard_start(RestCore *core) {
    setup_terminal();
    loop_watch_fd(STDIN_FILENO, on_stdin, core);
}
