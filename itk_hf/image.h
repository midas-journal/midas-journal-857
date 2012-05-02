#ifndef _IMAGE_H
#define _IMAGE_H

#include <stdlib.h>
#include <stdint.h>

class Image {
public:

  Image(): width(0), height(0), data(0) {}

  ~Image() { this->hostDeallocate(); }

  void hostAllocate(int w, int h) {
    this->hostDeallocate();
    this->setSize(w, h);
    this->data = (uint32_t*)malloc(this->totalBytes());
  }

  void hostDeallocate() {
    if (!data) {
      free(this->data);
      this->setSize(0, 0);
      this->data = 0;
    }
  }

  void setSize(int w, int h) {
    this->width = w;
    this->height = h;
  }

  void setData(void *ptr) {
    this->data = (uint32_t*)ptr;
  }

  size_t totalBytes() const {
    return this->width*this->height*sizeof(uint32_t);
  }
  
  void readPNG(void *imageData, size_t dataSize);

  int width;
  int height;
  uint32_t *data;
};

#endif
