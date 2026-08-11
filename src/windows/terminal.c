// 终端视图实现(无 GUI)：把倒计时打印到终端模拟显示。
// 键盘监听已独立到 keyboard.c，由核心统一启动，这里只负责显示。
#include "../view.h"
#include "../rest.h"
#include <stdio.h>

void view_init(RestCore *core) {
    (void)core; // 输入由 keyboard.c 负责，这里无需处理
}

void view_rest_begin(int seconds) {
    printf("\n===== 休息开始，倒计时 %d 秒（q 退出 / r 推迟 / c 继续工作 / l 置 999999 / b 重置）=====\n", seconds);
    fflush(stdout);
}

void view_tick(int seconds) {
    printf("\r  倒计时: %-8d", seconds); // \r 原地刷新，模拟大数字显示
    fflush(stdout);
}

void view_work_begin(void) {
    printf("\n===== 进入工作模式，界面隐藏 =====\n");
    fflush(stdout);
}

void view_destroy(void) {
}
