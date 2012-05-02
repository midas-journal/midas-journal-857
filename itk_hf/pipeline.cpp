#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <sys/time.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hyperflow.h>
#include "image.h"
#include "cpu_kernel.h"
#if USE_QT
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtGui/QImage>
#include "QImageViewer.h"
#else
class QImageViewer;
#endif

#include "itkGaussianBlurImageFunction.h"
#include "itkImageRegionIterator.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkInvertIntensityImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

USING_NAMESPACE

static bool useITK = false;

double WALLCLOCK() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec*1e-6;
}

class DStringData : public Data {
public:

  DStringData(const std::string &v) {
    this->value = v;
  }
  
  std::string value;
};

class DImageData : public Data {
public:

  DImageData(DMTYPE medium=DM_CPU_MEMORY) {
    this->setDataMedium(medium);
  }
  
  DImageData(void *imageData, size_t size) {
    this->image.readPNG(imageData, size);
    this->setDataMedium(DM_CPU_MEMORY);
  }

  void vpeAllocImage(int width, int height, VirtualProcessingElement *vpe) {
    this->image.setSize(width, height);
    this->image.setData(vpe->localAlloc(this->image.totalBytes()));
  }

  void releaseResource(VirtualProcessingElement *vpe) {
    if (this->image.data) {
      vpe->localFree(this->image.data);
      this->image.setData(0);
    }
  }

  Data* createInstance(DMTYPE medium, VirtualProcessingElement *vpe) {
    if (this->dataMedium==DM_GPU_MEMORY || medium==DM_GPU_MEMORY) {
      reportError("GPU implementation is not included.");
    }
    else 
      return Data::createInstance(medium, vpe);
  }

  Image image;
};

class IImageFetch : public TaskImplementation {
public:
  IImageFetch() {
    this->resource.set(CR_THREAD, 1);
  }

  const char* getName() const { return "IImageFetch"; }

  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) {
    SimpleData *inData = ((SimpleData*)input->flows[0]->data);
    Flow *flow = context->createFlowForOutputPort(0, input->flows[0]);
    flow->data = new DImageData(inData->pointer, inData->size);
    output->addFlow(flow);
    return 0;
  }
  
};

class MGaussianBlur : public TaskOrientedModule {
public:
  MGaussianBlur(int iter): TaskOrientedModule(1, 1, "Gaussian Blur") {
    this->iterations = iter;
  }

  int iterations;
};

class ICPUImageBlur : public TaskImplementation {
public:
  ICPUImageBlur() {
    this->resource.set(CR_THREAD, 1);
  }
  
  const char* getName() const { return "ICPUImageBlur"; }

  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) {
    DImageData *inData = ((DImageData*)input->flows[0]->data);
    DImageData *outData = new DImageData(DM_CPU_MEMORY);
    outData->vpeAllocImage(inData->image.width, inData->image.height, context->vpe);

    // typedef itk::RGBAPixel<unsigned char> PixelType;
    if (useITK)
    {
      typedef itk::Image<uint32_t, 2> ImageType; 
      typedef itk::DiscreteGaussianImageFilter<ImageType, ImageType >  FilterType;
      ImageType::SizeType size = {inData->image.width, inData->image.height};
      ImageType::IndexType start = {};
      ImageType::RegionType region;
      region.SetSize(size);
      region.SetIndex(start);
      ImageType::Pointer inImage = ImageType::New();
      inImage->SetRegions(region);
      inImage->Allocate();
      uint32_t *buf = inImage->GetBufferPointer();
      memcpy(buf, inData->image.data, inData->image.width*inData->image.height*sizeof(uint32_t));
      // Create and setup a Gaussian filter
      FilterType::Pointer gaussianFilter = FilterType::New();
      gaussianFilter->SetInput(inImage);
      gaussianFilter->SetVariance(3);
      gaussianFilter->Update();
      ImageType::Pointer outImage = gaussianFilter->GetOutput();
      memcpy(outData->image.data, outImage->GetBufferPointer(), inData->image.width*inData->image.height*sizeof(uint32_t));
    }
    else {
      gauss_cpu(inData->image, outData->image);
    }

    Flow *flow = context->createFlowForOutputPort(0, input->flows[0]);
    flow->data = outData;
    output->addFlow(flow);
    return 0;
  }
   
};

