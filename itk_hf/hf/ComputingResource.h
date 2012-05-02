#ifndef COMPUTING_RESOURCE_H
#define COMPUTING_RESOURCE_H

#include "global.h"

BEGIN_NAMESPACE

enum CRTYPE {
  CR_THREAD = 0,
  CR_GPU_DEVICE = 1,
  CR_MAX = 16
};

class ComputingResource {
public:
  ComputingResource();
  void set(CRTYPE type, int n) { this->amount[type] = n; }
  int  get(CRTYPE type) const { return this->amount[type]; }
private:
  int amount[CR_MAX];
};

END_NAMESPACE

#endif
