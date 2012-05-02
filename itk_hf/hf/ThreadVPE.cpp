#include <sys/sysinfo.h>
#include <ThreadVPE.h>
#include <TaskImplementation.h>

BEGIN_NAMESPACE

ThreadVPE::ThreadVPE(int tId) {
  bindToCore = true;
  this->resource.set(CR_THREAD, 1);
  char buf[128];
  sprintf(buf, "CPU Thread %d", tId);
  this->setName(buf);
  this->threadId = tId;
}

ThreadVPE::ThreadVPE() {
  bindToCore = false;
  this->resource.set(CR_THREAD, 1);
  this->setName("CPU Thread");
  this->threadId = -1;
}

void ThreadVPE::initialize() {
  if (bindToCore)
    setAffinity();
}

void ThreadVPE::setAffinity() {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  int desired_proc = this->threadId % getNumCPUs();
  CPU_SET(desired_proc, &cpuset);
  int res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (res!=0)
      reportWarning("Cannot set Thread Affinity for %s", this->getName());
}

int ThreadVPE::getNumCPUs() 
{
  return get_nprocs();
}

END_NAMESPACE
