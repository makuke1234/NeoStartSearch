#include "common.h"

struct dpi_t dpi = { 96, 96 };

enum windowsVersion_e windowsVersion = wV_WindowsVista;
const char* windowsVersionStr[wV_size] = {
	"Windows 2000",
	"Windows XP",
	"Windows Vista",
	"Windows 7",
	"Windows 8",
	"Windows 8.1",
	"Windows 10",
	"Windows 10 1607"
};
const uint32_t defSizex = 350, defSizey = 400;
uint32_t Posx, Posy, Sizex, Sizey, tbSizex, tbSizey;

void setDPIAware()
{
	typedef WINBOOL (WINAPI *PSetProcessDPIAware)();
	HMODULE hUser32 = LoadLibraryW(L"user32");
	if (hUser32)
	{
		PSetProcessDPIAware pSetProcessDPIAware = (PSetProcessDPIAware)GetProcAddress(hUser32, "SetProcessDPIAware");
		if (pSetProcessDPIAware)
		{
			if (windowsVersion < wV_WindowsVista)
				windowsVersion = wV_WindowsVista;
			pSetProcessDPIAware();
			FreeLibrary(hUser32);
		}
	}
}
void getWindowDPI(HWND hWnd)
{
	typedef HRESULT (WINAPI *PGetDpiForMonitor)(HMONITOR hmonitor, int dpiType, UINT * dpiX, UINT * dpiY);
	// Try to get the DPI setting for the monitor where the given window is located.
	// This API is Windows 8.1+.
	HMODULE hShcore = LoadLibraryW(L"shcore");
	if (hShcore)
	{
		PGetDpiForMonitor pGetDpiForMonitor = (PGetDpiForMonitor)GetProcAddress(hShcore, "GetDpiForMonitor");
		if (pGetDpiForMonitor)
		{
			if (windowsVersion < wV_Windows8_1)
				windowsVersion = wV_Windows8_1;
			HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
			UINT uiDpiX;
			UINT uiDpiY;
			HRESULT hr = pGetDpiForMonitor(hMonitor, 0, &uiDpiX, &uiDpiY);
			if (SUCCEEDED(hr))
			{
				dpi.x = uiDpiX;
				dpi.y = uiDpiY;
			}
		}
		FreeLibrary(hShcore);
	}
	else
	{
		// We couldn't get the window's DPI above, so get the DPI of the primary monitor
		// using an API that is available in all Windows versions.
		HDC hScreenDC = GetDC(NULL);
		dpi.x = GetDeviceCaps(hScreenDC, LOGPIXELSX);
		dpi.y = GetDeviceCaps(hScreenDC, LOGPIXELSY);
		ReleaseDC(NULL, hScreenDC);
	}
}
uint32_t getDipx(uint32_t coord)
{
	return (coord * dpi.x) / 96u;
}
uint32_t getDipy(uint32_t coord)
{
	return (coord * dpi.y) / 96u;
}

resultitems_t resultItems = { 0 };
