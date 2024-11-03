#pragma once
#ifndef _HEADER_WHID_UTILS_H
#define _HEADER_WHID_UTILS_H

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "whid_struct.h"

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

void     ConvertSecondsToTime(u32 totalSeconds, wchar_t* p_buffer);
void     RemoveCR(wchar_t* p_buffer);
wchar_t* StrncpyTruncate(wchar_t* p_dst, size_t szDest, const wchar_t* p_src);
int      UniSwprintf(wchar_t* p_buffer, size_t count, const wchar_t* p_format, ...);
void     UniFopen(FILE** fp_file, const char* p_filename, const char* p_modes);
size_t   SafeCsnlen(const wchar_t* p_str, size_t maxLength);
char*    WcharToUtf8(const wchar_t* wstr);
wchar_t* Utf8ToWchar(const char* str);

#endif  // _HEADER_WHID_UTILS_H