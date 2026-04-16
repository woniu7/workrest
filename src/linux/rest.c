#include "../rest.h"
#include <gtk/gtk.h>
#include <glib.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *label;
    int remaining_seconds;
    gboolean is_resting; // TRUE = 休息中, FALSE = 工作中
    guint timer_id;      // 定时器 ID
} AppContext;

// 前向声明
static void start_work(AppContext *app);
static void start_rest(AppContext *app, int seconds);
static gboolean on_timer_tick(gpointer user_data);

// 停止当前定时器
static void stop_timer(AppContext *app) {
    if (app->timer_id > 0) {
        g_source_remove(app->timer_id);
        app->timer_id = 0;
    }
}

// 键盘按键回调 (对应 WM_KEYDOWN)
static gboolean on_key_pressed(GtkEventControllerKey *controller,
                               guint keyval, guint keycode,
                               GdkModifierType state,
                               gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    switch (keyval) {
        case GDK_KEY_q:
        case GDK_KEY_Q:
            // 退出程序
            g_application_quit(G_APPLICATION(gtk_window_get_application(GTK_WINDOW(app->window))));
            return TRUE;
        case GDK_KEY_r:
        case GDK_KEY_R:
            // 推迟 2 分钟 (进入工作模式 120秒)
            start_work(app);
            // 这是一个特殊的 Work 状态，我们需要手动覆盖定时器逻辑
            // 但为了简单复刻原逻辑，这里直接重置为 120s 后触发休息
            stop_timer(app);
            // 设置 120s 后触发休息的定时器 (单次触发)
            app->timer_id = g_timeout_add_seconds(120, (GSourceFunc)on_timer_tick, app);
            app->is_resting = FALSE; // 标记为工作中
            return TRUE;

    	case GDK_KEY_c:
        case GDK_KEY_C:
            // C 立即继续工作
            // 停止当前倒计时/休息状态，并开始主工作计时
            start_work(app);
            return TRUE;
    }
    return FALSE;
}

// 更新 UI 文本 (对应 WM_PAINT 中的 DrawText)
static void update_label(AppContext *app) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", app->remaining_seconds);
    
    // 使用 Pango Markup 设置大字体和白色
    char *markup = g_strdup_printf("<span font='220' weight='bold' foreground='white'>%s</span>", buffer);
    gtk_label_set_markup(GTK_LABEL(app->label), markup);
    g_free(markup);
}

// 定时器回调 (对应 WM_TIMER)
static gboolean on_timer_tick(gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    if (app->is_resting) {
        // --- 休息倒计时模式 ---
        app->remaining_seconds--;
        update_label(app);

        if (app->remaining_seconds <= 0) {
            start_work(app);
            return G_SOURCE_REMOVE; // 停止当前定时器
        }
        return G_SOURCE_CONTINUE; // 继续定时器
    } else {
        // --- 工作模式结束 ---
        // 在 GTK 中，通常用单次定时器模拟 "Sleep"，
        // 这里如果是通过 Work 定时器触发进来的，说明工作时间到了
        start_rest(app, BREAK_SECONDS);
        return G_SOURCE_REMOVE;
    }
}

// 开始休息 (显示窗口)
static void start_rest(AppContext *app, int seconds) {
    stop_timer(app);
    app->is_resting = TRUE;
    app->remaining_seconds = seconds;

    // 显示并全屏 (对应 ShowWindow SW_SHOWMAXIMIZED 和 HWND_TOPMOST)
    gtk_widget_set_visible(app->window, TRUE);
    gtk_window_fullscreen(GTK_WINDOW(app->window));
    gtk_window_present(GTK_WINDOW(app->window)); // 获取焦点

    update_label(app);
    // 启动 1秒 定时器
    app->timer_id = g_timeout_add(1000, on_timer_tick, app);
}

// 开始工作 (隐藏窗口)
static void start_work(AppContext *app) {
    stop_timer(app);
    app->is_resting = FALSE;

    // 隐藏窗口 (对应 ShowWindow SW_HIDE)
    gtk_widget_set_visible(app->window, FALSE);

    // 启动工作时长定时器 (seconds * 1000)
    // WORK_SECONDS 后调用回调，回调中会检测到 is_resting == FALSE，从而触发休息
    app->timer_id = g_timeout_add_seconds(WORK_SECONDS, on_timer_tick, app);
}

// 应用程序启动回调 (对应 WM_CREATE)
static void activate(GtkApplication *app_instance, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    // 创建窗口
    app->window = gtk_application_window_new(app_instance);
    gtk_window_set_title(GTK_WINDOW(app->window), "Have a break");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 600);

    // 设置黑色背景 CSS (对应 hbrBackground = BLACK_BRUSH)
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, "window { background-color: red; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // 创建 Label
    app->label = gtk_label_new(NULL);
    gtk_window_set_child(GTK_WINDOW(app->window), app->label);

    // 绑定按键事件
    GtkEventController *controller = gtk_event_controller_key_new();
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_pressed), app);
    gtk_widget_add_controller(app->window, controller);

    // 初始状态：先休息 initCount 秒
    start_rest(app, INIT_SECONDS);
}

int app_main(int argc, char **argv) {
    GtkApplication *app_instance;
    int status;
    AppContext app_ctx = {0};

    // 初始化 GTK 应用
    app_instance = gtk_application_new("com.example.haveabreak", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app_instance, "activate", G_CALLBACK(activate), &app_ctx);

    status = g_application_run(G_APPLICATION(app_instance), argc, argv);
    g_object_unref(app_instance);

    return status;
}
