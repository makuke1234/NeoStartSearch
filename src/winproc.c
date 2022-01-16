#include "winproc.h"
#include "search.h"
#include <stdio.h>
#include <time.h>
#include <shellapi.h>

static int fontWidth, fontHeight, lineChars;
static int textlen = 0;

static int SpacingxIcon, SpacingyIcon, SpacingyText, iconHeight;

// Initiates search, WPARAM -> 0, LPARAM -> 0
// Reindexes files, WPARAM -> -1, LPARAM -> 0
#define WM_MAIN_SEARCH (WM_USER + 1)
/*
 *	Moves in the list:
 *	WPARAM = 1 -> Move up if possible
 *	WPARAM = 2 -> Move down if possible
 *	WPARAM = 3 -> Enter
 */
#define WM_MAIN_MOVE (WM_USER + 2)
#define WM_MAIN_ACTIVATE (WM_USER + 3)
#define WM_MAIN_HOOK (WM_APP + 1)
HWND mainWindow;
static HWND textbox;

static HHOOK llkbhook;
LRESULT CALLBACK LLKbHookProc(int code, WPARAM wp, LPARAM lp)
{
	if (!(code < 0))
	{
		SendMessageW(mainWindow, WM_MAIN_HOOK, wp, lp);
	}
	return CallNextHookEx(llkbhook, code, wp, lp);
}

void startSearchProgram(HWND hwnd)
{
	if (IsWindowVisible(hwnd))
		return;

	ShowWindow(hwnd, SW_SHOW);

	double j = 10.;
	int previ = 0;
	for (int i = 0, size = (int)Sizey; i < size; i += (int)(j * j / 2.5), j++)
	{
		MoveWindow(
			hwnd,
			Posx,
			Posy + (Sizey - i),
			Sizex,
			i,
			FALSE
		);
		RECT updateR = {
			.left = 0,
			.top = previ,
			.right = Sizex,
			.bottom = i
		};
		InvalidateRect(hwnd, &updateR, FALSE);
		Sleep(10);
		previ = i;
	}
	MoveWindow(
		hwnd,
		Posx,
		Posy,
		Sizex,
		Sizey,
		FALSE
	);
	RECT updateR = {
		.left = 0,
		.top = previ,
		.right = Sizex,
		.bottom = Sizey
	};
	InvalidateRect(hwnd, &updateR, FALSE);
	resizeEditControl(textbox);
	SendMessageW(hwnd, WM_MAIN_SEARCH, 0, 0);
	SendMessageW(hwnd, WM_MAIN_ACTIVATE, 0, 0);
}

void exitSearchProgram(HWND hwnd)
{
	double j = 10.;
	for (int i = Sizey - 1; i > 0; i -= (int)(j * j / 2.5), j++)
	{
		MoveWindow(
			hwnd,
			Posx,
			Posy + (Sizey - i),
			Sizex,
			i,
			FALSE
		);
		Sleep(10);
	}
	ShowWindow(hwnd, SW_HIDE);
}

void resizeEditControl(HWND hedit)
{
	tbSizex = Sizex;
	tbSizey = 2 * getDipy(7) + fontHeight * (1 + textlen / lineChars);
	MoveWindow(
		hedit,
		0, Sizey - tbSizey,
		tbSizex, tbSizey,
		TRUE
	);
	RECT newrect = {
		.left   = getDipx(7),
		.top    = getDipy(7),
		.right  = tbSizex - getDipx(7),
		.bottom = tbSizey - getDipy(7)
	};
	SendMessageW(hedit, EM_SETRECT, 0, (LPARAM)&newrect);
}

static HANDLE h_indexingThread = NULL;
DWORD WINAPI indexingThread(LPVOID __attribute__((unused)) lpParam)
{
	// Initialize the search index
	pathEntry_t * entries;
	int numentries;
	getPathEntries(&entries, &numentries, 1);

	h_indexingThread = NULL;
	PostMessageW(mainWindow, WM_MAIN_SEARCH, 0, 0);
	return 0;
}

