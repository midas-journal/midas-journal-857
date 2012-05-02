#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "global.h"
#include "stltypes.h"
#include "thread.h"
#include "objectpool.h"

BEGIN_NAMESPACE

class EEMainThread;
class Flow;
class Scheduler;
class TaskData;
class TaskOrientedModule;
class VirtualProcessingElement;
typedef ObjectPool<Flow> FlowPool;

enum RETTYPE {
  RET_IMMEDIATELY= 0,
  RET_ON_FREE = 1,
  RET_ON_DEPLOY = 2,
};

class ExecutionEngine {
public:
  ExecutionEngine();
  ~ExecutionEngine();

  void setNumberOfVPEs(unsigned n);
  unsigned getNumberOfVPEs() const;  
  unsigned addVPE(VirtualProcessingElement *vpe);
  void setVPE(unsigned i, VirtualProcessingElement *vpe);
  VirtualProcessingElement* getVPE(unsigned i) const;
  VirtualProcessingElement* getMemVPE() const;
  
  void quickConfig(unsigned numberOfThreads, unsigned numberOfGPUDevices);
  void quickConfig();
  void start();
  void stop();

  Flow* createFlow(Flow *refFlow=NULL, bool copyData=false);
  Flow* createFlow(TaskOrientedModule *dstModule, int dstPort, Data *data=NULL);
  void  freeFlow(Flow *flow);

  void  sendUpdate(TaskOrientedModule *module, RETTYPE ret=RET_IMMEDIATELY);
  void  sendFlow(Flow *flow, RETTYPE ret);
  void  sendFlows(int n, Flow **flows, RETTYPE ret, bool retOnAny=true);
  void  doneProcessing(int n, Flow **flows, TaskOrientedModule *module, int flowId);
  void  signalSchedulingChange();

  void  waitForAllDone();
  void  waitForAvailable();
  
protected:
  void updateStatus();
  static void onNotification(void *cond);
  static void onMultiNotification(void *sentNotification);
  
private:
  bool                     running;
  bool                     mainThreadReady;
  VPEVector                vpes;
  
  WaitCondition            condAllDone;
  WaitCondition            condMainThreadReady;
  
  Scheduler                *scheduler;
  EEMainThread             *mainThread;
  FlowPool                 *flowPool;  
  VirtualProcessingElement *memVPE;

  friend class EEMainThread;
};

class EEMainThread: public Thread {
public:
  EEMainThread(ExecutionEngine *ee);
  virtual void run();
private:
  ExecutionEngine *engine;
};

inline unsigned ExecutionEngine::getNumberOfVPEs() const {
  return this->vpes.size();
}

END_NAMESPACE

#endif