class ICPUImageDiff : public TaskImplementation {
public:
  ICPUImageDiff() {
    this->resource.set(CR_THREAD, 1);
  }

  const char* getName() const { return "ICPUImageDiff"; }

  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) {
    input->sortByDstPort();
    DImageData *inData1 = (DImageData*)input->flows[0]->data;
    DImageData *inData2 = (DImageData*)input->flows[1]->data;
    DImageData *outData = new DImageData(DM_CPU_MEMORY);
    outData->vpeAllocImage(inData1->image.width, inData1->image.height, context->vpe);
    
    blend_diff_cpu(inData1->image, inData2->image, outData->image);
    
    Flow *flow = context->createFlowForOutputPort(0, input->flows[0]);
    flow->data = outData;
    output->addFlow(flow);
    return 0;
  }
   
};

class ICPUImageInvert : public TaskImplementation {
public:
  ICPUImageInvert() {
    this->resource.set(CR_THREAD, 1);
  }
  
  const char* getName() const { return "ICPUImageInvert"; }

  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) {
    DImageData *inData = ((DImageData*)input->flows[0]->data);
    DImageData *outData = new DImageData();
    outData->vpeAllocImage(inData->image.width, inData->image.height, context->vpe);
    
    if (1)
    {
      typedef itk::Image<uint32_t, 2> ImageType; 
      typedef itk::InvertIntensityImageFilter<ImageType >  FilterType;
      ImageType::SizeType size = {inData->image.width, inData->image.height};
      ImageType::IndexType start = {};
      ImageType::RegionType region;
      region.SetSize(size);
      region.SetIndex(start);
      ImageType::Pointer inImage = ImageType::New();
      inImage->SetRegions(region);
      inImage->Allocate();
      uint32_t *buf = inImage->GetBufferPointer();
      memcpy(buf, inData->image.data, inData->image.width*inData->image.height*sizeof(uint32_t));
      FilterType::Pointer invertFilter = FilterType::New();
      invertFilter->SetInput(inImage);
      invertFilter->Update();
      ImageType::Pointer outImage = invertFilter->GetOutput();
      memcpy(outData->image.data, outImage->GetBufferPointer(), inData->image.width*inData->image.height*sizeof(uint32_t));
    }
    else {
      invert_cpu(inData->image, outData->image);
    }
    
    Flow *flow = context->createFlowForOutputPort(0, input->flows[0]);
    flow->data = outData;
    output->addFlow(flow);
    return 0;
  }
   
};

class MImageThreshold : public TaskOrientedModule {
public:
  MImageThreshold(float min, float max): TaskOrientedModule(1, 1, "Pixel Threshold") {
    this->pMin = min;
    this->pMax = max;
  }

  float pMin;
  float pMax;
};

class ICPUImageThreshold : public TaskImplementation {
public:
  ICPUImageThreshold() {
    this->resource.set(CR_THREAD, 1);
  }
  
  const char* getName() const { return "ICPUImageThreshold"; }

  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) {
    MImageThreshold *module = (MImageThreshold*)context->module;
    DImageData *inData = ((DImageData*)input->flows[0]->data);
    DImageData *outData = new DImageData();
    outData->vpeAllocImage(inData->image.width, inData->image.height, context->vpe);
    
    if (useITK)
    {
      typedef itk::Image<uint32_t, 2> ImageType; 
      typedef itk::BinaryThresholdImageFilter<ImageType, ImageType>  FilterType;
      ImageType::SizeType size = {inData->image.width, inData->image.height};
      ImageType::IndexType start = {};
      ImageType::RegionType region;
      region.SetSize(size);
      region.SetIndex(start);
      ImageType::Pointer inImage = ImageType::New();
      inImage->SetRegions(region);
      inImage->Allocate();
      uint32_t *buf = inImage->GetBufferPointer();
      memcpy(buf, inData->image.data, inData->image.width*inData->image.height*sizeof(uint32_t));
      FilterType::Pointer thresholdFilter = FilterType::New();
      thresholdFilter->SetLowerThreshold(module->pMin);
      thresholdFilter->SetUpperThreshold(module->pMax);
      thresholdFilter->SetInput(inImage);
      thresholdFilter->Update();
      ImageType::Pointer outImage = thresholdFilter->GetOutput();
      memcpy(outData->image.data, outImage->GetBufferPointer(), inData->image.width*inData->image.height*sizeof(uint32_t));
    }
    else {
      threshold_cpu(inData->image, outData->image, module->pMin, module->pMax);
    }

    Flow *flow = context->createFlowForOutputPort(0, input->flows[0]);
    flow->data = outData;
    output->addFlow(flow);
    return 0;
  }
   
};

