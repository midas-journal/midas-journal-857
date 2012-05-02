#ifndef FLOW_H
#define FLOW_H

#include "global.h"
#include "callback.h"
#include "stltypes.h"
#include "ComputingResource.h"
#include <vector>
#include <queue>

BEGIN_NAMESPACE

class Data;
class SimpleData;
class VirtualProcessingElement;
class TaskOrientedModule;

class Flow {
public:

  Flow();
  void clear();
  void releaseData();
  void copyInfoFrom(Flow* flow);
  void copyDataFrom(Data* data);

  int                       id;
  Data*                     data;
  
  VirtualProcessingElement* srcVPE;
  TaskOrientedModule*       srcModule;
  int                       srcPort;
  
  TaskOrientedModule*       dstModule;
  int                       dstPort;

  bool                      shallow;

  Callback                  cbOnFree;
  Callback                  cbOnDeploy;
  Callback                  cbOnTransferred;
};

END_NAMESPACE

#endif
