#include <thread.h>

BEGIN_NAMESPACE

void* RunThread(void *arg) {
  Thread *t = (Thread*)arg;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
  t->run();
  t->running = false;
  return NULL;
}

Thread::Thread() {
  this->running = false;
}

Thread::~Thread() {
  this->terminate();
}

void Thread::start() {
  if (this->running) {
    reportWarning("Thread::start() : Thread is already running\n");
    return;
  }
  this->running = true;
  pthread_attr_t *pattr = NULL;
#ifdef USE_PAPI
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)!=0)
    reportError("Set PTHREAD_SCOPE_SYSTEM failed.");
  pattr = &attr;
#endif
  if (pthread_create(&(this->threadId), pattr, RunThread, this)!=0)
    reportError("Thread::start(): Cannot start thread\n");
}

void Thread::wait() {
  if (this->running) {
    pthread_join(this->threadId, NULL);
    this->running = false;
  }
}

void Thread::terminate() {
  if (this->running) {
    if (pthread_cancel(this->threadId)==0)
      this->wait();
    this->running = false;
  }
}

END_NAMESPACE