class IImageContrast : public TaskImplementation {
public:
  IImageContrast(QImageViewer *v) {
    this->resource.set(CR_THREAD, 1);
    this->viewer = v;
  }
  
  const char* getName() const { return "IImageContrast"; }

  virtual int process(ExecutionContext *context, TaskData *input, TaskData *output) {
    input->sortByDstPort();
#if USE_QT

    DImageData *inImage = ((DImageData*)input->flows[0]->data);
    DImageData *origImage = ((DImageData*)input->flows[1]->data);

    int x1 = ((SimpleData*)input->flows[2]->data)->asInt() - this->xOff;
    int y1 = ((SimpleData*)input->flows[3]->data)->asInt() - this->yOff;

    int width = inImage->image.width;
    int height = inImage->image.height;

    QImage img(width, height, QImage::Format_RGB32);
    uint32_t *in = (uint32_t*)origImage->image.data;
    for (int y=0; y<height; y++) {
      memcpy(img.scanLine(y), in+y*width, width*sizeof(uint32_t));
    }

    int subWidth = (int)ceil(width * this->scale);
    int subHeight = (int)ceil(height * this->scale);
    int subX = (int)(x1 * this->scale);
    int subY = (int)(y1 * this->scale);
    
    QImage *outOrigImage = new QImage(img.scaled(subWidth, subHeight,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation));
    QCoreApplication::postEvent(this->viewer, new QPlaceImageEvent(outOrigImage, subX, subY));

    subY = (int)(y1 * this->scale + 0.5 * this->targetHeight);

    in = (uint32_t*)inImage->image.data;
    for (int y=0; y<height; y++) {
      memcpy(img.scanLine(y), in+y*width, width*sizeof(uint32_t));
    }
    QImage *outImage = new QImage(img.scaled(subWidth, subHeight,
                                             Qt::IgnoreAspectRatio,
                                             Qt::SmoothTransformation));
    unsigned avg[3] = {};
    for (int y=0; y<outImage->height(); y++) {
      uint8_t *u = outImage->scanLine(y);
      for (int x=0; x<outImage->width(); x++) {
        avg[0] += u[x*4+0];
        avg[1] += u[x*4+1];
        avg[2] += u[x*4+2];
      }
    }
    avg[0] /= subWidth*subHeight;
    avg[1] /= subWidth*subHeight;
    avg[2] /= subWidth*subHeight;

    int base = 240;
    
    if (avg[0]+avg[1]+avg[2]==0)
      base = 255;
    for (int y=0; y<subHeight; y++) {
      uint8_t *u = outImage->scanLine(y);
      for (int x=0; x<subWidth; x++) {
        for (int i=0; i<3; i++) {
          int v = ((int)u[x*4+i]-avg[i])*10 + base;
          if (v<0) v = 0;
          if (v>255) v = 255;
          u[x*4+i] = v;
        }
      }
    }
    QCoreApplication::postEvent(this->viewer, new QPlaceImageEvent(outImage, subX, subY));
//     QCoreApplication::postEvent(this->viewer, new QPlaceImageEvent(new QImage(img), 0, 0));
#endif
    return 0;
  }

  void setDisplayProps(const int bbox[4], int tgtWidth, int tgtHeight) {
    int totalWidth = bbox[1]-bbox[0];
    int totalHeight = bbox[3]-bbox[2];
    double scaleX = tgtWidth / double(totalWidth);
    double scaleY = 0.5*tgtHeight / double(totalHeight);
    this->scale = fmin(scaleX, scaleY);
    this->xOff = bbox[0];
    this->yOff = bbox[2];
    this->targetHeight = tgtHeight;
  }

  QImageViewer *viewer;
  double scale;
  int targetHeight;
  int xOff;
  int yOff;
  int runs;
};

#if REPORT_EFFICIENCY
void incEfficiency(void *arg) {
  ((SafeCounter*)arg)->increase();
}

void decEfficiency(void *arg) {
  ((SafeCounter*)arg)->decrease();
}

