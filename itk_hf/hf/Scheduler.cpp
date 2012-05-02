#include <Scheduler.h>
#include <Data.h>
#include <ExecutionEngine.h>
#include <Flow.h>
#include <TaskImplementation.h>
#include <TaskOrientedModule.h>
#include <VirtualProcessingElement.h>
#include <algorithm>

#define USE_TIME_STATS 1
#define USE_KEEP_GPU 1
#define USE_GPU_FIRST 0
#define SCHEDULING_INTERRUPTION 1

BEGIN_NAMESPACE

struct RunStat {
  RunStat(): runCount(0), avgTime(0.) {}
  int    runCount;
  double avgTime;
};

typedef HASH_MAP<VirtualProcessingElement*, RunStat> VPEStatHashMap;
typedef HASH_SET<VirtualProcessingElement*> VPEHashSet;

struct ModuleStat {
  ModuleStat(): cpuExclude(false) {}
  
  VPEStatHashMap gpuStat;
  RunStat        cpuStat;
  RunStat        avgStat;
  VPEHashSet     gpuExclude;
  bool           cpuExclude;
};

typedef HASH_MAP<TaskOrientedModule*, ModuleStat, TaskOrientedModuleHasher> ModuleStatHashMap;

static ModuleStatHashMap executionStats;

#define STORE_FLOW(flow)                                        \
  this->flowBuffer[flow->dstModule][flow->id].push_back(flow)

Scheduler::Scheduler(ExecutionEngine *e) {
  this->uniqueFlowId = 0;
  this->engine = e;
  this->schedulingHasChanged = false;
  this->idling = false;
}

int Scheduler::getNextFlowId() {
  return this->uniqueFlowId++;
}

struct lessFlowById: public std::binary_function<Flow*, Flow*, bool> {
  bool operator()(Flow *x, Flow *y) { return x->id<y->id; }
};

