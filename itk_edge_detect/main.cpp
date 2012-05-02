#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRescaleIntensityImageFilter.h"

#include "itkDataObject.h"
#include "itkObjectFactory.h"

#include "itkDiscreteGaussianImageFilter.h"
#include "itkGaussianBlurImageFunction.h"
#include "itkSubtractImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkInvertIntensityImageFilter.h"
#include "itkWeightedAddImageFilter.h"
#include "itkProcessScheduler.h"
#include "itkProcessObject.h"

#include <vector>
#include <tr1/unordered_map>
#include <sys/time.h>
inline double WALLCLOCK() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec*1e-6;
}

typedef std::vector<itk::ProcessObject*> POVector;
typedef std::vector<float> POStat;
typedef std::tr1::unordered_map<itk::ProcessObject*, bool> PODone;
typedef std::tr1::unordered_map<itk::ProcessObject*, int> POTask;

class MyScheduler : public itk::ProcessScheduler
{
public:

  typedef MyScheduler                     Self;
  typedef itk::Object                     Superclass;
  typedef itk::SmartPointer< Self >       Pointer;
  typedef itk::SmartPointer< const Self > ConstPointer;
  
  POVector jobs;
  PODone   jobDone;
  POTask   jobTask;
  POStat   jobStat;
  int      taskCount;
  int      count;
  bool     complete;
  itk::ProcessObject *parent;

  itkNewMacro(Self);
  
  itkTypeMacro(MyScheduler, itk::Object);

  virtual void scheduleTraverseUpstream(itk::ProcessObject *proc) {
    if (!this->complete) {
      count++;
    }
    itk::ProcessObject *kid = this->parent;
    if (proc->GetNumberOfIndexedInputs()==0 && this->jobTask.find(proc)==this->jobTask.end()) {
      this->jobTask[proc] = this->taskCount++;
    }
    this->parent = proc;
    itk::ProcessScheduler::scheduleTraverseUpstream(proc); 
    if (kid)
      this->jobTask[kid] = this->jobTask[proc];
    this->parent = kid;
  }

  typedef struct  {
    MyScheduler *self;
    int          taskId;
    int          nThread;
    double       time;
    pthread_t    pid;
  } ExecInput;

  static void* execThread(void *arg) {
    ExecInput *in = (ExecInput*)arg;
    double start = WALLCLOCK();
    for (size_t i=0; i+1<in->self->jobs.size(); i++) {
      itk::ProcessObject *proc = in->self->jobs[i];
      if (in->self->jobTask[proc]==in->taskId) {
        proc->SetNumberOfThreads(in->nThread);
        proc->PerformComputation();
      }
    }
    in->time = WALLCLOCK()-start;
    return NULL;
  }
    
  virtual void schedulePerformComputation(itk::ProcessObject *proc) {
    if (!this->complete) {
      count--;
      if (this->jobDone.find(proc)==jobDone.end()) {
        jobs.push_back(proc);
        jobDone[proc] = false;
      }
    }
    else
      itk::ProcessScheduler::schedulePerformComputation(proc);
    if (!this->complete && count==0 && jobs.size()>0) {
      this->complete = true;
      int totalThread = itk::MultiThreader::GetGlobalDefaultNumberOfThreads();
      if (this->jobStat.size()!=this->taskCount) {
        this->jobStat.resize(this->taskCount);
        for (int i=0; i<this->taskCount; i++) {
          this->jobStat[i] = 1.0/this->taskCount;
        }
      }

      ExecInput params[this->taskCount];
      int acc = 0;
      for (int i=0; i<this->taskCount; i++) {
        params[i].self = this;
        params[i].taskId = i;
        if (i<this->taskCount-1)
          params[i].nThread = totalThread-totalThread * this->jobStat[i];
        else
          params[i].nThread = totalThread-acc>0?totalThread-acc:1;
        acc += params[i].nThread;
        pthread_create(&(params[i].pid), 0, execThread, params+i);
      }
      double sum = 0;
      for (int i=0; i<this->taskCount; i++) {
        pthread_join(params[i].pid, 0);
        sum += params[i].time;
      }
      for (int i=0; i<this->taskCount; i++) {
        this->jobStat[i] = params[i].time/sum;
      }      
      jobs.back()->PerformComputation();
      jobs.clear();
      jobDone.clear();
      jobTask.clear();
      this->taskCount = 0;
      this->complete = false;
    }
  }