class ReportThread: public Thread {
public:
  ReportThread(SafeCounter *cpuCounter, SafeCounter *gpuCounter) {
    this->gpuUsage = gpuCounter;
    this->cpuUsage = cpuCounter;
  }
  
  virtual void run() {
    while (true) {
      fprintf(stdout, "%d %d\n", this->gpuUsage->getValue(), this->cpuUsage->getValue());
      fflush(stdout);
      sleep(1);
    }
  }
  
private:
  SafeCounter *gpuUsage;
  SafeCounter *cpuUsage;
};

#endif

void *runPipeline(const char *filename, ExecutionEngine *engine, QImageViewer *viewer, bool itk, int ncpu) {
  useITK = itk;
  if (useITK) {
    itk::MultiThreader::SetGlobalDefaultNumberOfThreads(ncpu);
  }
  TaskOrientedModule *fetch = new TaskOrientedModule(1, 2, "Decode Image");
  fetch->addImplementation(new IImageFetch());
  
  TaskOrientedModule *contrast = new TaskOrientedModule(4, 0, "Increase Contrast");
  IImageContrast *contrastImpl = new IImageContrast(viewer);
  contrast->addImplementation(contrastImpl);

  TaskOrientedModule *blur = new MGaussianBlur(1);
  blur->addImplementation(new ICPUImageBlur());
  
  TaskOrientedModule *diff = new TaskOrientedModule(2, 1, "Blending Diff");
  diff->addImplementation(new ICPUImageDiff());

  TaskOrientedModule *invert = new TaskOrientedModule(1, 1, "Invert");
  invert->addImplementation(new ICPUImageInvert());
  
  TaskOrientedModule *threshold = new MImageThreshold(0.99, 1);
  threshold->addImplementation(new ICPUImageThreshold());
  
  TaskOrientedModule::connect(fetch, 0, blur, 0);
  TaskOrientedModule::connect(fetch, 0, diff, 0);
  TaskOrientedModule::connect(blur, 0, diff, 1);
  TaskOrientedModule::connect(diff, 0, threshold, 0);
  TaskOrientedModule::connect(threshold, 0, invert, 0);
  
  TaskOrientedModule::connect(invert, 0, contrast, 0);
  TaskOrientedModule::connect(fetch, 0, contrast, 1);

  double start, end;
  const struct HEADER {
    int count;
    int bbox[4]; // Xmin, Xmax, Ymin, Ymax
  }* headerptr;
  const struct IMGHEADER {
    int x, y, size;
  }* imgheaderptr;
  start = WALLCLOCK();

  int fd = open(filename, O_RDONLY);
  struct stat st;
  fstat(fd, &st);
  size_t insize = st.st_size;
  void * const ptr = mmap(0, insize, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (ptr == (void*)-1) {
    fprintf(stderr, "could not mmap %s\n", filename);
    abort();
  }
  
  headerptr = reinterpret_cast<const HEADER*>(ptr);
  const char* here = reinterpret_cast<const char*>(headerptr+1);
  
#if USE_QT
  contrastImpl->setDisplayProps(headerptr->bbox, viewer->image.width(), viewer->image.height());
#endif
  
  for (int i=0; i<headerptr->count; i++) {
    imgheaderptr = reinterpret_cast<const IMGHEADER*>(here);
    here += sizeof(IMGHEADER);
    void *imageData = const_cast<void*>(reinterpret_cast<const void*>(here));
    here += imgheaderptr->size;
    Flow *flows[3] = {
      engine->createFlow(fetch, 0, SimpleData::createFromVoidPtr(imgheaderptr->size, imageData, false)),
      engine->createFlow(contrast, 2, SimpleData::createInt(imgheaderptr->x)),
      engine->createFlow(contrast, 3, SimpleData::createInt(imgheaderptr->y)),
    };
#if 0
    if (i%16 == 0)
      engine->sendFlows(sizeof(flows)/sizeof(flows[0]), flows, RET_ON_FREE);
    else
      engine->sendFlows(sizeof(flows)/sizeof(flows[0]), flows, RET_ON_DEPLOY);
#else
    engine->sendFlows(sizeof(flows)/sizeof(flows[0]), flows, RET_IMMEDIATELY);
#endif
  }
  
  engine->waitForAllDone();
  end = WALLCLOCK();
  
  fprintf(stderr, "TOTAL TIME: %.4lf\n", end-start);
  fprintf(stderr, "\n");
  return NULL;
}
