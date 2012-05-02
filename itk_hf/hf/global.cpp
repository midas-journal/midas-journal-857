#include <global.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


BEGIN_NAMESPACE

void reportWarning(const char *format, ...) {
  va_list argList;
  va_start(argList, format);
  fprintf(stderr, "WARNING: ");
  vfprintf(stderr, format, argList);
  fprintf(stderr, "\n");
  va_end(argList);
}

void reportError(const char *format, ...) {
  va_list argList;
  va_start(argList, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, argList);
  fprintf(stderr, "\n");
  va_end(argList);
  exit(0);
}

END_NAMESPACE
