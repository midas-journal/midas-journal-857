#include <VirtualProcessingElement.h>
#include <Data.h>
#include <ExecutionEngine.h>
#include <Flow.h>
#include <TaskImplementation.h>
#include <TaskOrientedModule.h>
#include <thread.h>

#include <sys/time.h>

BEGIN_NAMESPACE

double WALLCLOCK() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec*1e-6;
}

VirtualProcessingElement::VirtualProcessingElement() {
  this->mainThread = new VPEMainThread(this);
  this->currentRequestValid = false;
  this->processing = true;
  this->stopping = false;
  this->firstWait = true;
  this->acceptRequests = true;
  this->currentRequest = new RequestData();
  this->cbBeforeExecution = new Callback();
  this->cbAfterExecution = new Callback();
}

VirtualProcessingElement::~VirtualProcessingElement() {
  this->mainThread->terminate();
  delete this->mainThread;
  delete this->currentRequest;
  delete this->cbBeforeExecution;
  delete this->cbAfterExecution;
}

void VirtualProcessingElement::start() {
  this->processing = false;
  this->stopping = false;
  this->firstWait = true;
  this->condMainThreadReady.selfLock();
  this->mainThread->start();
//   this->condMainThreadReady.selfWait();
//   this->condMainThreadReady.selfUnlock();
}

void VirtualProcessingElement::waitUntilStarted() {
  this->condMainThreadReady.selfWait();
  this->condMainThreadReady.selfUnlock();
}

void VirtualProcessingElement::stop() {
  this->condNewRequest.selfLock();
  // We want to force the thread to halt if it is processing a
  // request. This sounds wrong but if stop() gets called when the VPE
  // is processing something, there is no 'clean' finalization without
  // waiting and we don't want to wait.
  if (false && this->processing) {
    reportWarning("VPE %s gets terminated while executing %s", this->getName(),
                  this->currentRequest->context.module->getName());
    this->condNewRequest.selfUnlock();
    this->mainThread->terminate();
    this->stopping = true;
  } else {
    this->stopping = true;
    this->condNewRequest.signal();
    this->condNewRequest.selfUnlock();
    if (this->mainThread->isRunning())
      this->mainThread->wait();
  }
}

void VirtualProcessingElement::mainLoop() {
  this->initialize();
  
  this->condMainThreadReady.selfLock();
  this->condMainThreadReady.broadcast();
  this->condMainThreadReady.selfUnlock();
  bool isProcessing = false;
  while (!this->stopping) {    
    this->condNewRequest.selfLock();
    if (this->firstWait)
      this->firstWait = false;
    else if (isProcessing) {
      this->currentRequest->context.engine->signalSchedulingChange();
      this->processing = false;
    }
    if (this->transferRequests.size()==0 && !this->processing)
      this->condNewRequest.selfWait();
    this->transferRequestsCopy = this->transferRequests;
    this->transferRequests.clear();
    isProcessing = this->processing;
    this->condNewRequest.selfUnlock();
    
    if (this->stopping)
      break;
    
    if (isProcessing)
      this->cbBeforeExecution->trigger();
    
    this->enterExecution();
    
    this->performCPUTransfer();
    
    if (isProcessing) {
      for (unsigned i=0; i<this->currentRequest->input.getNumberOfFlows(); i++) {
        this->currentRequest->input.getFlow(i)->cbOnDeploy.trigger();
      }
      this->currentRequest->time = WALLCLOCK();
      if (this->localizeCurrentInput()) {
        //       fprintf(stderr, "%s: %s with flow Id: %d\n", this->getName(),
        //               this->currentRequest->context.module->getName(),
        //               this->currentRequest->input.getFlow(0)->id);
        {
          this->currentRequest->impl->process(&this->currentRequest->context,
                                              &this->currentRequest->input,
                                              &this->currentRequest->output);
        }
      
        this->releaseCurrentInput();
        this->emitOutputAndComplete();
      }
    }
    
    this->leaveExecution();
    
    if (isProcessing) {
      this->cbAfterExecution->trigger();
    }
  }
  this->finalize();
  
}

