#include "search.h"
#include <stdlib.h>
#include <string.h>
#include <shellapi.h>
#include "com_components.h"
#include "winproc.h"

int addPathEntries(const wchar_t * path, pathEntry_t ** entry, int * entrysz)
{
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	{
		wchar_t buffer[MAX_PATH];
		wcscpy_s(buffer, MAX_PATH, path);
		wcscat_s(buffer, MAX_PATH, L"\\*");
		hFind = FindFirstFileW(buffer, &ffd);
	}
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	if ((*entry) == NULL || (*entrysz) == 0)
	{
		*entry = malloc(sizeof(pathEntry_t));
		*entrysz = 1;
	}

	int index = 0;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
				continue;
			wchar_t dir[MAX_PATH];
			wcscpy_s(dir, MAX_PATH, path);
			wcscat_s(dir, MAX_PATH, L"\\");
			wcscat_s(dir, MAX_PATH, ffd.cFileName);
			pathEntry_t * newentry = NULL;
			int newentrysz = 0;
			int sz = addPathEntries(dir, &newentry, &newentrysz);
			// Merge
			*entrysz = index + sz;
			*entry = realloc(*entry, (*entrysz) * sizeof(pathEntry_t));
			memcpy((*entry) + index, newentry, sz * sizeof(pathEntry_t));
			free(newentry);
			index += sz;
		}
		else
		{
			if (index >= *entrysz)
			{
				*entrysz = index * 2;
				*entry = realloc(*entry, (*entrysz) * sizeof(pathEntry_t));
			}
			(*entry)[index].name = wcsdup(ffd.cFileName);
			
			wchar_t fullpath[MAX_PATH];
			wcscpy_s(fullpath, MAX_PATH, path);
			wcscat_s(fullpath, MAX_PATH, L"\\");
			wcscat_s(fullpath, MAX_PATH, ffd.cFileName);

			(*entry)[index].fullpath = wcsdup(fullpath);
			
			SHFILEINFOW sfi;
			SHGetFileInfoW(fullpath, 0, &sfi, sizeof sfi, SHGFI_TYPENAME | SHGFI_ICON);
			if (wcscmp(sfi.szTypeName, L"Shortcut") == 0)
			{
				DestroyIcon(sfi.hIcon);
				SHGetFileInfoW(fullpath, 0, &sfi, sizeof sfi, SHGFI_ICONLOCATION);
				if (wcslen(sfi.szDisplayName))
				{
					wchar_t iconstr[MAX_PATH];
					ExpandEnvironmentStringsW(sfi.szDisplayName, iconstr, MAX_PATH);
					(*entry)[index].icon = ExtractIconW(hInstance, iconstr, sfi.iIcon);
				}
				else
				{
					wchar_t buf[MAX_PATH];
					// Resolve shortcut
					if (com_resolveshortcut(mainWindow, fullpath, buf))
						(*entry)[index].icon = ExtractIconW(hInstance, buf, 0);
					else
						(*entry)[index].icon = LoadIconW(NULL, IDI_APPLICATION);
				}
				(*entry)[index].show = com_queryshow(fullpath);
			}
			else
			{
				(*entry)[index].icon = sfi.hIcon;
				(*entry)[index].show = SW_SHOW;
			}

			index++;
		}
	} while (FindNextFileW(hFind, &ffd));
	FindClose(hFind);

	return index;
}
void getPathEntries(pathEntry_t ** results, int * resultlen, char renew)
{
	pathEntry_t * Rresults1 = NULL, * Rresults2 = NULL;
	int Rresultlen1 = 0, Rresultlen2 = 0;
	static pathEntry_t * compRresults = NULL;
	static int compRresultlen = 0;
	
	if (renew)
	{
		if (compRresults != NULL)
		{
			freeSearch(compRresults, compRresultlen);
			compRresults = NULL;
		}
	}

	if (compRresults == NULL)
	{
		com_init();
		// Load in entries from start menu
		wchar_t dest[MAX_PATH];
		ExpandEnvironmentStringsW(L"%appdata%\\Microsoft\\Windows\\Start Menu", dest, MAX_PATH);
		int sz1 = addPathEntries(dest, &Rresults1, &Rresultlen1);
		ExpandEnvironmentStringsW(L"%programdata%\\Microsoft\\Windows\\Start Menu", dest, MAX_PATH);
		int sz2 = addPathEntries(dest, &Rresults2, &Rresultlen2);
		compRresultlen = sz1 + sz2;
		compRresults = realloc(Rresults1, compRresultlen * sizeof(pathEntry_t));
		memcpy(compRresults + sz1, Rresults2, sz2 * sizeof(pathEntry_t));
		free(Rresults2);
		com_uninit();
	}

	*results = compRresults;
	*resultlen = compRresultlen;
}


static inline int search_intmin2(int num1, int num2)
{
	return num1 < num2 ? num1 : num2;
}
static inline int search_intmin3(int num1, int num2, int num3)
{
	return search_intmin2(search_intmin2(num1, num2), num3);
}
static inline int search_intmax(int num1, int num2)
{
	return num2 < num1 ? num1 : num2;
}

