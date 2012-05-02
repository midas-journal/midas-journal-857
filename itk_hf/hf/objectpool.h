#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <stdlib.h>
#include "global.h"
#include "thread.h"
#include <vector>

BEGIN_NAMESPACE

#define USE_OBJECT_POOL 1

template<class Object>
class ObjectPool {
public:
  ObjectPool(size_t _capacity);
  ~ObjectPool();
  Object *create();
  void    free(Object *flow);
private:
#if USE_OBJECT_POOL

  void    allocate(size_t _capacity);
  
  size_t  capacity;
  int     curFreeIdx;
  int    *nextFreeIdx;
  Object *objs;
  Mutex   mutex;
  std::vector<Object*> dead;
#endif
};

template<class Object>
ObjectPool<Object>::ObjectPool(size_t _capacity) {
  this->nextFreeIdx = NULL;
  this->allocate(_capacity);
}

template<class Object>
void ObjectPool<Object>::allocate(size_t _capacity) {
#if USE_OBJECT_POOL
  this->capacity = _capacity;
  this->nextFreeIdx = (int*)realloc(this->nextFreeIdx, sizeof(int)*this->capacity);
  this->objs = new Object[this->capacity];
  this->curFreeIdx = 0;
  if (this->capacity>0) {
    for (size_t i=0; i<this->capacity-1; i++)
      this->nextFreeIdx[i] = i+1;
    this->nextFreeIdx[this->capacity-1] = -1;
  }
#endif
}

template<class Object>
ObjectPool<Object>::~ObjectPool() {
#if USE_OBJECT_POOL
  ::free(this->nextFreeIdx);
  delete [] this->objs;
  for (size_t i=0; i<this->dead.size(); i++)
    delete [] this->dead[i];
#endif
}

template<class Object>
Object* ObjectPool<Object>::create() {
#if USE_OBJECT_POOL
  this->mutex.lock();
  Object *flow = NULL;
  if (this->curFreeIdx<0) {
    reportWarning("ObjectPool::create() Maximum capacity of %d is reached. Grow double to %ld.",
                  this->capacity, this->capacity*2);
    this->dead.push_back(this->objs);
    this->allocate(this->capacity*2);
  }
  if (this->curFreeIdx>=0) {
    flow = this->objs + this->curFreeIdx;
    int next = this->nextFreeIdx[this->curFreeIdx];;
    this->nextFreeIdx[this->curFreeIdx] = -1;
    this->curFreeIdx = next;
  }
  else {
    reportError("ObjectPool::create() Invalid object creation");
  }
  this->mutex.unlock();
  return flow;
#else
  return new Object();
#endif
}

template<class Object>
void ObjectPool<Object>::free(Object *flow) {
#if USE_OBJECT_POOL
  this->mutex.lock();
  ptrdiff_t idx = flow-this->objs;
  if (idx>=0 && idx<(ptrdiff_t)this->capacity) {
    this->nextFreeIdx[idx] = this->curFreeIdx;
    this->curFreeIdx = idx;
  }
  this->mutex.unlock();
#else
  delete flow;
#endif
}

END_NAMESPACE

#endif
