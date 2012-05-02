#include <ExecutionEngine.h>
#include <Data.h>
#include <Flow.h>
#include <Scheduler.h>
#include <TaskOrientedModule.h>
#include <ThreadVPE.h>

BEGIN_NAMESPACE

ExecutionEngine::ExecutionEngine() {
  this->running = false;
  this->flowPool = new FlowPool(2048);
  this->memVPE = new ThreadVPE();
  this->scheduler = new Scheduler(this);
  this->mainThread = new EEMainThread(this);
}

ExecutionEngine::~ExecutionEngine() {
  this->mainThread->terminate();
  delete this->mainThread;
  delete this->scheduler;
  delete this->memVPE;
  delete this->flowPool;
}

void ExecutionEngine::setNumberOfVPEs(unsigned n) {
  this->vpes.resize(n);
}

unsigned ExecutionEngine::addVPE(VirtualProcessingElement *vpe) {
  this->vpes.push_back(vpe);
  return this->vpes.size()-1;
}

void ExecutionEngine::setVPE(unsigned i, VirtualProcessingElement *vpe) {
  if (i>=this->vpes.size())
    reportError("setVPE: VPE %d does not exist.", i);
  this->vpes[i] = vpe;
}

VirtualProcessingElement* ExecutionEngine::getVPE(unsigned i) const {
  if (i>=this->vpes.size())
    reportError("getVPE: VPE %d does not exist.", i);
  return this->vpes[i];
}

VirtualProcessingElement* ExecutionEngine::getMemVPE() const {
  return this->memVPE;
}

void ExecutionEngine::quickConfig(unsigned numberOfThreads, unsigned numberOfGPUDevices) {
  this->setNumberOfVPEs(0);
  for (unsigned i=0; i<numberOfThreads; i++)
    this->addVPE(new ThreadVPE(i));
}

void ExecutionEngine::quickConfig() {
  int ncpus = ThreadVPE::getNumCPUs();
  int ngpus = 0;
  quickConfig(ncpus, ngpus);
}

void ExecutionEngine::start() {
  if (this->running) {
    reportWarning("ExecutionEngine::start: The engine has already started.");
    return;
  }
  this->running = true;
  for (size_t i=0; i<this->vpes.size(); i++) {
    this->vpes[i]->start();
  }
  for (size_t i=0; i<this->vpes.size(); i++) {
    this->vpes[i]->waitUntilStarted();
  }
  this->mainThreadReady = false;
  this->condMainThreadReady.selfLock();
  this->mainThread->start();
  this->condMainThreadReady.selfWait();
  this->condMainThreadReady.selfUnlock();
  
#ifdef USE_PAPI
  if (PAPI_library_init(PAPI_VER_CURRENT)!=PAPI_VER_CURRENT)
    reportError("PAPI_library_init failed.");
#endif
}

void ExecutionEngine::stop() {
  if (!this->running) {
    reportWarning("ExecutionEngine::stop: The engine is not running.");
    return;
  }
  this->running = false;
  for (size_t i=0; i<this->vpes.size(); i++) {
    this->vpes[i]->stop();
  }
  this->mainThread->terminate();
}

Flow* ExecutionEngine::createFlow(Flow *refFlow, bool copyData) {
  Flow *flow = this->flowPool->create();
  if (refFlow) {
    flow->copyInfoFrom(refFlow);
    if (copyData)
      flow->copyDataFrom(refFlow->data);
  }
  else {
    flow->clear();
    flow->srcVPE = this->memVPE;
  }
  return flow;
}

Flow* ExecutionEngine::createFlow(TaskOrientedModule *dstModule, int dstPort, Data *data) {
  Flow *flow = this->flowPool->create();
  flow->clear();
  flow->dstModule = dstModule;
  flow->dstPort = dstPort;
  flow->data = data;
  flow->srcVPE = this->memVPE;
  return flow;
}

void ExecutionEngine::freeFlow(Flow *flow) {
  flow->cbOnFree.trigger();
  flow->clear();
  this->flowPool->free(flow);
}

