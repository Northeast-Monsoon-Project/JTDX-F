#ifndef JTDX_FM_DEBUG_FILE_HPP
#define JTDX_FM_DEBUG_FILE_HPP
#include <cstdio>
#include <cstdarg>
// A failed optional debug log must never crash radio control or change its branches.
#if defined(__GNUC__)
__attribute__((format(printf, 2, 3)))
#endif
inline int debugFilePrintf(FILE *file, const char *format, ...) {
  if (!file) return 0;
  va_list args; va_start(args, format);
  int result = vfprintf(file, format, args);
  va_end(args); return result;
}
inline void debugFileClose(FILE *file) { if (file) fclose(file); }
#endif
