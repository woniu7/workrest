#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <wtsapi32.h>
//#pragma comment(lib, "user32.lib") // 显式链接 user32.lib
//#pragma comment(lib, "gdi32.lib")  // 显式链接 gdi32.lib
//#pragma comment(lib, "wtsapi32.lib")

#define ID_TIMER_REST 1  // 休息倒计时, 用于休息倒计时
#define ID_TIMER_WORK_END 2  // 工作计时器，用于等待工作时间

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int HaveARest(HWND hwnd, int restSecond);
int HaveAWork(HWND hwnd, int workSecond);

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

	//会发送WM_CREATE消息
	hwnd = CreateWindow(szAppName, _T("Have a break"),
			WS_POPUP | WS_VISIBLE,
			0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
			NULL, NULL, hInstance, NULL);

	if (!hwnd) {
		MessageBox(NULL, "CreateWindow failed！", szAppName, MB_ICONERROR);
		return 0;
	}

	// 将窗口设置为最顶层, 只需调用一次,隐藏后再次显示仍保留
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);

	// 注册会话通知, 锁屏用
	// WTSRegisterSessionNotification: 这个函数用于注册会话通知。当会话状态发生变化时（例如锁定或解锁），系统会发送WM_WTSSESSION_CHANGE消息
	if (!WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION)) {
		printf("Failed to register session notification.\n");
		return 1;
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

#define workCount 2700
#define breakCount 300
#define initCount 6

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static HFONT hFont = NULL; // 字体句柄

	switch (message) {
		case WM_CREATE:
			// 创建一个大字体
			hFont = CreateFont(200, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
					OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
					VARIABLE_PITCH, _T("Arial"));

			HaveARest(hwnd, initCount);
			return 0;

		case WM_TIMER:
			if (wParam == ID_TIMER_REST) {
				// restCount秒倒计时
				int count = (int)GetWindowLongPtr(hwnd, GWLP_USERDATA);  
				count--;
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)count);  
				if (count <= 0) {
					//倒计时结束，开始工作
					HaveAWork(hwnd, workCount);
				} else {
					//发送WM_PAINT给hwnd
					InvalidateRect(hwnd, NULL, TRUE); // 触发重绘
				}
			} else if (wParam == ID_TIMER_WORK_END) {
				//工作结束, 开始休息
				HaveARest(hwnd, breakCount);
			}
			return 0;
		case WM_WTSSESSION_CHANGE: 
			if (wParam == WTS_SESSION_UNLOCK) {
				printf("User has unlocked the computer.\n");
				KillTimer(hwnd, ID_TIMER_WORK_END);  // 先取消之前的定时器
				KillTimer(hwnd, ID_TIMER_REST);  // 先取消之前的定时器
				HaveARest(hwnd, initCount);
				return 0;
			}
			break;

		case WM_PAINT:
			HDC hdc;
			PAINTSTRUCT ps;
			RECT rect;
			TCHAR szBuffer[10];
			hdc = BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &rect);

			// 选择字体
			if (hFont) {
				SelectObject(hdc, hFont);
			}

			// 设置文本颜色为白色
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkMode(hdc, TRANSPARENT); // 背景透明

			int count = (int)GetWindowLongPtr(hwnd, GWLP_USERDATA);  
			// 绘制倒计时数字
			wsprintf(szBuffer, _T("%d"), count);
			DrawText(hdc, szBuffer, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

			EndPaint(hwnd, &ps);
			return 0;

		//case WM_SYSCOMMAND:
		//	// 拦截系统命令，防止窗口被恢复
		//	if (wParam == SC_RESTORE || wParam == SC_MAXIMIZE) {
		//		return 0; // 阻止窗口恢复或最大化
		//	}
		//	break;

		case WM_DESTROY:
			if (hFont) {
				DeleteObject(hFont); // 删除字体对象
			}
			// 停止所有定时器
			KillTimer(hwnd, ID_TIMER_REST);
			KillTimer(hwnd, ID_TIMER_WORK_END);
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			switch (wParam)
			{
				// q关闭
				case 0x51:
					PostQuitMessage(0);
					break;
				// r 推迟2分钟
				case 0x52:
					KillTimer(hwnd, ID_TIMER_WORK_END);  // 先取消之前的定时器
					KillTimer(hwnd, ID_TIMER_REST);  // 先取消之前的定时器
					HaveAWork(hwnd, 120);
					break;
				default:
					break;
			}
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}


int HaveARest(HWND hwnd, int restSecond) {
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)restSecond);
	// 等待结束，停止工作定时器
	KillTimer(hwnd, ID_TIMER_WORK_END);
	//显示窗口，并将其最大化。 (窗口变化，会触发WM_PAINT)
	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	SetFocus(hwnd);

	// 启动休息定时器, 开始倒计时
	SetTimer(hwnd, ID_TIMER_REST, 1000, NULL);
	return 0;
}

int HaveAWork(HWND hwnd, int workSecond) {
	// 倒计时结束，停止休息计时器
	KillTimer(hwnd, ID_TIMER_REST);
	// 最小化窗口
	//ShowWindow(hwnd, SW_MINIMIZE);
	// 隐藏任务栏图标
	ShowWindow(hwnd, SW_HIDE);

	// workSecond秒后发送ID_TIMER_WORK_END定时器消息
	SetTimer(hwnd, ID_TIMER_WORK_END, workSecond*1000, NULL);
	return 0;
}
