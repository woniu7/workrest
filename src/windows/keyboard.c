// 控制台键盘监听：独立线程读键并送入核心。
// 与视图无关，GUI/终端两种构建都会链接。
#include "../keyboard.h"
#include "../rest.h"
#include <windows.h>
#include <conio.h>
#include <process.h>
#include <stdint.h>

// 控制台按键监听线程：读到按键就(线程安全地)送入核心
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