void ExecutionEngine::waitForAllDone() {
  if (!this->scheduler->isIdling())
    this->condAllDone.wait();
}

void ExecutionEngine::updateStatus() {
  if (this->scheduler->isIdling())
    this->condAllDone.broadcast();
}

void ExecutionEngine::doneProcessing(int n, Flow **flows, TaskOrientedModule *module, int flowId) {
  this->scheduler->doneProcessing(n, flows, module, flowId);
}

void ExecutionEngine::signalSchedulingChange() {
  this->scheduler->signalSchedulingChange();
}

void ExecutionEngine::onNotification(void *arg) {
  WaitCondition *cond = (WaitCondition*)arg;
  cond->selfLock();
  cond->signal();
  cond->selfUnlock();
}

void ExecutionEngine::sendUpdate(TaskOrientedModule *module, RETTYPE ret) {
  this->sendFlow(this->createFlow(module, -1), ret);
}

void ExecutionEngine::sendFlow(Flow *flow, RETTYPE ret) {
  WaitCondition *cond;
  switch (ret) {
  case RET_IMMEDIATELY:
    this->scheduler->addFlow(flow);
    break;
  case RET_ON_FREE:
  case RET_ON_DEPLOY:
    cond = new WaitCondition();
    if (ret==RET_ON_FREE)
      flow->cbOnFree.add(ExecutionEngine::onNotification, cond);
    else if (ret==RET_ON_DEPLOY)
      flow->cbOnDeploy.add(ExecutionEngine::onNotification, cond);
    cond->selfLock();
    this->scheduler->addFlow(flow);
    cond->selfWait();
    cond->selfUnlock();
    delete cond;
    break;
  default:
    break;
  }
}

class SentNotification {
public:
  int           leftUntilDelete;
  int           leftUntilSignal;
  WaitCondition cond;
};

void ExecutionEngine::onMultiNotification(void *arg) {
  SentNotification *sn = (SentNotification*)arg;
  bool needDelete = false;
  sn->cond.selfLock();
  if ((--sn->leftUntilDelete)==0)
    needDelete = true;
  if ((--sn->leftUntilSignal)==0)
    sn->cond.signal();
  sn->cond.selfUnlock();
  if (needDelete)
    delete sn;
}

void ExecutionEngine::sendFlows(int n, Flow **flows, RETTYPE ret, bool retOnAny) {
  SentNotification *sn;
  bool needDelete = false;
  switch (ret) {
  case RET_IMMEDIATELY:
    this->scheduler->addFlows(n, flows);
    break;
  case RET_ON_FREE:
  case RET_ON_DEPLOY:
    sn = new SentNotification();
    sn->leftUntilDelete = n+1;
    if (retOnAny)
      sn->leftUntilSignal = 1;
    else
      sn->leftUntilSignal = n;
    for (int i=0; i<n; i++) {
      if (ret==RET_ON_FREE)
        flows[i]->cbOnFree.add(ExecutionEngine::onMultiNotification, sn);
      else if (ret==RET_ON_DEPLOY)
        flows[i]->cbOnDeploy.add(ExecutionEngine::onMultiNotification, sn);
    }
    sn->cond.selfLock();
    this->scheduler->addFlows(n, flows);
    sn->cond.selfWait();
    if ((--sn->leftUntilDelete)==0)
      needDelete = true;
    sn->cond.selfUnlock();
    if (needDelete)
      delete sn;
    break;
  default:
    break;
  }
}

EEMainThread::EEMainThread(ExecutionEngine *ee) {
  this->engine = ee;
}

void EEMainThread::run() {
  ExecutionEngine *self = this->engine;
  while (self->running) {
    self->scheduler->scheduleNext(self->vpes.size(), &(self->vpes[0]));
    if (self->mainThreadReady)
      self->updateStatus();
    else {
      self->mainThreadReady = true;
      self->condMainThreadReady.selfLock();
      self->condMainThreadReady.broadcast();
      self->condMainThreadReady.selfUnlock();
    }
    self->scheduler->waitForSchedulingChange();
  }
}

END_NAMESPACE
