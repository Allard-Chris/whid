#include "whid_utils.h"

void ConvertSecondsToTime(u32 totalSeconds, wchar_t* p_buffer) {
  u32 remainingSeconds = totalSeconds;
  u32 hours, minutes, seconds;

  hours = remainingSeconds / 3600;
  remainingSeconds = remainingSeconds - (hours * 3600);
  minutes = remainingSeconds / 60;
  remainingSeconds = remainingSeconds - (minutes * 60);
  seconds = remainingSeconds;
  UniSwprintf(p_buffer, MAX_STRING_SIZE, L"%02d:%02d:%02d", (int)hours, (int)minutes, (int)seconds);
}

void RemoveCR(wchar_t* p_buffer) {
  int cursor = 0;
  for (wchar_t* head = p_buffer; *head != L'\0'; head++) {
    if (p_buffer[cursor] == L'\n') {
      p_buffer[cursor] = L'\0';
    }
    cursor += 1;
  }
}

wchar_t* StrncpyTruncate(wchar_t* p_dst, size_t szDest, const wchar_t* p_src) {
  assert(szDest > 0);
// get size of the src (we truncate to force null caracter at the end).
#if defined(WIN32) || defined(_WIN32)
  size_t szSrc = wcsnlen_s(p_src, szDest - 1);
  assert(szDest > szSrc);
  memmove_s(p_dst, szDest, p_src, szSrc * sizeof(wchar_t));
#endif
#ifdef __linux__
  size_t szSrc = wcsnlen(p_src, szDest - 1);
  assert(szDest > szSrc);
  memmove(p_dst, p_src, szSrc * sizeof(wchar_t));
#endif
  p_dst[szSrc] = '\0';
  return p_dst;
}

int UniSwprintf(wchar_t* p_buffer, size_t count, const wchar_t* p_format, ...) {
  int     result = 0;
  va_list args;
  va_start(args, p_format);

#if defined(WIN32) || defined(_WIN32)
  result = vswprintf_s(p_buffer, MAX_STRING_SIZE, p_format, args);
#endif
#ifdef __linux__
  result = vswprintf(p_buffer, MAX_STRING_SIZE, p_format, args);
#endif

  va_end(args);
  return result;
}

void UniFopen(FILE** fp_file, const char* p_filename, const char* p_modes) {
#if defined(WIN32) || defined(_WIN32)
  fopen_s(fp_file, p_filename, p_modes);
#endif
#ifdef __linux__
  *fp_file = fopen(p_filename, p_modes);
#endif
  return;
}

size_t SafeCsnlen(const wchar_t* p_str, size_t maxLength) {
  size_t length = 0;
  while (length < maxLength && p_str[length] != '\0') {
    length++;
  }
  return length;
}

#if defined(_WIN32) || defined(WIN32)
char* WcharToUtf8(const wchar_t* p_wcstr) {
  if (!p_wcstr) {
    return NULL;
  }
  size_t src_length = wcslen(p_wcstr);
  int    length = WideCharToMultiByte(CP_UTF8, 0, p_wcstr, (int)src_length, 0, 0, NULL, NULL);
  char*  output_buffer = (char*)malloc((length + 1) * sizeof(char));
  if (output_buffer) {
    WideCharToMultiByte(CP_UTF8, 0, p_wcstr, (int)src_length, output_buffer, length, NULL, NULL);
    output_buffer[length] = '\0';
  }
  return output_buffer;
}

wchar_t* Utf8ToWchar(const char* p_src) {
  if (!p_src) {
    return NULL;
  }

  size_t   srcLength = strlen(p_src);
  int      length = MultiByteToWideChar(CP_UTF8, 0, p_src, (int)srcLength, 0, 0);
  wchar_t* p_buffer = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
  if (p_buffer) {
    MultiByteToWideChar(CP_UTF8, 0, p_src, (int)srcLength, p_buffer, length);
    p_buffer[length] = L'\0';
  }

  return p_buffer;
}
#endif
#ifdef __linux__
char* WcharToUtf8(const wchar_t* p_wcstr) {
  if (!p_wcstr) {
    return NULL;
  }

  // Get the length of the output string
  size_t nbValue;
  nbValue = wcstombs(NULL, p_wcstr, 0) + 1;  // +1 for the null terminator
  if (nbValue == (size_t)-1) {
    return NULL;
  }

  // Allocate memory for the UTF-8 string
  char* p_mbstr = (char*)malloc(nbValue);
  if (p_mbstr == NULL) {
    return NULL;
  }

  // Perform the conversion
  if (nbValue > MAX_SIZE) {
    free(p_mbstr);
    return NULL;
  } else {
    wcstombs(p_mbstr, p_wcstr, nbValue);
  }

  return p_mbstr;
}

wchar_t* Utf8ToWchar(const char* p_src) {
  if (!p_src) {
    return NULL;
  }

  int      length = mbstowcs(NULL, p_src, MAX_SIZE);
  wchar_t* p_buffer = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
  if (p_buffer) {
    mbstowcs(p_buffer, p_src, length);
    p_buffer[length] = L'\0';
  }

  return p_buffer;
}
#endif

void UniLocaltime(struct tm** fp_tmDest, const time_t* p_time) {
#if defined(_WIN32) && defined(WIN32)
  localtime_s(*fp_tmDest, p_time);
#endif
#if defined(__linux__)
  *fp_tmDest = localtime(p_time);
#endif
}