int Scheduler::scheduleNext(int n, VirtualProcessingElement **vpes) {
  this->mutexFlow.lock();
  this->schedulingHasChanged = false;
  FlowVector waitingQueueCopy = this->waitingQueue;
  this->waitingQueue.clear();
  this->mutexFlow.unlock();
  int numberOfScheduled = 0;
  std::sort(waitingQueueCopy.begin(), waitingQueueCopy.end(), lessFlowById());
  for (int i=0; i<(int)waitingQueueCopy.size(); i++) {
    Flow *flow = waitingQueueCopy[i];
    int impl = -1;
    VirtualProcessingElement *chosenVPE = NULL;
    CPTYPE cp = CP_NO;
    
    if (flow->shallow) {
      cp = this->findImplementation(flow->dstModule, flow->srcVPE, &impl);
      if (cp==CP_YES) {
        waitingQueueCopy.erase(waitingQueueCopy.begin()+i);
        i--;
        this->deployFlow(flow, impl, flow->srcVPE);
        numberOfScheduled++;
      }
      continue;
    }   
    
    if (this->canSkipUpdate(flow)) {
      STORE_FLOW(flow);
      waitingQueueCopy.erase(waitingQueueCopy.begin()+i);
      i--;
      continue;
    }

#if USE_KEEP_GPU && !USE_TIME_STATS
    if (flow->data && flow->data->getDataMedium()==DM_GPU_MEMORY) {
      cp = this->findImplementation(flow->dstModule, flow->srcVPE, &impl);
      if (cp==CP_YES) {
        chosenVPE = flow->srcVPE;
      }
      else if (cp==CP_BUSY) {
        if (numberOfScheduled>0 || this->delaySchedulingSet.find(flow)==this->delaySchedulingSet.end()) {
          this->delaySchedulingSet.insert(flow);
          continue;
        }
//         else
//           this->delaySchedulingSet.erase(flow);
      }
    }
#endif
    
    if (cp!=CP_YES) {
#if USE_TIME_STATS
      ModuleStat *stat = &(executionStats[flow->dstModule]);
      bool busy = false;
//       if (strcmp(flow->dstModule->getName(), "Pixel Threshold")==0)
//         fprintf(stderr, "STATISTICS %s : CPU(%.3lf) GPU(%.3lf)\n",
//                 flow->dstModule->getName(), stat.cpuTime, stat.gpuTime);
      for (int k=0; k<n; k++) {
        if (stat->cpuExclude && vpes[k]->resource.get(CR_THREAD)>0) continue;
        if (stat->gpuExclude.find(vpes[k])!=stat->gpuExclude.end()) continue;
        if (stat->gpuStat.size()==0 && vpes[k]->resource.get(CR_THREAD)>0) continue;
        cp = this->findImplementation(flow->dstModule, vpes[k], &impl);
        if (cp==CP_YES) {
          chosenVPE= vpes[k];
          break;
        }
        if (cp==CP_BUSY)
          busy = true;
      }
      
#if USE_KEEP_GPU
      if (cp!=CP_YES && !busy && flow->data && flow->data->getDataMedium()==DM_GPU_MEMORY) {
        cp = this->findImplementation(flow->dstModule, flow->srcVPE, &impl);
        if (cp==CP_YES) {
          chosenVPE = flow->srcVPE;
        }
        else if (cp==CP_BUSY) {
          if (numberOfScheduled>0 || this->delaySchedulingSet.find(flow)==this->delaySchedulingSet.end()) {
            this->delaySchedulingSet.insert(flow);
            continue;
          }
        }
      }
#endif
      
      if (cp!=CP_YES && !busy) {
#endif        
        
#if !USE_GPU_FIRST
      for (int k=0; k<n; k++) {
        cp = this->findImplementation(flow->dstModule, vpes[k], &impl);
        if (cp==CP_YES) {
          chosenVPE= vpes[k];
          break;
        }
      }
#else
      {
        for (int k=0; k<n; k++) {
          if (vpes[k]->resource.get(CR_GPU_DEVICE)==0) continue;
          cp = this->findImplementation(flow->dstModule, vpes[k], &impl);
          if (cp==CP_YES) {
            chosenVPE= vpes[k];
            break;
          }
        }
        if (cp!=CP_YES)
          for (int k=0; k<n; k++) {
            if (vpes[k]->resource.get(CR_GPU_DEVICE)>0) continue;
            cp = this->findImplementation(flow->dstModule, vpes[k], &impl);
            if (cp==CP_YES) {
              chosenVPE= vpes[k];
              break;
            }
          }
      }
#endif
      
#if USE_TIME_STATS
      }
//       if (cp==CP_YES) {
//         fprintf(stderr, "AVERAGE %s    CPU: %.6lf (%d)         GPU: %.6lf (%d)   PICK: ",
//               flow->dstModule->getName(),
//               stat.cpuTime, stat.cpuRun, stat.gpuTime, stat.gpuRun);
//         fprintf(stderr, "%s\n", chosenVPE->getName());
//       }
//       else
//         fprintf(stderr, "N/A\n");
#endif      
    }

#if SCHEDULING_INTERRUPTION
    if (schedulingHasChanged) break;
#endif
    
    if (cp==CP_YES) {
      STORE_FLOW(flow);
      waitingQueueCopy.erase(waitingQueueCopy.begin()+i);
      i--;
      this->deployFlow(flow, impl, chosenVPE);
      numberOfScheduled++;
    }
  }
  if (numberOfScheduled>0)
    this->delaySchedulingSet.clear();
  this->mutexFlow.lock();
  this->waitingQueue.insert(this->waitingQueue.end(),
                            waitingQueueCopy.begin(),
                            waitingQueueCopy.end());
  this->mutexRunning.lock();
  this->idling = this->runningSet.empty() && this->waitingQueue.empty() && !this->schedulingHasChanged &&
    this->delaySchedulingSet.empty();
  this->mutexRunning.unlock();
  this->mutexFlow.unlock();
  return numberOfScheduled;
}

void Scheduler::waitForSchedulingChange() {
  this->condChange.selfLock();
  if (!this->schedulingHasChanged) {
    this->condChange.selfWait();
  }
  this->condChange.selfUnlock();
}

bool Scheduler::isIdling() {
  return this->idling;
}

void Scheduler::addFlow(Flow *flow) {
  this->mutexFlow.lock();
  if (flow->id==-1)
    flow->id = this->getNextFlowId();
  this->waitingQueue.push_back(flow);
  this->idling = false;
  this->signalSchedulingChange();
  this->mutexFlow.unlock();
}