static HANDLE h_mouseActivityThread = NULL;
DWORD WINAPI mouseActivityThread(LPVOID lpParam)
{
	int ** lpArr = (int **)lpParam;
	int * mouseyPos = lpArr[0];
	int * mousedown = lpArr[1];
	RECT progrect = {
		.left = 0,
		.top = 0,
		.right = Sizex,
		.bottom = Sizey - tbSizey
	};
	while (1)
	{
		if (IsWindowVisible(mainWindow) == FALSE)
			break;
		if (*mouseyPos == -1)
			continue;
		POINT p;
		GetCursorPos(&p);
		RECT r;
		GetWindowRect(mainWindow, &r);
		// Mouse out of boundaries
		if (p.x < r.left || p.x > r.right || p.y < r.top || p.y > r.bottom)
		{
			*mouseyPos = -1;
			*mousedown = FALSE;
			InvalidateRect(mainWindow, &progrect, FALSE);
		}
		Sleep(50);
	}
	h_mouseActivityThread = NULL;
	return 0;
}
static inline void createMouseActivityThread(int * pmypos, int * pmdown)
{
	static int * p[2];
	if (h_mouseActivityThread == NULL)
	{
		p[0] = pmypos;
		p[1] = pmdown;
		h_mouseActivityThread = CreateThread(
			NULL,
			20 * sizeof(size_t),
			&mouseActivityThread,
			p,
			0,
			NULL
		);
	}
}

static inline void getMaxitemsRealstart(int Areay, int startpos, int * maxitems, int * realstart)
{
	int maxitems_ = (Areay - SpacingyIcon) / (iconHeight + SpacingyIcon);
	int realstart_ = startpos < maxitems_ ? 0 : startpos - maxitems_ + 1;
	if (maxitems != NULL) *maxitems = maxitems_;
	if (realstart != NULL) *realstart = realstart_;
}
static inline void optimizedInvalidate(HWND hwnd, int prevpos, int newpos)
{
	if (prevpos == -1 && newpos == -1)
		return;

	// newpos can't be -1
	RECT r = {
		.left  = 0,
		.right = Sizex
	};
	if (prevpos == -1 || prevpos == newpos)
	{
		r.top    = newpos * (iconHeight + SpacingyIcon);
		r.bottom = r.top  +  iconHeight + SpacingyIcon;
		InvalidateRect(hwnd, &r, FALSE);
	}
	// first condition can only fail if prevpos != -1
	else if (newpos == -1)
	{
		r.top    = prevpos * (iconHeight + SpacingyIcon);
		r.bottom = r.top   +  iconHeight + SpacingyIcon;
		InvalidateRect(hwnd, &r, FALSE);
	}
	else if (prevpos == newpos + 1)
	{
		r.top    =    newpos * (iconHeight + SpacingyIcon);
		r.bottom = r.top + 2 * (iconHeight + SpacingyIcon);
		InvalidateRect(hwnd, &r, FALSE);
	}
	else if (prevpos == newpos - 1)
	{
		r.top    =   prevpos * (iconHeight + SpacingyIcon);
		r.bottom = r.top + 2 * (iconHeight + SpacingyIcon);
		InvalidateRect(hwnd, &r, FALSE);
	}
	else
	{
		r.top    = prevpos * (iconHeight + SpacingyIcon);
		r.bottom = r.top   +  iconHeight + SpacingyIcon;
		InvalidateRect(hwnd, &r, FALSE);
		r.top    = newpos * (iconHeight + SpacingyIcon);
		r.bottom = r.top  +  iconHeight + SpacingyIcon;
		InvalidateRect(hwnd, &r, FALSE);
	}
}

static HFONT resultFont, resultFontBold;

