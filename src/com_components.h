#ifndef COM_COMPONENTS_H
#define COM_COMPONENTS_H

#include "common.h"

int com_init();
void com_uninit();

int com_resolveshortcut(HWND hwnd, const wchar_t * link, wchar_t * resolvedpath);
int com_queryshow(const wchar_t * link);

#endif
