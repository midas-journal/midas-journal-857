#ifndef GLOBAL_H
#define GLOBAL_H

#define BEGIN_NAMESPACE namespace hyperflow {
#define END_NAMESPACE }
#define USING_NAMESPACE using namespace hyperflow;

BEGIN_NAMESPACE
void reportWarning(const char *format, ...);
void reportError(const char *format, ...);
END_NAMESPACE

#endif
