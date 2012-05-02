#ifndef STL_TYPES_H
#define STL_TYPES_H

#include "global.h"
#include <vector>
#include <deque>
#include <queue>

#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)  // 4.1 or later... might work on previous versions too
#define HYPERFLOW_USE_TR1
#endif

#ifndef HYPERFLOW_USE_TR1
#include <ext/hash_map>
#include <ext/hash_set>
#define HASH_MAP __gnu_cxx::hash_map
#define HASH_SET __gnu_cxx::hash_set
#else
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define HASH_MAP std::tr1::unordered_map
#define HASH_SET std::tr1::unordered_set
#endif

BEGIN_NAMESPACE

class Data;
class Flow;
class ModulePairHasher;
class ModuleIntPairHasher;
class TaskImplementation;
class TaskOrientedModule;
class VirtualProcessingElement;
template <class Object> class PointerHasher;

typedef PointerHasher<Flow> FlowHasher;
typedef PointerHasher<TaskOrientedModule> TaskOrientedModuleHasher;
typedef void(*VoidPtrFunction)(void*);

typedef std::vector<VirtualProcessingElement*> VPEVector;
typedef std::vector<Flow*> FlowVector;
typedef std::deque<Flow*> FlowDeque;
typedef std::queue<Flow*> FlowQueue;
typedef std::vector<Data*> DataVector;
typedef std::vector<void*> VoidPtrVector;
typedef std::pair<TaskOrientedModule*, TaskOrientedModule*> ModulePair;
typedef std::pair<TaskOrientedModule*, int> ModuleIntPair;
typedef std::queue<TaskOrientedModule*> ModuleQueue;
typedef std::pair<TaskOrientedModule*,int> Port;
typedef std::vector<TaskImplementation*> ImplementationVector;
typedef std::vector<Port> PortVector;
typedef std::vector<VoidPtrFunction> VoidPtrFunctionVector;

typedef HASH_MAP<int, FlowVector> IntFlowHashMap;
typedef HASH_MAP<TaskOrientedModule*, IntFlowHashMap, TaskOrientedModuleHasher> ModuleFlowHashMap;
typedef HASH_MAP<ModulePair, int, ModulePairHasher> ModulePairIntHashMap;
typedef HASH_SET<Flow*, FlowHasher> FlowHashSet;
typedef HASH_SET<TaskOrientedModule*, TaskOrientedModuleHasher> ModuleHashSet;
typedef HASH_MAP<ModuleIntPair, VirtualProcessingElement*, ModuleIntPairHasher> ModuleIntVPEHashMap;

class ModulePairHasher
{
public:
  size_t operator()(const ModulePair & e) const 
  {
    return ((size_t)e.first&0xFFFFFFFF)*((size_t)e.second&0xFFFFFFFF);
  }
};

class ModuleIntPairHasher
{
public:
  size_t operator()(const ModuleIntPair & e) const 
  {
    return ((size_t)e.first&0xFFFFFFFF)*((size_t)e.second);
  }
};

template <class Object>
class PointerHasher
{
public:
  size_t operator()(const Object* e) const 
  {
    return reinterpret_cast<size_t>(e);
  }
};

END_NAMESPACE

#endif