void drawTextWithMatches(HDC hdc, const wchar_t * textboxtext, const wchar_t * text, int len, RECT * r)
{
	int matchIndexes[MAX_PATH] = { [0] = -1 }, matchIndexeslen = 1;
	for (int i = 0; (i < len) && (textboxtext[matchIndexeslen - 1] != L'\0'); ++i)
	{
		if (tolower(textboxtext[matchIndexeslen - 1]) == tolower(text[i]))
		{
			matchIndexes[matchIndexeslen] = i;
			++matchIndexeslen;
		}
	}
	int x = r->left, y = r->top;
	const wchar_t * ttext = text;
	SelectObject(hdc, resultFontBold);
	for (int i = 1; i < matchIndexeslen; ++i)
	{
		// if any text can be drawn in between highlighted characters
		int diff = matchIndexes[i] - matchIndexes[i - 1] - 1;
		if (diff > 0)
		{
			SelectObject(hdc, resultFont);
			// Print out text
			TextOutW(hdc, x, y, ttext, diff);
			// Calculate new x coordinate
			SIZE sz;
			GetTextExtentPoint32W(hdc, ttext, diff, &sz);
			x += sz.cx;
			ttext += diff;
			SelectObject(hdc, resultFontBold);
		}
		// draw highlighted character
		TextOutW(hdc, x, y, ttext, 1);
		SIZE sz;
		GetTextExtentPoint32W(hdc, ttext, 1, &sz);
		x += sz.cx;

		++ttext;
	}
	SelectObject(hdc, resultFont);
	TextOutW(hdc, x, y, ttext, len - matchIndexes[matchIndexeslen - 1]);
}

