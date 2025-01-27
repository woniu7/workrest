#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib") // 显式链接 user32.lib
#pragma comment(lib, "gdi32.lib")  // 显式链接 gdi32.lib

#define ID_TIMER_COUNTDOWN 1  // 10秒倒计时定时器
#define ID_TIMER_WAIT      2  // 20秒等待定时器

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
    static TCHAR szAppName[] = _T("Have a break");
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wndclass;

    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wndclass)) {
        MessageBox(NULL, _T("This program requires Windows NT!"), szAppName, MB_ICONERROR);
        return 0;
    }

    hwnd = CreateWindow(szAppName, _T("Have a break"),
                        WS_POPUP | WS_VISIBLE,
                        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                        NULL, NULL, hInstance, NULL);

    // 将窗口设置为最顶层
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

#define workCount 2400
#define breakCount 300

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static int count = breakCount; // 倒计时计数
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    TCHAR szBuffer[10];
    static HFONT hFont = NULL; // 字体句柄

    switch (message) {
    case WM_CREATE:
        // 创建一个大字体
        hFont = CreateFont(200, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                           VARIABLE_PITCH, _T("Arial"));
        // 启动10秒倒计时定时器
        SetTimer(hwnd, ID_TIMER_COUNTDOWN, 1000, NULL);
        return 0;

    case WM_TIMER:
        if (wParam == ID_TIMER_COUNTDOWN) {
            // 10秒倒计时
            count--;
            if (count <= 0) {
                // 倒计时结束，最小化窗口
                ShowWindow(hwnd, SW_MINIMIZE);
                // 隐藏任务栏图标
                ShowWindow(hwnd, SW_HIDE);
                // 停止倒计时定时器
                KillTimer(hwnd, ID_TIMER_COUNTDOWN);
                // 启动20秒等待定时器
                SetTimer(hwnd, ID_TIMER_WAIT, workCount*1000, NULL);
            }
            InvalidateRect(hwnd, NULL, TRUE); // 触发重绘
        } else if (wParam == ID_TIMER_WAIT) {
            // 20秒等待结束，最大化窗口并重新开始倒计时
            ShowWindow(hwnd, SW_MAXIMIZE);
            ShowWindow(hwnd, SW_SHOW); // 显示窗口
            count = breakCount; // 重置倒计时
            // 停止等待定时器
            KillTimer(hwnd, ID_TIMER_WAIT);
            // 启动10秒倒计时定时器
            SetTimer(hwnd, ID_TIMER_COUNTDOWN, 1000, NULL);
        }
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);

        // 选择字体
        if (hFont) {
            SelectObject(hdc, hFont);
        }

        // 设置文本颜色为白色
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT); // 背景透明

        // 绘制倒计时数字
        wsprintf(szBuffer, _T("%d"), count);
        DrawText(hdc, szBuffer, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        EndPaint(hwnd, &ps);
        return 0;

    case WM_SYSCOMMAND:
        // 拦截系统命令，防止窗口被恢复
        if (wParam == SC_RESTORE || wParam == SC_MAXIMIZE) {
            return 0; // 阻止窗口恢复或最大化
        }
        break;

    case WM_DESTROY:
        if (hFont) {
            DeleteObject(hFont); // 删除字体对象
        }
        // 停止所有定时器
        KillTimer(hwnd, ID_TIMER_COUNTDOWN);
        KillTimer(hwnd, ID_TIMER_WAIT);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
