#include <ComputingResource.h>

BEGIN_NAMESPACE

ComputingResource::ComputingResource() {
  for (int i=0; i<CR_MAX; i++)
    this->amount[i] = 0;
}

END_NAMESPACE