uint16_t search_LevenshteinDist(const wchar_t * str1, const wchar_t * str2)
{
	return search_LevenshteinDist_s(str1, wcslen(str1), str2, wcslen(str2));
}
uint16_t search_LevenshteinDist_s(const wchar_t * str1, int len1, const wchar_t * str2, int len2)
{
	++len1;
	++len2;
	uint16_t dist[len1][len2];

	dist[0][0] = 0;
	for (int i = 1; i < len1; ++i)
		dist[i][0] = i;
	for (int j = 1; j < len2; ++j)
		dist[0][j] = j;

	// Eliminate the need to do index - 1
	--str1;
	--str2;
	for (int j = 1; j < len2; ++j)
	{
		for (int i = 1; i < len1; ++i)
		{
			uint16_t cost = (tolower(str1[i]) == tolower(str2[j])) ? 0 : 1;
			dist[i][j] = search_intmin3(
				dist[i - 1][j    ] + 1,
				dist[i    ][j - 1] + 1,
				dist[i - 1][j - 1] + cost
			);
		}
	}
	return dist[len1 - 1][len2 - 1];
}


static pathEntry_t * search_s_compare_arr;
static uint16_t * search_s_compare_dist;
static int search_s_compare(const void * arg1, const void * arg2)
{
	int index1 = (pathEntry_t *)arg1 - search_s_compare_arr;
	int index2 = (pathEntry_t *)arg2 - search_s_compare_arr;
	return (int)search_s_compare_dist[index1] - (int)search_s_compare_dist[index2];
}

void search(const wchar_t * str, pathEntry_t ** results, int * resultlen)
{
	search_s(str, wcslen(str), results, resultlen);
}
void search_s(const wchar_t * str, int len, pathEntry_t ** results, int * resultlen)
{
	pathEntry_t * pathRes;
	int pathLen;
	getPathEntries(&pathRes, &pathLen, 0);

	pathEntry_t * Rresults = malloc(sizeof(pathEntry_t));
	uint16_t * distances = malloc(sizeof(uint16_t));
	int Rresultlen = 1;
	int index = 0;

	for (int i = 0; i < pathLen; i++)
	{
		int templen = wcslen(pathRes[i].name);
		uint16_t distance = search_LevenshteinDist_s(str, len, pathRes[i].name, templen);
		/*
		 * Only add node if Levenshtein distance is smaller than maximum
		 * length of 2 strings - search term length
		 */
		if (distance < (uint16_t)(search_intmax(len, templen) - len + 1))
		{
			// Resize if necessary
			if (index >= Rresultlen)
			{
				Rresultlen = index * 2;
				Rresults = realloc(Rresults, Rresultlen * sizeof(pathEntry_t));
				distances = realloc(distances, Rresultlen * sizeof(uint16_t));
			}
			// Add result to list
			Rresults[index] = pathRes[i];

			distances[index] = distance;

			++index;
		}
	}
	// Shrink to fit
	Rresultlen = index;
	Rresults = realloc(Rresults, Rresultlen * sizeof(pathEntry_t));
	distances = realloc(distances, Rresultlen * sizeof(uint16_t));

	// Sort list by Levenshtein distance
	search_s_compare_arr = Rresults;
	search_s_compare_dist = distances;
	qsort(Rresults, Rresultlen, sizeof(pathEntry_t), &search_s_compare);
	search_s_compare_arr = NULL;
	search_s_compare_dist = NULL;
	free(distances);

	*results = Rresults;
	*resultlen = Rresultlen;
}
void freeSearch(pathEntry_t * results, int resultlen)
{
	for (int i = 0; i < resultlen; ++i)
	{
		free(results[i].name);
		free(results[i].fullpath);
	}
	free(results);
}

void searchAndDraw(HWND hwnd, const wchar_t * str, int len)
{
	pathEntry_t * results;
	int resultlen;
	search_s(str, len, &results, &resultlen);

	resultItems.numitems = search_intmin2(resultlen, 100);
	resultItems.items = realloc(resultItems.items, resultItems.numitems * sizeof(struct resultitems_node));
	for (int i = 0; i < resultItems.numitems; i++)
	{
		resultItems.items[i].i.fullpath = results[i].fullpath;
		int doti = wcslen(results[i].name);
		for (wchar_t * str = results[i].name; doti >= 0; --doti)
		{
			if (str[doti] == L'.')
				break;
		}
		resultItems.items[i].name = malloc((doti + 1) * sizeof(wchar_t));
		memcpy(resultItems.items[i].name, results[i].name, doti * sizeof(wchar_t));
		resultItems.items[i].name[doti] = L'\0';

		resultItems.items[i].icon = results[i].icon;
		resultItems.items[i].show = results[i].show;
	}
	free(results);

	RECT r = {
		.left = 0,
		.top = 0,
		.right = Sizex,
		.bottom = Sizey - tbSizey
	};
	// Only invalidate the result area
	InvalidateRect(hwnd, &r, FALSE);
}
