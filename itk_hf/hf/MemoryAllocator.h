#ifndef MEMORY_ALLOCATOR_VPE_H
#define MEMORY_ALLOCATOR_VPE_H

#include "global.h"
#include "stltypes.h"
#include <stdint.h>

BEGIN_NAMESPACE

class MemoryAllocator {
public:
  virtual ~MemoryAllocator() {}
  virtual void *alloc(size_t size)=0;
  virtual bool free(void *ptr)=0;
};

class BlockMemoryAllocator: public MemoryAllocator {
public:

  BlockMemoryAllocator(void *ptr, size_t size, size_t sBlock);

  void *alloc(size_t size);
  bool free(void *ptr);
  
  size_t getBlockSize() const { return this->blockSize; }
  unsigned getNumberOfFreeBlocks() const { return this->freeBlocks.size(); }
  unsigned getNumberOfBlocks() const { return this->totalBlock; }

private:
  size_t     blockSize;
  unsigned   totalBlock;
  uint8_t*   data;
  
  typedef HASH_SET<unsigned> IntHashSet;
  IntHashSet freeBlocks;
  std::vector<unsigned> allocatedBlock;
};

END_NAMESPACE

#endif
