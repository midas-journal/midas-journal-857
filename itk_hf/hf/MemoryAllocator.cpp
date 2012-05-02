#include <MemoryAllocator.h>

BEGIN_NAMESPACE

BlockMemoryAllocator::BlockMemoryAllocator(void *ptr, size_t size, size_t sBlock) {
  this->blockSize = sBlock;
  this->totalBlock = size/this->blockSize;
  this->data = (uint8_t*)ptr;
  this->freeBlocks.clear();
  this->allocatedBlock.resize(this->totalBlock, 0);
  for (unsigned i=0; i<this->totalBlock; i++)
    this->freeBlocks.insert(i);
}

void* BlockMemoryAllocator::alloc(size_t size) {
  unsigned numBlock = (size+(this->blockSize-1))/this->blockSize;
  if (numBlock>this->freeBlocks.size()) {
    return NULL;
  }
  
  int block = -1;
  
  if (numBlock==1) {
    IntHashSet::iterator it = this->freeBlocks.begin();
    block = *it;
    this->allocatedBlock[block] = numBlock;
    this->freeBlocks.erase(it);
  }
  else {
    for (unsigned i=0; i<this->totalBlock-numBlock; i++) {
      bool ok = true;
      for (unsigned j=0; j<numBlock; j++) {
        if (this->allocatedBlock[i+j]>0) {
          i += j;
          ok = false;
          break;
        }
      }
      if (ok) {
        block = i;
        for (unsigned j=0; j<numBlock; j++) {
          this->allocatedBlock[block+j] = numBlock;
          this->freeBlocks.erase(block+j);
        }
        break;
      }
    }
  }
  
  if (block!=-1)
    return this->data+this->blockSize*block;

  return NULL;
}

bool BlockMemoryAllocator::free(void *ptr) {
  unsigned i = ((uint8_t*)ptr-this->data)/this->blockSize;
  if (ptr<data || i>=this->totalBlock)
    return false;
  unsigned numBlock = this->allocatedBlock[i];
  for (unsigned j=0; j<numBlock; j++) {
    this->freeBlocks.insert(i+j);
    this->allocatedBlock[i+j] = 0;
  }
  return true;
}

END_NAMESPACE
