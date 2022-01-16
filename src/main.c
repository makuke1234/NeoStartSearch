#include "common.h"
#include "winproc.h"

ATOM registerMyClass(HINSTANCE hInst, WNDCLASSEXW * wc);
HWND createMyWindow(WNDCLASSEXW * wc);


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE __attribute__((unused)) hPrev, LPSTR __attribute__((unused)) lpCmdArgs, int __attribute__((unused)) nCmdShow)
{
	// Make the program DPI-aware if possible
	setDPIAware();

	WNDCLASSEXW wcex = { 0 };
	if (!registerMyClass(hInst, &wcex))
	{
		MessageBoxW(NULL, L"Error initializing window class!", wcex.lpszClassName, MB_OK | MB_ICONERROR);
		return 1;
	}
	if (!createMyWindow(&wcex))
	{
		MessageBoxW(NULL, L"Error creating window!", wcex.lpszClassName, MB_OK | MB_ICONERROR);
		return 2;
	}

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessageW(&msg, NULL, 0, 0)))
	{
		if (bRet == -1)
			return -1;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}

ATOM registerMyClass(HINSTANCE hInst, WNDCLASSEXW * wc)
{
	wc->cbSize        = sizeof(WNDCLASSEXW);
	wc->hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc->hCursor       = LoadCursorW(NULL, IDC_ARROW);
	wc->hIconSm       = wc->hIcon = LoadIconW(hInst, IDI_APPLICATION);
	wc->hInstance     = hInst;
	wc->lpfnWndProc   = &mainWinProc;
	wc->lpszClassName = L"NeoStartSearchClass";
	wc->style         = CS_HREDRAW | CS_VREDRAW;
	
	return RegisterClassExW(wc);
}

HWND createMyWindow(WNDCLASSEXW * wc)
{
	return CreateWindowExW(
		0,
		wc->lpszClassName,
		NULL,
		WS_POPUP | WS_BORDER,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		wc->hInstance,
		NULL
	);
}
