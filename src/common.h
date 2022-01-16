#ifndef COMMON_H
#define COMMON_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>

struct dpi_t
{
	uint32_t x, y;
};
extern struct dpi_t dpi;

enum windowsVersion_e
{
	wV_Windows2000,
	wV_WindowsXP,
	wV_WindowsVista,
	wV_Windows7,
	wV_Windows8,
	wV_Windows8_1,
	wV_Windows10,
	wV_Windows10_1607,

	wV_size
};
extern enum windowsVersion_e windowsVersion;
extern const char * windowsVersionStr[wV_size];
extern const uint32_t defSizex, defSizey;
extern uint32_t Posx, Posy, Sizex, Sizey, tbSizex, tbSizey;

void setDPIAware();
void getWindowDPI(HWND hWnd);
uint32_t getDipx(uint32_t coord);
uint32_t getDipy(uint32_t coord);

typedef struct pathEntry
{
	wchar_t * name;
	wchar_t * fullpath;
	HICON icon;
	int show;
} pathEntry_t;

typedef struct resultitems
{
	struct resultitems_node
	{
		union
		{
			pathEntry_t;
			pathEntry_t i;
		};
	} * items;
	int numitems;
} resultitems_t;
extern resultitems_t resultItems;

#endif
