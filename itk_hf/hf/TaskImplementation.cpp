#include <TaskImplementation.h>
#include <ExecutionEngine.h>
#include <Flow.h>
#include <VirtualProcessingElement.h>
#include <stdio.h>
#include <algorithm>

BEGIN_NAMESPACE

struct lessFlowDstPort: public std::binary_function<Flow*, Flow*, bool> {
  bool operator()(Flow *x, Flow *y) { return x->dstPort<y->dstPort; }
};

void TaskData::sortByDstPort() {
  std::sort(this->flows.begin(), this->flows.end(), lessFlowDstPort());
}

Flow* ExecutionContext::createFlowForOutputPort(int port, Flow *refFlow) {
  Flow *flow = this->engine->createFlow(refFlow);
  flow->srcModule = this->module;
  flow->srcPort = port;
  flow->srcVPE = this->vpe;
  return flow;
}

TaskImplementation::TaskImplementation() {
  this->setLocalMedium(DM_CPU_MEMORY);
}

bool TaskImplementation::ensureInputData(ExecutionContext *context, Flow *flow) {
  if (flow->data && flow->data->getDataMedium()!=this->localMedium) {
    if (this->localMedium==DM_GPU_MEMORY) {
      Data *localData = flow->data->createInstance(this->localMedium, context->vpe);
      if (!localData) {
//         reportWarning("TaskImplementation::localizeFlow: cannot localize data\n");
        return false;
      }
      flow->releaseData();
      flow->data = localData;
      flow->srcVPE = context->vpe;
    }
    else {
      flow->srcVPE->transferFlowToCPU(flow);
    }
  }
  return true;
}

const char*
TaskImplementation::name() const
{
    return "TaskImplementation";
}


END_NAMESPACE
