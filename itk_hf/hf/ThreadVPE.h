#ifndef THREAD_VPE_H
#define THREAD_VPE_H

#include "global.h"
#include "VirtualProcessingElement.h"

BEGIN_NAMESPACE

class TaskImplementation;

class ThreadVPE: public VirtualProcessingElement {
public:
  ThreadVPE(int threadId);
  ThreadVPE();

  virtual void initialize();

  static int getNumCPUs();
private:
  int threadId;
  void setAffinity();
  bool bindToCore;
};

END_NAMESPACE

#endif
