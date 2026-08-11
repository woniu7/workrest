// Win32 视图实现：全屏弹出窗口显示倒计时 + 键盘输入。
#include "../view.h"
#include "../rest.h"
#include <windows.h>
#include <tchar.h>

static RestCore *g_core  = NULL;
static HWND      g_hwnd  = NULL;
static HFONT     g_font  = NULL;
static int       g_count = 0;   // 当前显示的秒数

// 窗口过程：绘制倒计时、把按键转交核心
static LRESULT CALLBACK ui_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rect;
            TCHAR buf[16];

            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            if (g_font) SelectObject(hdc, g_font);
            SetTextColor(hdc, RGB(255, 255, 255)); // 白色
            SetBkMode(hdc, TRANSPARENT);
            wsprintf(buf, TEXT("%d"), g_count);
            DrawText(hdc, buf, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_KEYDOWN:
            // 按键转成字符送入核心(VK 码对字母即大写 ASCII)
            rest_core_send_key(g_core, (char)wp);
            return 0;
        case WM_DESTROY:
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// --- view 接口实现 ---
void view_init(RestCore *core) {
    WNDCLASSEX wc;
    HINSTANCE hInst = GetModuleHandle(NULL);
    const TCHAR *CLASS_NAME = TEXT("HaveABreakWindow");

    g_core = core;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ui_wndproc;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    g_hwnd = CreateWindow(
        CLASS_NAME, TEXT("Have a break"),
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInst, NULL);

    g_font = CreateFont(200, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        VARIABLE_PITCH, TEXT("Arial"));
}

void view_rest_begin(int seconds) {
    g_count = seconds;
    ShowWindow(g_hwnd, SW_SHOWMAXIMIZED);
    SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0,
                 GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                 SWP_SHOWWINDOW);
    SetFocus(g_hwnd);
    InvalidateRect(g_hwnd, NULL, TRUE);
}

void view_tick(int seconds) {
    g_count = seconds;
    InvalidateRect(g_hwnd, NULL, TRUE); // 触发重绘
}

void view_work_begin(void) {
    ShowWindow(g_hwnd, SW_HIDE);
}

void view_destroy(void) {
    if (g_font) { DeleteObject(g_font); g_font = NULL; }
    if (g_hwnd) { DestroyWindow(g_hwnd); g_hwnd = NULL; }
}
