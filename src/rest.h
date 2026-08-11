#ifndef REST_H
#define REST_H

// 配置常量
#define WORK_SECONDS 2700
#define BREAK_SECONDS 180
#define INIT_SECONDS 3
#define DELAY_SECONDS 1
#define POSTPONE_SECONDS 120  // r 键推迟工作的秒数

// 运行选项(由命令行解析)
typedef struct {
    int debug;   // --debug：显示日志(默认开启)
} Options;

// 核心状态机(不透明类型，实现见各平台 rest.c，不依赖任何视图)
typedef struct RestCore RestCore;

RestCore *rest_core_new(const Options *opts);
void rest_core_free(RestCore *core);
void rest_core_start(RestCore *core);   // 进入初始休息，启动状态机
void rest_core_run(RestCore *core);     // 运行主循环，阻塞直到退出

// 线程安全地送入一个按键(视图从任意线程调用)。按键语义以当前 UI 为准：
//   q 退出 / r 推迟 / c 继续工作 / l 倒计时设为 999999
// 倒计时开始后的 DELAY_SECONDS 秒内会忽略按键，防止误触。
void rest_core_send_key(RestCore *core, char key);

int app_main(int argc, char **argv);

#endif // REST_H
