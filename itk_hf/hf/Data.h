#ifndef DATA_H
#define DATA_H

#include "global.h"
#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

BEGIN_NAMESPACE

class VirtualProcessingElement;

enum DMTYPE {
  DM_CPU_MEMORY = 0,
  DM_GPU_MEMORY = 1,
};

// ========================= Data =========================
class Data {
public:
  Data();
  virtual ~Data() {}
  virtual Data* createInstance(DMTYPE dm, VirtualProcessingElement *vpe);
  virtual void releaseResource(VirtualProcessingElement *vpe) {}

  void setDataMedium(DMTYPE dm);
  DMTYPE getDataMedium();
  
  int getRefCount();
  int increaseRef();
  int decreaseRef();
  
protected:
  DMTYPE  dataMedium;
  int     refCount;
  Mutex   mutex;
};

// ========================= SimpleData =========================
class SimpleData: public Data {
public:
  SimpleData(size_t size=0);
  ~SimpleData();
  
  int& asInt();
  float& asFloat();
  
  static SimpleData* createInt(int value);
  static SimpleData* createFloat(float value);
  static SimpleData* createFromVoidPtr(size_t size, void *data, bool passOwnership);

  size_t  size;
  void   *pointer;
  bool    owned;
};

// ========================= Inline functions =========================
inline void Data::setDataMedium(DMTYPE dm) {
  this->dataMedium = dm;
}

inline DMTYPE Data::getDataMedium() {
  return this->dataMedium;
}

inline int& SimpleData::asInt() {
  return *((int*)this->pointer);
}

inline SimpleData* SimpleData::createInt(int value) {
  SimpleData *data = new SimpleData(sizeof(int));
  data->asInt() = value;
  return data;
}

inline float& SimpleData::asFloat() {
  return *((float*)this->pointer);
}

inline SimpleData* SimpleData::createFloat(float value) {
  SimpleData *data = new SimpleData(sizeof(float));
  data->asFloat() = value;
  return data;
}

inline SimpleData* SimpleData::createFromVoidPtr(size_t inSize, void *voidPtr, bool passOwnership) {
  SimpleData *data = new SimpleData(0);
  data->size = inSize;
  data->pointer = voidPtr;
  data->owned = passOwnership;
  return data;
}

END_NAMESPACE

#endif
