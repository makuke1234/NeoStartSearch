#include "com_components.h"
#include <shlobj.h>
#include <shellapi.h>
#include <string.h>
#include <stdbool.h>

static char com_initialized = false;
int com_init()
{
	if (!com_initialized && SUCCEEDED(CoInitialize(NULL)))
		com_initialized = true;
	
	return com_initialized;
}
void com_uninit()
{
	if (com_initialized)
	{
		CoUninitialize();
		com_initialized = false;
	}
}

int com_resolveshortcut(HWND hwnd, const wchar_t * link, wchar_t * resolvedpath)
{
	HRESULT hr;
	IShellLink * psl;

	int ret = false;

	hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID *)&psl);
	if (SUCCEEDED(hr))
	{
		IPersistFile * ppf;
		hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID *)&ppf);
		if (SUCCEEDED(hr))
		{
			hr = ppf->lpVtbl->Load(ppf, link, STGM_READ);
			if (SUCCEEDED(hr))
			{
				hr = psl->lpVtbl->Resolve(psl, hwnd, SLR_NO_UI);
				if (SUCCEEDED(hr))
				{
					WIN32_FIND_DATA wfd;
					hr = psl->lpVtbl->GetPath(psl, resolvedpath, MAX_PATH, &wfd, 0);
					if (SUCCEEDED(hr))
					{
						ret = true;
					}
				}
			}
			ppf->lpVtbl->Release(ppf);
		}
		psl->lpVtbl->Release(psl);
	}

	return ret;
}

int com_queryshow(const wchar_t * link)
{
	HRESULT hr;
	IShellLink * psl;

	int ret = SW_SHOW;

	hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID *)&psl);
	if (SUCCEEDED(hr))
	{
		hr = psl->lpVtbl->SetPath(psl, link);
		if (SUCCEEDED(hr))
		{
			hr = psl->lpVtbl->GetShowCmd(psl, &ret);
			if (FAILED(hr))
			{
				ret = SW_SHOW;
			}
		}

		psl->lpVtbl->Release(psl);
	}
	return ret;
}
