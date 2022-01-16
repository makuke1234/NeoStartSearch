#ifndef WINPROC_H
#define WINPROC_H

#include "common.h"

extern HWND mainWindow;

void startSearchProgram(HWND hwnd);
void exitSearchProgram(HWND hwnd);
void resizeEditControl(HWND hwnd);

DWORD WINAPI indexingThread(LPVOID __attribute__((unused)) lpParam);

LRESULT CALLBACK mainWinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK mainWinEditSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

#endif