void Scheduler::addFlows(int n, Flow **flows) {
  if (n<1)
    return;
  this->mutexFlow.lock();
  int flowId = flows[0]->id;
  if (flowId==-1)
    flowId = this->getNextFlowId();
  for (int i=0; i<n; i++) {
    flows[i]->id = flowId;
    this->waitingQueue.push_back(flows[i]);
  }
  this->idling = false;
  this->signalSchedulingChange();
  this->mutexFlow.unlock();
}

void Scheduler::signalSchedulingChange() {
  this->condChange.selfLock();
  this->schedulingHasChanged = true;
  this->condChange.broadcast();
  this->condChange.selfUnlock();
}

bool Scheduler::isUpstream(TaskOrientedModule *up, TaskOrientedModule *down) {
  if (up==down)
    return true;
  ModulePair conn(up, down);
  ModulePairIntHashMap::iterator ti = this->cacheConnection.find(conn);
  if (ti!=this->cacheConnection.end()) {
    return (*ti).second!=0;
  } else {
    ModuleHashSet visited;
    ModuleQueue Q;
    Q.push(up);
    visited.insert(up);
    while (!Q.empty()) {
      TaskOrientedModule *src = Q.front();
      Q.pop();
      for (size_t i=0; i<src->getNumberOfOutputPorts(); i++) {
        PortVector *p = src->getOutputPort(i);
        for (size_t j=0; j<p->size(); j++) {
          TaskOrientedModule *dst = p->at(j).first;
          if (visited.find(dst)==visited.end()) {
            visited.insert(dst);
            Q.push(dst);
            this->cacheConnection[ModulePair(up, dst)] = 1;
            this->cacheConnection[ModulePair(src, dst)] = 1;
            if (dst==down)
              return true;
          }
        }
      }
    }
  }
  return false;
}

bool Scheduler::canSkipUpdate(Flow *f1) {
  ModuleFlowHashMap::iterator mi = this->flowBuffer.find(f1->dstModule);
  size_t nFlows = 0;
  if (mi!=this->flowBuffer.end()) {    
    IntFlowHashMap::iterator ii = (*mi).second.find(f1->id);
    if (ii!=(*mi).second.end())
      nFlows = (*ii).second.size();
  }
  if (nFlows+1<f1->dstModule->getNumberOfInputPorts()) {
    return true;
  }
  return false;
}

// void Scheduler::storeFlow(Flow *flow) {
//   this->flowBuffer[flow->dstModule][flow->id].push_back(flow);
// }

CPTYPE Scheduler::findImplementation(TaskOrientedModule *tom, VirtualProcessingElement *vpe, int *impl) {
  CPTYPE result = CP_NO;
  *impl = -1;
  for (size_t i=0; i<tom->getNumberOfImplementations(); i++) {
    CPTYPE cp = vpe->canProcess(tom->getImplementation(i));
    if (cp==CP_YES) {
      *impl = i;
      return CP_YES;
    }
    else if (cp==CP_BUSY)
      result = CP_BUSY;
  }
  return result;
}

class TransferredNotification {
public:
  SafeCounter               counter;
  int                       flowId;
  TaskOrientedModule*       updatingModule;
  Scheduler*                scheduler;
  VirtualProcessingElement* vpe;
};

void Scheduler::onFlowTransferred(void *arg) {
  TransferredNotification *tn = (TransferredNotification*)arg;
  if (tn->counter.decrease()==0) {
    Flow *flow      = tn->scheduler->engine->createFlow();
    flow->id        = tn->flowId;
    flow->srcVPE    = tn->vpe;
    flow->dstModule = tn->updatingModule;
    flow->shallow   = true;
    
    tn->vpe->unblockNewRequests();
    
    tn->scheduler->addFlow(flow);

//     tn->scheduler->mutexRunning.lock();
//     tn->scheduler->runningSet.erase(ModuleIntPair(tn->updatingModule, tn->flowId));
//     tn->scheduler->mutexRunning.unlock();
    
    delete tn;    
  }
}

