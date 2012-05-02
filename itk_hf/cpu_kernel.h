#ifndef _CPU_KERNEL_H
#define _CPU_KERNEL_H

#include "image.h"

// this file include three functions to compute various image
// transformations.  all transformations are done on the CPU and are
// pipeline-based; that is, input is always constant and output is
// always a new image instance, allocated on cpu memory

// computes a 3x3 gaussian blur, takes im as input and generates out
// as output.  out can be an empty image; in this case it will be
// allocated inside the function
void gauss_cpu(const Image &in, Image &out);

// computes im1 - im2, and stores it in out. out can be an empty
// image; in this case it will be allocated inside the function
void blend_diff_cpu(const Image &im1, const Image &im2, Image &out);

// inverts in, and stores the result in out. out can be an empty
// image; in this case it will be allocated inside the function
void invert_cpu(const Image &in, Image &out);

// thresholds in, stores the result in out. This is done by computing
// the histogram for the image, and then discarding all pixels whose
// occurence is outside the range [rmin, rmax). That is, if a pixel
// with quantized value v0 occurs more frequently than rmax or less
// frequently than rmin, it will be discarded
void threshold_cpu(const Image &in, Image &out, float pMin, float pMax);

#endif //_CPU_KERNEL_H
