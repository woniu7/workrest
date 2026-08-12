// CLI view implementation (no GUI): prints the countdown to the terminal as a simulated display.
// Keyboard listening is separated into keyboard.c and started by the core; this file only handles display.
#include "../view.h"
#include "../rest.h"
#include <stdio.h>

void view_init(RestCore *core) {
    (void)core; // Input is handled by keyboard.c, nothing to do here
}

void view_rest_begin(int seconds) {
    printf("\n===== Rest started, countdown %d s (q quit / r postpone / c continue working / l set 999999 / b reset) =====\n", seconds);
    fflush(stdout);
}

void view_tick(int seconds) {
    printf("\r  Countdown: %-8d", seconds); // \r refreshes in place, simulating a large digit display
    fflush(stdout);
}

void view_work_begin(void) {
    printf("\n===== Entering work mode, UI hidden =====\n");
    fflush(stdout);
}

void view_destroy(void) {
}