LRESULT CALLBACK mainWinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static HFONT textboxFont;
	static HBRUSH selectBrush, mselectBrush;
	static int startpos = 0;
	static int mouseypos = -1;
	static BOOL mousedown = FALSE;

	switch (msg)
	{
	case WM_MAIN_SEARCH:
	{
		if (h_indexingThread != NULL)
			break;
		if ((int)wp == -1)
		{
			mouseypos = -1;
			// empty textbox
			SetWindowTextW(textbox, L"");
			textlen = 0;

			h_indexingThread = CreateThread(
				NULL,
				0,
				&indexingThread,
				NULL,
				0,
				NULL
			);
			if (h_indexingThread == NULL)
			{
				MessageBoxW(hwnd, L"Indexing thread creation failed!", L"Search", MB_ICONERROR | MB_OK);
				PostMessageW(hwnd, WM_QUIT, 0, 0);
				break;
			}

			RECT r = {
				.left = 0,
				.top = 0,
				.right = Sizex,
				.bottom = Sizey - tbSizey
			};
			InvalidateRect(hwnd, &r, FALSE);
			break;
		}

		wchar_t text[MAX_PATH];
		int len = GetWindowTextW(textbox, text, MAX_PATH);
		// do nothing if text length is under 3 characters
		if (len < 3)
		{
			resultItems.numitems = 0;
			RECT r = {
				.left = 0,
				.top = 0,
				.right = Sizex,
				.bottom = Sizey - tbSizey
			};
			InvalidateRect(hwnd, &r, FALSE);
			break;
		}
		startpos = 0;
		searchAndDraw(hwnd, text, len);
		break;
	}
	case WM_MAIN_MOVE:
	{
		RECT r = {
			.left = 0,
			.top = 0,
			.right = Sizex,
			.bottom = Sizey - tbSizey
		};
		switch (wp)
		{
		case 1:
		{
			// Move up
			int prevpos = startpos;
			if (startpos > 0)
				--startpos;
			else
				startpos = resultItems.numitems - 1;
			int maxitems;
			getMaxitemsRealstart(r.bottom, startpos, &maxitems, NULL);
			if (prevpos < maxitems && startpos < maxitems)
				optimizedInvalidate(hwnd, prevpos, startpos);
			else
				InvalidateRect(hwnd, &r, FALSE);

			break;
		}
		case 2:
		{
			// Move down
			int prevpos = startpos;
			if (startpos < resultItems.numitems - 1)
				++startpos;
			else
				startpos = 0;
			int maxitems;
			getMaxitemsRealstart(r.bottom, startpos, &maxitems, NULL);
			if (prevpos < maxitems && startpos < maxitems)
				optimizedInvalidate(hwnd, prevpos, startpos);
			else
				InvalidateRect(hwnd, &r, FALSE);
			
			break;
		}
		case 3:
			// Open link and close application
			ShellExecuteW(hwnd, 0, resultItems.items[startpos].fullpath, 0, 0, resultItems.items[startpos].show);
			exitSearchProgram(hwnd);
			break;
		}
		break;
	}
	case WM_MAIN_HOOK:
	{
		static char winkey = 0;

		wchar_t key = ((KBDLLHOOKSTRUCT*)lp)->vkCode;
		char pressed = (wp == WM_KEYDOWN) | (wp == WM_SYSKEYDOWN);

		switch (key)
		{
		case VK_LWIN:
		case VK_RWIN:
			winkey = pressed;
			break;
		case L'F':
			if (pressed && winkey)
			{
				startSearchProgram(hwnd);
			}
			break;
		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (h_indexingThread != NULL)
		{
			RECT r = {
				.left = 0,
				.top = 0,
				.right = Sizex,
				.bottom = Sizey - tbSizey
			};
			FillRect(hdc, &r, (HBRUSH)COLOR_WINDOW);
			SelectObject(hdc, resultFont);
			SetBkMode(hdc, TRANSPARENT);
			r.top = (r.bottom - SpacingyText) / 2;
			r.bottom = r.top + SpacingyText;
			const wchar_t indextext[] = L"Indexing files. Please wait...";
			DrawTextW(hdc, indextext, sizeof indextext / sizeof *indextext - 1, &r, DT_CENTER);

			break;
		}


		// Draw items
		if (startpos >= resultItems.numitems && resultItems.numitems)
			startpos = resultItems.numitems - 1;

		int Areay = Sizey - tbSizey;
		int realtspacing = (iconHeight + SpacingyIcon - SpacingyText) / 2;
		int maxitems, tempstart;
		getMaxitemsRealstart(Areay, startpos, &maxitems, &tempstart);

		SelectObject(hdc, resultFont);
		SetBkMode(hdc, TRANSPARENT);
		{
			RECT r = {
				.left = 0,
				.top = 0,
				.right = Sizex,
				.bottom = Areay
			};
			if (resultItems.numitems > 0)
			{
				r.bottom = (startpos - tempstart) * (iconHeight + SpacingyIcon);
				FillRect(hdc, &r, (HBRUSH)COLOR_WINDOW);
				RECT selectr = {
					.left = 0,
					.top = r.bottom,
					.right = r.right,
					.bottom = selectr.top + iconHeight + SpacingyIcon
				};
				FillRect(hdc, &selectr, selectBrush);
				r.top = selectr.bottom;
				r.bottom = Areay;
				--selectr.right;
				DrawFocusRect(hdc, &selectr);

				if ((mouseypos != -1) && mouseypos < resultItems.numitems && mouseypos < maxitems && mouseypos != (startpos - tempstart))
				{
					RECT mselectr = {
						.left = 0,
						.top = mouseypos * (iconHeight + SpacingyIcon),
						.right = r.right,
						.bottom = mselectr.top + iconHeight + SpacingyIcon
					};
					if (mselectr.top > selectr.top)
					{
						r.top = selectr.bottom;
						r.bottom = mselectr.top;
						FillRect(hdc, &r, (HBRUSH)COLOR_WINDOW);
						r.top = mselectr.bottom;
					}
					else
					{
						r.top = mselectr.bottom;
						r.bottom = selectr.top;
						FillRect(hdc, &r, (HBRUSH)COLOR_WINDOW);
						r.top = selectr.bottom;
					}
					FillRect(hdc, &mselectr, mselectBrush);
					r.bottom = Areay;
				}
			}
			FillRect(hdc, &r, (HBRUSH)COLOR_WINDOW);
		}
		// Get textbox text for highlighting purposes
		wchar_t textboxtext[MAX_PATH];
		GetWindowTextW(textbox, textboxtext, MAX_PATH);
		for (int i = tempstart, j = 0; i < resultItems.numitems && j < maxitems; ++i, ++j)
		{
			DrawIcon(hdc, SpacingxIcon, SpacingyIcon + j * (iconHeight + SpacingyIcon), resultItems.items[i].icon);
			RECT r = {
				.left = SpacingxIcon + iconHeight + SpacingxIcon,
				.top = realtspacing + j * (iconHeight + SpacingyIcon),
				.right = Sizex,
				.bottom = r.top + SpacingyText
			};
			if (i == startpos)
				SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
			else if (i == startpos + 1)
				SetTextColor(hdc, RGB(0, 0, 0));

			const wchar_t * item = resultItems.items[i].name;
			drawTextWithMatches(hdc, textboxtext, item, wcslen(item), &r);
		}

		EndPaint(hwnd, &ps);
		break;
	}
	case WM_MOUSEMOVE:
	{
		// Highlight
		int y = HIWORD(lp), prevmouseypos = mouseypos;
		if (y < (int)(Sizey - tbSizey))
			mouseypos = y / (iconHeight + SpacingyIcon);
		else
			mouseypos = -1;
		if (mouseypos != prevmouseypos)
		{			
			if (mousedown == TRUE)
			{
				int tempstart;
				getMaxitemsRealstart(Sizey - tbSizey, startpos, NULL, &tempstart);
				startpos = tempstart + mouseypos;
			}
			optimizedInvalidate(hwnd, prevmouseypos, mouseypos);
		}
		createMouseActivityThread(&mouseypos, &mousedown);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (mouseypos == -1)
			break;
		mousedown = TRUE;

		int tempstart;
		RECT r = {
			.left = 0,
			.top = 0,
			.right = Sizex,
			.bottom = Sizey - tbSizey
		};
		getMaxitemsRealstart(r.bottom, startpos, NULL, &tempstart);
		startpos = tempstart + mouseypos;
		InvalidateRect(hwnd, &r, FALSE);
		break;
	}
	case WM_LBUTTONUP:
	{
		if (mouseypos == -1)
			break;

		int tempstart;
		RECT r = {
			.left = 0,
			.top = 0,
			.right = Sizex,
			.bottom = Sizey - tbSizey
		};
		getMaxitemsRealstart(r.bottom, startpos, NULL, &tempstart);
		startpos = tempstart + mouseypos;
		InvalidateRect(hwnd, &r, FALSE);
		PostMessageW(hwnd, WM_MAIN_MOVE, 3, 0);
		mousedown = FALSE;
		break;
	}
	case WM_MOUSEWHEEL:
	{
		static int mousewheel = 0;
		int zDelta = GET_WHEEL_DELTA_WPARAM(wp);

		mousewheel += zDelta;
		// Move up
		if (mousewheel >= WHEEL_DELTA)
		{
			int iter = mousewheel / WHEEL_DELTA;
			mousewheel %= WHEEL_DELTA;

			for (int i = 0; i < iter; i++)
			{
				PostMessageW(hwnd, WM_MAIN_MOVE, 1, 0);
			}
		}
		// Move down
		else if (mousewheel <= WHEEL_DELTA)
		{
			int iter = -mousewheel / WHEEL_DELTA;
			mousewheel %= WHEEL_DELTA;

			for (int i = 0; i < iter; i++)
			{
				PostMessageW(hwnd, WM_MAIN_MOVE, 2, 0);
			}
		}
		break;
	}
	case WM_SIZE:
		resizeEditControl(textbox);
		break;
	case WM_SHOWWINDOW:
		if (wp == TRUE)	// Window is being shown
		{
			mouseypos = -1;
			// empty textbox
			SetWindowTextW(textbox, L"");
			textlen = 0;
			// Resize and reposition the window appropriately in respect to the desktop size
			Sizex = getDipx(defSizex);
			lineChars = (Sizex - 2 * getDipx(7)) / fontWidth;
			Sizey = getDipy(defSizey);
			Posx = 0;
			Posy = GetSystemMetrics(SM_CYFULLSCREEN) + GetSystemMetrics(SM_CYCAPTION) - Sizey;
			MoveWindow(hwnd, Posx, Posy, Sizex, Sizey, FALSE);
		}
		break;
	case WM_SETFOCUS:
		SetFocus(textbox);
		break;
	case WM_ACTIVATE:
		if (wp == WA_INACTIVE)
			exitSearchProgram(hwnd);
		break;
	case WM_CREATE:
	{
		mainWindow = hwnd;
		// Get DPI
		getWindowDPI(hwnd);
		SpacingxIcon = getDipx(5);
		SpacingyIcon = getDipy(5);
		iconHeight = getDipy(38);

		selectBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
		mselectBrush = GetSysColorBrush(COLOR_GRAYTEXT);

		// Initialize all important "stuff about window"
		textboxFont = CreateFontW(
			-MulDiv(11, dpi.y, 72),
			0,
			0,
			0,
			FW_NORMAL,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			FIXED_PITCH,
			L"Consolas"
		);
		resultFont = CreateFontW(
			-MulDiv(11, dpi.y, 72),
			0,
			0,
			0,
			FW_NORMAL,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			FIXED_PITCH,
			L"Segoe UI"
		);
		resultFontBold = CreateFontW(
			-MulDiv(11, dpi.y, 72),
			0,
			0,
			0,
			FW_BOLD,
			FALSE,
			FALSE,
			FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			FIXED_PITCH,
			L"Segoe UI"
		);
		HDC hdc = GetDC(NULL);

		SelectObject(hdc, textboxFont);
		TEXTMETRICW metric;
		GetTextMetricsW(hdc, &metric);
		fontHeight = metric.tmHeight;
		fontWidth = metric.tmAveCharWidth;

		SelectObject(hdc, resultFont);
		GetTextMetricsW(hdc, &metric);
		SpacingyText = metric.tmHeight;

		ReleaseDC(NULL, hdc);

		textbox = CreateWindowExW(
			0,
			L"edit",
			L"",
			WS_CHILD | WS_VISIBLE | ES_MULTILINE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			hwnd,
			NULL,
			NULL,
			NULL
		);
		// Set custom font
		SendMessageW(textbox, WM_SETFONT, (WPARAM)textboxFont, FALSE);
		// Set 260 character limit, as this is the path limit for NTFS
		SendMessageW(textbox, EM_SETLIMITTEXT, MAX_PATH, 0);
		SetWindowSubclass(textbox, &mainWinEditSubclass, 0, 0);

		Sizex = getDipx(defSizex);
		lineChars = (Sizex - 2 * getDipx(7)) / fontWidth;
		Sizey = getDipy(defSizey);
		Posx = 0;
		Posy = GetSystemMetrics(SM_CYFULLSCREEN) + GetSystemMetrics(SM_CYCAPTION) - Sizey;
		MoveWindow(
			hwnd,
			Posx, Posy,
			Sizex, Sizey,
			FALSE
		);

		llkbhook = SetWindowsHookExW(WH_KEYBOARD_LL, &LLKbHookProc, NULL, 0);


		h_indexingThread = CreateThread(
			NULL,
			0,
			&indexingThread,
			NULL,
			0,
			NULL
		);
		if (h_indexingThread == NULL)
		{
			MessageBoxW(hwnd, L"Indexing thread creation failed!", L"Search", MB_ICONERROR | MB_OK);
			PostMessageW(hwnd, WM_QUIT, 0, 0);
		}

		break;
	}
	case WM_CLOSE:
		UnhookWindowsHookEx(llkbhook);
		DeleteObject(textboxFont);
		DeleteObject(selectBrush);
		DeleteObject(mselectBrush);
		DestroyWindow(hwnd);
		{
			pathEntry_t * result;
			int resultlen;
			getPathEntries(&result, &resultlen, 0);
			for (int i = 0; i < resultlen; i++)
			{
				DestroyIcon(result[i].icon);
			}
			freeSearch(result, resultlen);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MAIN_ACTIVATE:
	{
		ShowWindow(hwnd, SW_RESTORE);
		HWND curWnd = GetForegroundWindow();

		DWORD dwMyID  = GetWindowThreadProcessId(hwnd, NULL);
		DWORD dwCurID = GetWindowThreadProcessId(curWnd, NULL);
		AttachThreadInput(dwCurID, dwMyID, TRUE);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		SetActiveWindow(hwnd);
		AttachThreadInput(dwCurID, dwMyID, FALSE);
		break;
	}
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
	return 0;
}

LRESULT CALLBACK mainWinEditSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR __attribute__((unused)) uIdSubclass, DWORD_PTR __attribute__((unused)) dwRefData)
{
	static DWORD startpos;
	static BOOL setstartpos = FALSE;

	switch (msg)
	{
	case WM_CHAR:
		switch (wp)
		{
		case VK_TAB:
		case VK_RETURN:
			PostMessageW(mainWindow, WM_MAIN_MOVE, 3, 0);
			return 0;
		case 1:
			SendMessageW(hwnd, EM_GETSEL, (WPARAM)&startpos, (LPARAM)NULL);
			SendMessageW(hwnd, EM_SETSEL, 0, -1);
			setstartpos = TRUE;
			return 1;
		}
		PostMessageW(mainWindow, WM_MAIN_SEARCH, 0, 0);
		break;
	case WM_KEYDOWN:
		switch (wp)
		{
		case VK_ESCAPE:
			exitSearchProgram(mainWindow);
			break;
		case VK_UP:
			PostMessageW(mainWindow, WM_MAIN_MOVE, 1, 0);
			return 0;
		case VK_DOWN:
			PostMessageW(mainWindow, WM_MAIN_MOVE, 2, 0);
			return 0;
		case VK_LEFT:
		case VK_RIGHT:
			if (setstartpos == TRUE)
			{
				SendMessageW(hwnd, EM_SETSEL, startpos, startpos);
				setstartpos = FALSE;
			}
			break;
		case VK_DELETE:
		case VK_BACK:
			textlen = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0) - 1;
			if (textlen < 0)
				textlen = 0;
			{
				RECT rect = {
					.left   = getDipx(7),
					.top    = getDipy(7),
					.right  = tbSizex - getDipx(7),
					.bottom = tbSizey - getDipy(7)
				};
				if (textlen && (textlen / lineChars + 1) < (rect.bottom - rect.top) / fontHeight)
					SendMessageW(mainWindow, WM_SIZE, 0, 0);
			}
			break;
		case VK_F5:
			SendMessageW(mainWindow, WM_MAIN_SEARCH, (WPARAM)-1, 0);
			break;
		default:
		{
			textlen = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0) + 1;
			if (textlen > MAX_PATH)
				textlen = MAX_PATH;
			RECT rect = {
				.left   = getDipx(7),
				.top    = getDipy(7),
				.right  = tbSizex - getDipx(7),
				.bottom = tbSizey - getDipy(7)
			};
			if (textlen && (textlen / lineChars) >= (rect.bottom - rect.top) / fontHeight)
				SendMessageW(mainWindow, WM_SIZE, 0, 0);
		}
		}
		break;
	case WM_KEYUP:
	{
		textlen = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
		RECT rect = {
			.left   = getDipx(7),
			.top    = getDipy(7),
			.right  = tbSizex - getDipx(7),
			.bottom = tbSizey - getDipy(7)
		};
		switch (wp)
		{
		case VK_DELETE:
		case VK_BACK:
			if ((textlen / lineChars + 1) < (rect.bottom - rect.top) / fontHeight)
				SendMessageW(mainWindow, WM_SIZE, 0, 0);
			break;
		default:
			if ((textlen / lineChars) >= (rect.bottom - rect.top) / fontHeight)
				SendMessageW(mainWindow, WM_SIZE, 0, 0);
		}
		break;
	}
	}
	return DefSubclassProc(hwnd, msg, wp, lp);
}
