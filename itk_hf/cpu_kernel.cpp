#include "cpu_kernel.h"
#include <stdio.h>

#include <cstring>

static const float gauss_kernel_cpu[3][3] = {
  {0.0625, 0.125, 0.0625},
  {0.125, 0.25, 0.125},
  {0.0625, 0.125, 0.0625}
};

#define CONVOLVE(I, J, K)                                    \
  r += ((in.data[I]>>0) & 0xFF)*gauss_kernel_cpu[J][K];      \
  g += ((in.data[I]>>8) & 0xFF)*gauss_kernel_cpu[J][K];      \
  b += ((in.data[I]>>16) & 0xFF)*gauss_kernel_cpu[J][K];  

void gauss_cpu(const Image &in, Image &out)
{
  unsigned width = in.width;
  unsigned height = in.height;

  for (unsigned i = 0; i < width; ++i) {
    for (unsigned j = 0; j < height; ++j) {
      unsigned left = (i == 0) ? 0 : i-1,
		right = (i == width-1) ? width-1 : i+1,
		down = (j == 0) ? 0 : j-1,
		up = (j == height-1) ? height-1 : j+1;

      float r=0.0, g=0.0, b=0.0;
      
      CONVOLVE(width*down+left, 0, 0);
      CONVOLVE(width*down+i, 0, 1);
      CONVOLVE(width*down+right, 0, 2);
      CONVOLVE(width*j+left, 1, 0);
      CONVOLVE(width*j+i, 1, 1);
      CONVOLVE(width*j+right, 1, 2);
      CONVOLVE(width*up+left, 2, 0);
      CONVOLVE(width*up+i, 2, 1);
      CONVOLVE(width*up+right, 2, 2);
      
      out.data[width*j+i] = ((uint32_t)r) + (((uint32_t)g)<<8) + (((uint32_t)b)<<16);
    }
  }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void blend_diff_cpu(const Image &im1, const Image &im2, Image &out)
{
  unsigned width = im1.width;
  unsigned height = im1.height;
  
  for (unsigned i=0; i<width*height; i++) {
    int v1 = im1.data[i];
    int v2 = im2.data[i];
    int r = (int)((v1>>0) & 0xFF) - ((v2>>0) & 0xFF);
    int g = (int)((v1>>8) & 0xFF) - ((v2>>8) & 0xFF);
    int b = (int)((v1>>16) & 0xFF) - ((v2>>16) & 0xFF);
    out.data[i] = (r<0?-r:r) + ((g<0?-g:g)<<8) + ((b<0?-b:b)<<16);
  }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void invert_cpu(const Image &in, Image &out)
{
  unsigned width = in.width;
  unsigned height = in.height;

  for (unsigned i = 0; i<width*height; i++) {
    out.data[i] = (~in.data[i])&0xFFFFFF;
  }    
}

void threshold_cpu(const Image &in, Image &out, float pMin, float pMax)
{
  unsigned width = in.width;
  unsigned height = in.height;

  // computes histogram
  unsigned hist[256] = {};
  for (unsigned i = 0; i<width*height; i++) {
    uint32_t val = in.data[i];
    uint8_t grey = (uint8_t)((val&0xFF)*0.3 + ((val>>8)&0xFF)*0.59 + ((val>>16)&0xFF)*0.11);
    ++hist[grey];
  }

  // accumulate the histogram
  for (unsigned i=1; i<256; i++)
    hist[i] += hist[i-1];
  
  unsigned yMin = (unsigned)(pMin*hist[255]);
  unsigned yMax = (unsigned)(pMax*hist[255]);

  // applies the threshold
  for (unsigned i = 0; i<width*height; ++i) {
    uint32_t val = in.data[i];
    uint8_t grey = (uint8_t)((val&0xFF)*0.3 + ((val>>8)&0xFF)*0.59 + ((val>>16)&0xFF)*0.11);
    if (hist[grey]>=yMin && hist[grey]<=yMax)
      out.data[i] = 0xFFFFFF;
    else
      out.data[i] = 0;
  }
}
