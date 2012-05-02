#include <Data.h>

BEGIN_NAMESPACE

Data::Data() {
  this->refCount = 1;
  this->setDataMedium(DM_CPU_MEMORY);
}

int Data::increaseRef() {
  this->mutex.lock();
  int r = ++this->refCount;
  this->mutex.unlock();
  return r;
}

int Data::decreaseRef() {
  this->mutex.lock();  
  int r = this->refCount>0?--this->refCount:0;
  this->mutex.unlock();
  return r;
}

int Data::getRefCount() {
  this->mutex.lock();
  int r = this->refCount;
  this->mutex.unlock();
  return r;
}

Data* Data::createInstance(DMTYPE medium, VirtualProcessingElement *) {
  if (this->dataMedium==medium) {
    this->increaseRef();
    return this;
  }
  return NULL;
}

SimpleData::SimpleData(size_t s) {
  if (s>0) {
    this->size = s;
    this->pointer = malloc(this->size);
    this->owned = true;
    this->setDataMedium(DM_CPU_MEMORY);
  }
  else {
    this->pointer = 0;
    this->size = 0;
    this->owned = false;
    this->setDataMedium(DM_CPU_MEMORY);
  }
}

SimpleData::~SimpleData() {
  if (this->owned && this->pointer) {
    free(this->pointer);
    this->pointer = 0;
  }
  this->size = 0;
}

END_NAMESPACE
