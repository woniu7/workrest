#ifndef TERMINAL_H
#define TERMINAL_H

// 终端按键回调：每读到一个按键时被调用。
// 注意：回调在独立的监听线程中执行，若要操作 GUI 请用 PostMessage 投递到窗口线程。
typedef void (*TerminalKeyCallback)(char key, void *user_data);

// 启动终端(控制台)按键监听线程。每读到一个字符就调用 callback。
void terminal_start_listen(TerminalKeyCallback callback, void *user_data);

#endif // TERMINAL_H