void Scheduler::deployFlow(Flow *flow, int implId, VirtualProcessingElement *vpe) {
  ModuleFlowHashMap::iterator mi = this->flowBuffer.find(flow->dstModule);
  if (mi==this->flowBuffer.end())
    reportError("Scheduler::deployFlow: could not find flow dstModule.");
  IntFlowHashMap::iterator ii = (*mi).second.find(flow->id);
  if (ii==(*mi).second.end())
    reportError("Scheduler::deployFlow: could not find flow id.");
  TaskImplementation *impl = flow->dstModule->getImplementation(implId);

  FlowVector needTransfer;
  for (unsigned i=0; i<(*ii).second.size(); i++) {
    Flow *f = (*ii).second[i];
    if (f->data && f->data->getDataMedium()!=DM_CPU_MEMORY && f->srcVPE!=vpe)
      needTransfer.push_back(f);
  }

  if (needTransfer.size()>0) {
    TransferredNotification *tn = new TransferredNotification();
    tn->counter.increase(needTransfer.size());
    tn->flowId         = flow->id;
    tn->updatingModule = flow->dstModule;
    tn->vpe            = vpe;
    tn->scheduler      = this;
    vpe->blockNewRequests();
    for (unsigned i=0; i<needTransfer.size(); i++) {
      Flow *f = needTransfer[i];
      f->cbOnTransferred.add(Scheduler::onFlowTransferred, tn);
      
      this->mutexRunning.lock();
      this->runningSet[ModuleIntPair(flow->dstModule, flow->id)] = flow->srcVPE;
      this->mutexRunning.unlock();
      
      f->srcVPE->transferFlowToCPU(f);
    }
  }
  else {
    TaskData input;
    input.flows = (*ii).second;
    (*mi).second.erase(ii);
    ExecutionContext context;
    context.engine = this->engine;
    context.module = flow->dstModule;
    context.flowId = flow->id;
    this->mutexRunning.lock();
    this->runningSet[ModuleIntPair(context.module, context.flowId)] = vpe;
    this->mutexRunning.unlock();
    vpe->process(context, impl, input);
    if (flow->shallow)
      this->engine->freeFlow(flow);
  }
}

void Scheduler::doneProcessing(int n, Flow **flows, TaskOrientedModule *module, int flowId) {
  this->mutexFlow.lock();
  for (int i=0; i<n; i++) {
    if (flows[i]->id==-1)
      flows[i]->id = this->getNextFlowId();
    this->waitingQueue.push_back(flows[i]);
  }  
  this->idling = false;
  
  this->mutexRunning.lock();
  ModuleIntVPEHashMap::iterator rit = this->runningSet.find(ModuleIntPair(module, flowId));
  VirtualProcessingElement *vpe = (*rit).second;
  this->runningSet.erase(rit);
  this->mutexRunning.unlock();
  
  this->signalSchedulingChange();
  this->mutexFlow.unlock();

  ModuleStat *stat = &(executionStats[module]);  
  stat->avgStat.avgTime = (stat->avgStat.avgTime*stat->avgStat.runCount + vpe->currentRequest->time)/(stat->avgStat.runCount+1);
  stat->avgStat.runCount++;
  
  if (vpe->resource.get(CR_THREAD)>0) {
    stat->cpuStat.avgTime = (stat->cpuStat.avgTime*stat->cpuStat.runCount + vpe->currentRequest->time)/(stat->cpuStat.runCount+1);
    stat->cpuStat.runCount++;
    if (stat->cpuStat.avgTime/stat->avgStat.avgTime>1.5)
      stat->cpuExclude = true;
  }
  if (vpe->resource.get(CR_GPU_DEVICE)>0) {
    RunStat *gStat = &(stat->gpuStat[vpe]);
    gStat->avgTime = (gStat->avgTime*gStat->runCount + vpe->currentRequest->time)/(gStat->runCount+1);
    gStat->runCount++;
    if (gStat->avgTime/stat->avgStat.avgTime>1.5)
      stat->gpuExclude.insert(vpe);
  }

//   if (strcmp(module->getName(), "Gaussian Blur")==0)
//     fprintf(stderr, "Run on %s in %.3lf vs. %.3lf\n", vpe->getName(),
//             vpe->currentRequest->time, stat->avgStat.avgTime);
  
}

END_NAMESPACE
