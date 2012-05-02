#include <TaskOrientedModule.h>
#include <TaskImplementation.h>

BEGIN_NAMESPACE

TaskOrientedModule::TaskOrientedModule(int nInput, int nOutput, const std::string &name) {
  this->numberOfInputPorts = 0;
  this->inputPorts = NULL;
  this->numberOfOutputPorts = 0;
  this->outputPorts = NULL;
  this->moduleName = name;

  this->setNumberOfInputPorts(nInput);
  this->setNumberOfOutputPorts(nOutput);
}

TaskOrientedModule::~TaskOrientedModule() {
  this->setNumberOfInputPorts(0);
  this->setNumberOfOutputPorts(0);
}

const char* TaskOrientedModule::getName() {
  return this->moduleName.c_str();
}

void TaskOrientedModule::setName(const char* name) {
  this->moduleName = name;
}

void TaskOrientedModule::setNumberOfInputPorts(size_t n) {
  if (this->numberOfInputPorts==n)
    return;

  if (this->numberOfInputPorts>0)
    delete [] this->inputPorts;

  this->numberOfInputPorts = n;
  if (this->numberOfInputPorts==0)
    this->inputPorts = NULL;
  else
    this->inputPorts = new PortVector[this->numberOfInputPorts];
}

void TaskOrientedModule::setNumberOfOutputPorts(size_t n) {
  if (this->numberOfOutputPorts==n)
    return;

  if (this->numberOfOutputPorts>0)
    delete [] this->outputPorts;

  this->numberOfOutputPorts = n;
  if (this->numberOfOutputPorts==0)
    this->outputPorts = NULL;
  else
    this->outputPorts = new PortVector[this->numberOfOutputPorts];
}

void TaskOrientedModule::connect(TaskOrientedModule* srcModule, int srcPort,
                                        TaskOrientedModule* dstModule, int dstPort) {
  srcModule->outputPorts[srcPort].push_back(Port(dstModule, dstPort));
  dstModule->inputPorts[dstPort].push_back(Port(srcModule, srcPort));
}

void TaskOrientedModule::setNumberOfImplementations(size_t n) {
  this->implementations.resize(n);
}

int TaskOrientedModule::addImplementation(TaskImplementation *taskImpl) {
  this->implementations.push_back(taskImpl);
  return this->implementations.size()-1;
}

void TaskOrientedModule::setImplementation(unsigned i, TaskImplementation *taskImpl) {
  if (i>=this->implementations.size())
    reportError("setImplementation: Implementation %d does not exist.", i);
  this->implementations[i] = taskImpl;
}

TaskImplementation* TaskOrientedModule::getImplementation(unsigned i) const {
  if (i>=this->implementations.size())
    reportError("getImplementation: Implementation %d does not exist.", i);
  return this->implementations[i];
}

END_NAMESPACE
