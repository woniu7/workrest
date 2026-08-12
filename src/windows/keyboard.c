// Console keyboard listener: a dedicated thread reads keys and feeds them into the core.
// View-independent; linked by both the GUI and CLI builds.
#include "../keyboard.h"
#include "../rest.h"
#include <windows.h>
#include <conio.h>
#include <process.h>
#include <stdint.h>

// Console key listener thread: on each key read, (thread-safely) feed it into the core
static unsigned __stdcall console_input_thread(void *arg) {
    RestCore *core = (RestCore *)arg;
    while (1) {
        int ch = _getch();
        rest_core_send_key(core, (char)ch);
    }
    return 0;
}

void keyboard_start(RestCore *core) {
    uintptr_t th = _beginthreadex(NULL, 0, console_input_thread, core, 0, NULL);
    if (th != 0) {
        CloseHandle((HANDLE)th);
    }
}