  virtual itk::ProcessScheduler::Pointer Create() {
    return static_cast<itk::ProcessScheduler*>(this);
  }
  
protected:
  MyScheduler() { this->complete = false; this->parent = NULL; this->taskCount = 0; }
  ~MyScheduler() {}
  
};

int main( int argc, char * argv[] )
{
  if( argc != 6 ) {
    fprintf(stderr,  "  USAGE:  %s <-d|-t> <thread_count> <iterations> inputImageFile  outputImageFile\n"
                     "          -d             : Use the default scheduling strategy.\n"
                     "          -t             : Use the task-parallel scheduling strategy.\n"
                     "          <thread_count> : the number of ITK thread per module/global for the default and task-parallel strategy.\n"
                     "                           Specify 0 to use maximum number of threads.\n"
                     "          <iterations>   : the number of computing iterations. Default is 10.\n", argv[0]);
    return EXIT_FAILURE;
  }
  int threadCount = atoi(argv[2]);
  int iterations  = atoi(argv[3]);
  if (strncmp(argv[1], "-t", 2)==0) {
    itk::ProcessScheduler::SetDefaultScheduler(MyScheduler::New());
  }
  if (threadCount>0) {
    itk::MultiThreader::SetGlobalDefaultNumberOfThreads(threadCount);
  }
  
  typedef    float    InputPixelType;
  typedef    float    OutputPixelType;
  typedef itk::Image< InputPixelType,  2 >   InputImageType;
  typedef itk::Image< OutputPixelType, 2 >   OutputImageType;

  typedef itk::ImageFileReader< InputImageType >  ReaderType;

  typedef itk::DiscreteGaussianImageFilter<InputImageType, InputImageType >  BlurFilterType;

  ReaderType::Pointer reader1 = ReaderType::New();
  reader1->SetFileName(argv[4]);

  BlurFilterType::Pointer blur = BlurFilterType::New();
  blur->SetInput(reader1->GetOutput());
  blur->SetVariance(3);

  typedef itk::SubtractImageFilter<InputImageType,InputImageType,InputImageType> SubtractionImageFilterType;
  SubtractionImageFilterType::Pointer subtractor=SubtractionImageFilterType::New();
  subtractor->SetInput1(reader1->GetOutput());
  subtractor->SetInput2(blur->GetOutput());
  
  typedef unsigned char WritePixelType;
  typedef itk::Image< WritePixelType, 2 > WriteImageType;
  typedef itk::RescaleIntensityImageFilter<
               InputImageType, WriteImageType > RescaleFilterType;
  RescaleFilterType::Pointer rescaler = RescaleFilterType::New();
  rescaler->SetOutputMinimum(0);
  rescaler->SetOutputMaximum(255);
  rescaler->SetInput( subtractor->GetOutput() );

  typedef itk::BinaryThresholdImageFilter<WriteImageType,WriteImageType > BinaryThresholdImageFilterType;
  BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New ();
  binaryThresholdFilter->SetInput( rescaler->GetOutput());
  binaryThresholdFilter->SetOutsideValue(255);
  binaryThresholdFilter->SetInsideValue(0);            
  binaryThresholdFilter->SetLowerThreshold(1);
  binaryThresholdFilter->SetUpperThreshold(95);
  
  typedef itk::InvertIntensityImageFilter <WriteImageType> InvertIntensityImageFilterType;
  InvertIntensityImageFilterType::Pointer invert = InvertIntensityImageFilterType::New();
  invert->SetInput(binaryThresholdFilter->GetOutput());
  
  ReaderType::Pointer reader2 = ReaderType::New();
  reader2->SetFileName(argv[4]);
  
  BlurFilterType::Pointer blur2 = BlurFilterType::New();
  blur2->SetInput(reader2->GetOutput());
  blur2->SetVariance(3);

  RescaleFilterType::Pointer rescaler2 = RescaleFilterType::New();
  rescaler2->SetOutputMinimum(0);
  rescaler2->SetOutputMaximum(255);
  rescaler2->SetInput(blur2->GetOutput());
  
  typedef itk::WeightedAddImageFilter<WriteImageType, WriteImageType, WriteImageType > AddFilterType;
  AddFilterType::Pointer add = AddFilterType::New();
  add->SetAlpha(0.4);
  add->SetInput1(invert->GetOutput());
  add->SetInput2(rescaler2->GetOutput());
  

  for (int i=0; i<iterations; i++) {
    reader1->Modified();
    reader2->Modified();
    add->Update();
  }

  typedef itk::ImageFileWriter< WriteImageType >  WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(argv[5]);
  writer->SetInput(add->GetOutput());
  writer->Update();

  return EXIT_SUCCESS;
}
