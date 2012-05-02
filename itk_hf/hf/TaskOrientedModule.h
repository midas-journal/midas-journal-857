#ifndef TASK_ORIENTED_MODULE_H
#define TASK_ORIENTED_MODULE_H

#include "global.h"
#include "stltypes.h"
#include <vector>
#include <string>

BEGIN_NAMESPACE

class TaskImplementation;

class TaskOrientedModule {
public:
  TaskOrientedModule(int nInput=0, int nOutput=0, const std::string &name=std::string());
  virtual ~TaskOrientedModule();

  const char* getName();
  void setName(const char *name);
  
  void   setNumberOfInputPorts(size_t n);
  size_t getNumberOfInputPorts() const {
    return this->numberOfInputPorts;
  }
  PortVector *getInputPort(size_t k) {
    return &(this->inputPorts[k]);
  }
  
  void   setNumberOfOutputPorts(size_t n);
  size_t getNumberOfOutputPorts() const {
    return this->numberOfOutputPorts;
  }
  PortVector *getOutputPort(size_t k) {
    return &(this->outputPorts[k]);
  }  

  void   setNumberOfImplementations(size_t n);
  size_t getNumberOfImplementations() const {
    return this->implementations.size();
  }
  
  int  addImplementation(TaskImplementation *taskImpl);
  void setImplementation(unsigned i, TaskImplementation *taskImpl);
  TaskImplementation* getImplementation(unsigned i) const;

  void connectFrom(TaskOrientedModule* srcModule, int srcPort,
                   int dstPort);

  static void connect(TaskOrientedModule* srcModule, int srcPort,
                      TaskOrientedModule* dstModule, int dstPort);
  
private:
  size_t               numberOfInputPorts;
  size_t               numberOfOutputPorts;
  PortVector*          inputPorts;
  PortVector*          outputPorts;
  ImplementationVector implementations;
  std::string          moduleName;
  
  TaskOrientedModule(const TaskOrientedModule&);  // Not implemented.
  void operator=(const TaskOrientedModule&);  // Not implemented.
};

inline void TaskOrientedModule::connectFrom(TaskOrientedModule* srcModule, int srcPort,
                                       int dstPort) {
  TaskOrientedModule::connect(srcModule, srcPort, this, dstPort);
}

END_NAMESPACE

#endif
