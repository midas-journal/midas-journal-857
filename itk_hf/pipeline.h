#ifndef PIPELINE_H
#define PIPELINE_H
#include <hyperflow.h>

USING_NAMESPACE
class QImageViewer;

void *runPipeline(const char *inputFile, ExecutionEngine *engine, QImageViewer *viewer, bool itk, int ncpu);

#endif
