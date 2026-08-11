#ifndef TERMINAL_H
#define TERMINAL_H

#include <glib.h>

// 终端按键回调：每读到一个按键字符时被调用
typedef void (*TerminalKeyCallback)(char key, gpointer user_data);

// 开始监听终端(stdin)按键。
// 将终端设为非规范模式(无需回车、不回显)，每读到一个字符就调用 callback。
// 程序退出时自动恢复终端原有设置。
void terminal_start_listen(TerminalKeyCallback callback, gpointer user_data);

#endif // TERMINAL_H
