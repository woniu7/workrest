#include "terminal.h"
#include <windows.h>
#include <conio.h>
#include <process.h>

// 按键回调及其用户数据
static TerminalKeyCallback g_callback = NULL;
static void *g_user_data = NULL;

// 终端(控制台)按键监听线程：每读到一个按键就回调
static unsigned __stdcall console_input_thread(void *arg) {
    (void)arg;
    while (1) {
        int ch = _getch();
        if (g_callback) {
            g_callback((char)ch, g_user_data);
        }
    }
    return 0;
}

void terminal_start_listen(TerminalKeyCallback callback, void *user_data) {
    uintptr_t th;

    g_callback = callback;
    g_user_data = user_data;

    th = _beginthreadex(NULL, 0, console_input_thread, NULL, 0, NULL);
    if (th != 0) {
        CloseHandle((HANDLE)th);
    }
}
