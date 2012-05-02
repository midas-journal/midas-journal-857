#include <Flow.h>
#include <Data.h>
#include <VirtualProcessingElement.h>

BEGIN_NAMESPACE

Flow::Flow() {
  this->data = NULL;
  this->clear();
}

void Flow::clear() {
  this->releaseData();
  this->id        = -1;
  this->srcVPE    = NULL;
  this->srcModule = NULL;
  this->srcPort   = -1;
  this->dstModule = NULL;
  this->dstPort   = -1;
  this->shallow   = false;
  this->cbOnFree.clear();
  this->cbOnDeploy.clear();
  this->cbOnTransferred.clear();
}

void Flow::copyInfoFrom(Flow *flow) {
  this->id        = flow->id;
  this->srcVPE    = flow->srcVPE;
  this->srcModule = flow->srcModule;
  this->srcPort   = flow->srcPort;
  this->dstModule = flow->dstModule;
  this->dstPort   = flow->dstPort;
  this->shallow   = false;
  this->cbOnFree.clear();
  this->cbOnDeploy.clear();
  this->cbOnTransferred.clear();
}

void Flow::copyDataFrom(Data *data) {
  data->increaseRef();
  this->data = data;
}

void Flow::releaseData() {
  if (this->data) {
    if (this->data->decreaseRef()==0)
      this->srcVPE->deleteData(this->data);
    this->data = NULL;
  }
}

END_NAMESPACE