CPTYPE VirtualProcessingElement::canProcess(TaskImplementation *impl) {
  if (!this->acceptRequests)
    return CP_NO;
  bool can=true, isProcessing=false;
  for (int i=0; i<CR_MAX; i++) {
    if (impl->getResource()->get((CRTYPE)i)>this->resource.get((CRTYPE)i)) {
      can = false;
      break;
    }
  }
  this->condNewRequest.selfLock();
  isProcessing = this->processing;
  this->condNewRequest.selfUnlock();
  if (can)
    return isProcessing?CP_BUSY:CP_YES;
  else
    return CP_NO;
}

void VirtualProcessingElement::process(ExecutionContext context, TaskImplementation *impl, const TaskData &input) {
  this->condNewRequest.selfLock();
  if (this->processing)
    reportError("INCORRECT PROCESSING");
  
//   if (this->resource.get(CR_GPU_DEVICE)>0)
//     fprintf(stderr, "%s SCHEDULE %s with flowId: %d\n",
//             this->getName(),
//             context.module->getName(),
//             context.flowId);
  this->currentRequest->context = context;
  this->currentRequest->context.vpe = this;
  this->currentRequest->impl = impl;
  this->currentRequest->input = input;
  this->currentRequest->output.flows.clear();
  this->processing = true;
  this->condNewRequest.signal();
  this->condNewRequest.selfUnlock();
}

void VirtualProcessingElement::performCPUTransfer() {
  if (this->transferRequestsCopy.size()>0) {
    for (unsigned i=0; i<this->transferRequestsCopy.size(); i++) {
      Flow *flow = this->transferRequestsCopy[i];
      if (flow->data->getDataMedium()!=DM_CPU_MEMORY) {
        VirtualProcessingElement *memVPE = this->currentRequest->context.engine->getMemVPE();
        Data *localData = flow->data->createInstance(DM_CPU_MEMORY, memVPE);
        flow->releaseData();
        flow->srcVPE = memVPE;
        flow->data = localData;
        flow->cbOnTransferred.trigger();
      }
    }
  }
}

void VirtualProcessingElement::transferFlowToCPU(Flow *flow) {
  this->condNewRequest.selfLock();
  this->transferRequests.push_back(flow);
  this->condNewRequest.signal();
  this->condNewRequest.selfUnlock();
}

bool VirtualProcessingElement::localizeCurrentInput() {
  for (unsigned i=0; i<this->currentRequest->input.getNumberOfFlows(); i++) {
    Flow *flow = this->currentRequest->input.getFlow(i);
    if (!this->currentRequest->impl->ensureInputData(&this->currentRequest->context, flow))
      return false;
  }
  return true;
}

void VirtualProcessingElement::releaseCurrentInput() {
  ExecutionEngine *engine = this->currentRequest->context.engine;
  for (size_t i=0; i<this->currentRequest->input.flows.size(); i++) {
    engine->freeFlow(this->currentRequest->input.flows[i]);
  }
}

void VirtualProcessingElement::emitOutputAndComplete() {
  this->currentRequest->time = WALLCLOCK()-this->currentRequest->time;
  
  FlowVector downFlow;
  ExecutionEngine *engine = this->currentRequest->context.engine;
  TaskData *output = &(this->currentRequest->output);
  for (size_t fid=0; fid<output->flows.size(); fid++) {
    Flow *flow = output->flows[fid];
    PortVector *plist = flow->srcModule->getOutputPort(flow->srcPort);
    if (plist->size()==0) {
      engine->freeFlow(flow);
      continue;
    }
    flow->dstModule = plist->at(0).first;
    flow->dstPort = plist->at(0).second;
    downFlow.push_back(flow);
    for (size_t i=1; i<plist->size(); i++) {
      Flow *repFlow = engine->createFlow(flow, true);
      repFlow->dstModule = plist->at(i).first;
      repFlow->dstPort = plist->at(i).second;
      downFlow.push_back(repFlow);
    }
  }
  
  engine->doneProcessing(downFlow.size(), downFlow.size()>0?(&(downFlow[0])):NULL,
                         this->currentRequest->context.module,
                         this->currentRequest->context.flowId);
}

void VirtualProcessingElement::deleteData(Data *data) {
  data->releaseResource(this);
  delete data;
}

void VirtualProcessingElement::blockNewRequests() {
  this->acceptRequests = false;
}

void VirtualProcessingElement::unblockNewRequests() {
  this->acceptRequests = true;
}

void VirtualProcessingElement::printInfo() {
  fprintf(stderr, "%s %d %d\n", this->getName(), this->processing, this->acceptRequests);
}

END_NAMESPACE
