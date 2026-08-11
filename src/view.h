#ifndef VIEW_H
#define VIEW_H

#include "rest.h"  // RestCore

// 视图接口(显示 + 输入)：由 ui.c(图形界面) 或 terminal.c(终端模拟) 二选一实现。
// 选择哪一种由编译期链接哪个源文件决定(见 Makefile 的 VIEW 变量)，运行期不做判断。
// 核心 (rest.c) 直接调用这些函数，不持有函数指针、不做 if 分支。
void view_init(RestCore *core);    // 初始化视图与输入(GUI 建窗口 / 终端设置 stdin)；core 用于回传按键
void view_rest_begin(int seconds); // 进入休息：显示界面，给出初始秒数
void view_tick(int seconds);       // 倒计时更新
void view_work_begin(void);        // 进入工作：隐藏界面
void view_destroy(void);           // 销毁视图

#endif // VIEW_H
