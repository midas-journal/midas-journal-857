#ifndef TASK_IMPLEMENTATION_H
#define TASK_IMPLEMENTATION_H

#include "global.h"
#include "stltypes.h"
#include "ComputingResource.h"
#include "Data.h"

BEGIN_NAMESPACE

class ExecutionEngine;
class Flow;
class TaskOrientedModule;
class VirtualProcessingElement;

class TaskData {
public:
  FlowVector flows;

  void sortByDstPort();

  size_t getNumberOfFlows();
  void   addFlow(Flow *flow);
  Flow*  getFlow(unsigned idx);
};

class ExecutionContext {
public:
  ExecutionEngine*          engine;
  VirtualProcessingElement* vpe;
  TaskOrientedModule       *module;
  int                       flowId;

  Flow* createFlowForOutputPort(int port, Flow *refFlow=NULL);
};

class TaskImplementation {
public:
  TaskImplementation();
  virtual ~TaskImplementation() {}
  virtual bool ensureInputData(ExecutionContext *context, Flow *flow);
  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) = 0;

  ComputingResource *getResource();
  void               setLocalMedium(DMTYPE medium);
  DMTYPE             getLocalMedium();
  
  virtual const char* name() const;

protected:
  ComputingResource   resource;
  DMTYPE              localMedium;
};

inline size_t TaskData::getNumberOfFlows() {
  return this->flows.size();
}

inline void TaskData::addFlow(Flow *flow) {
  this->flows.push_back(flow);
}

inline Flow* TaskData::getFlow(unsigned idx) {
  return flows[idx];
}

inline ComputingResource *TaskImplementation::getResource() {
  return &this->resource;
}

inline DMTYPE TaskImplementation::getLocalMedium() {
  return this->localMedium;
}

inline void TaskImplementation::setLocalMedium(DMTYPE medium) {
  this->localMedium = medium;
}


END_NAMESPACE

#endif
