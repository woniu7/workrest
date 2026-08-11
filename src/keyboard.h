#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "rest.h"  // RestCore

// 监听终端/控制台键盘，读到的按键通过 rest_core_send_key 送入核心。
// 与视图无关：GUI 与终端两种构建都会链接并启用它，所以 GUI 版本也能响应终端按键。
void keyboard_start(RestCore *core);

#endif // KEYBOARD_H
