#ifndef SEARCH_H
#define SEARCH_H

#include "common.h"

int addPathEntries(const wchar_t * path, pathEntry_t ** entry, int * entrysz);
void getPathEntries(pathEntry_t ** results, int * resultlen, char renew);
uint16_t search_LevenshteinDist(const wchar_t * str1, const wchar_t * str2);
uint16_t search_LevenshteinDist_s(const wchar_t * str1, const int len1, const wchar_t * str2, const int len2);
void search(const wchar_t * str, pathEntry_t ** results, int * resultlen);
void search_s(const wchar_t * str, int len, pathEntry_t ** results, int * resultlen);
void freeSearch(pathEntry_t * results, int resultlen);

void searchAndDraw(HWND hwnd, const wchar_t * str, int len);

#endif
