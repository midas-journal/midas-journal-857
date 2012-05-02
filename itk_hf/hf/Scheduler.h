#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "global.h"
#include "stltypes.h"
#include "thread.h"
#include "VirtualProcessingElement.h"

BEGIN_NAMESPACE

class EEMainThread;
class ExecutionEngine;
class Flow;
class TaskData;
class TaskOrientedModule;

class Scheduler {
public:
  Scheduler(ExecutionEngine *_engine);
  
  bool isIdling();
  
  int  scheduleNext(int n, VirtualProcessingElement**);
  
  void addFlow(Flow *flow);
  
  void addFlows(int n, Flow **flows);

  void waitForSchedulingChange();

  void signalSchedulingChange();

protected:
  int  getNextFlowId();
  bool isUpstream(TaskOrientedModule *up, TaskOrientedModule *down);
  bool canSkipUpdate(Flow *flow);
  void storeFlow(Flow *flow);
  CPTYPE findImplementation(TaskOrientedModule *tom, VirtualProcessingElement *vpe, int *impl);
  void deployFlow(Flow *flow, int implId, VirtualProcessingElement *vpe);
  void doneProcessing(int n, Flow **flows, TaskOrientedModule *module, int flowId);
  
  static void onFlowTransferred(void *arg);
private:
  ExecutionEngine*     engine;
  FlowVector           waitingQueue;
  ModuleIntVPEHashMap  runningSet;
  FlowHashSet          delaySchedulingSet;
  ModulePairIntHashMap cacheConnection;
  ModuleFlowHashMap    flowBuffer;
  int                  uniqueFlowId;
  Mutex                mutexFlow;
  Mutex                mutexRunning;
  WaitCondition        condChange;
  bool                 schedulingHasChanged;
  bool                 idling;
  friend class EEMainThread;
  friend class ExecutionEngine;
};

END_NAMESPACE

#endif
