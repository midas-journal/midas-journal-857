#ifndef VIRTUAL_PROCESSING_ELEMENT_H
#define VIRTUAL_PROCESSING_ELEMENT_H

#include <string>
#include "global.h"
#include "callback.h"
#include "thread.h"
#include "ComputingResource.h"
#include "TaskImplementation.h"

BEGIN_NAMESPACE

class Data;
class Flow;
class VPEMainThread;
class Scheduler;

class RequestData {
public:
  ExecutionContext context;
  TaskImplementation *impl;
  TaskData input;
  TaskData output;
  double   time;
};

enum CPTYPE {
  CP_NO = 0,
  CP_YES = 1,
  CP_BUSY = 2,
};

class VirtualProcessingElement {
public:
  VirtualProcessingElement();
  virtual ~VirtualProcessingElement();
  const char* getName() const;
  void setName(const std::string &name);

  virtual void start(); 
  virtual void waitUntilStarted();
  virtual void stop();
  
  virtual void mainLoop();
  
  virtual CPTYPE canProcess(TaskImplementation *impl);
  virtual void process(ExecutionContext context, TaskImplementation *impl, const TaskData &input);
  
  virtual void deleteData(Data *data);  
  virtual void transferFlowToCPU(Flow *srcFlow);
  virtual void *localAlloc(size_t size);
  virtual void localFree(void *ptr);

  virtual void initialize() {}
  virtual void finalize() {}
  
  virtual void enterExecution() {}
  virtual void leaveExecution() {}

  void blockNewRequests();
  void unblockNewRequests();
  void printInfo();

  RequestData* getCurrentRequest();
  Callback* getBeforeExecutionCallback();
  Callback* getAfterExecutionCallback();

  ComputingResource resource;

protected:
  void performCPUTransfer();
  bool localizeCurrentInput();
  void releaseCurrentInput();
  virtual void emitOutputAndComplete();

  std::string       deviceName;
  
  VPEMainThread*    mainThread;
  RequestData*      currentRequest;
  bool              currentRequestValid;
  bool              processing;
  bool              stopping;
  bool              firstWait;
  bool              acceptRequests;

  WaitCondition     condNewRequest;
  
  FlowVector        transferRequests;
  FlowVector        transferRequestsCopy;
  
  WaitCondition     condMainThreadReady;
  
  Callback*         cbBeforeExecution;
  Callback*         cbAfterExecution;

  friend class Scheduler;
};

class VPEMainThread: public Thread {
public:
  VPEMainThread(VirtualProcessingElement *vpe) { this->self = vpe; }
  virtual void run() { this->self->mainLoop(); }
private:
  VirtualProcessingElement *self;
};

inline const char* VirtualProcessingElement::getName() const {
  return this->deviceName.c_str();
}

inline void VirtualProcessingElement::setName(const std::string &name) {
  this->deviceName = name;
}

inline void* VirtualProcessingElement::localAlloc(size_t size) {
  return malloc(size);
}

inline void VirtualProcessingElement::localFree(void *ptr) {
  free(ptr);
}

inline RequestData* VirtualProcessingElement::getCurrentRequest() {
  return this->currentRequest;
}

inline Callback* VirtualProcessingElement::getBeforeExecutionCallback() {
  return this->cbBeforeExecution;
}

inline Callback* VirtualProcessingElement::getAfterExecutionCallback() {
  return this->cbAfterExecution;
}

END_NAMESPACE

#endif
