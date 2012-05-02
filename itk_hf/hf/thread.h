#ifndef THREAD_H
#define THREAD_H

#include "global.h"
#include "stltypes.h"
#include <pthread.h>
#include <time.h>
#include <string.h>

BEGIN_NAMESPACE

// ========================= Thread =========================
void* RunThread(void *arg);
class Thread {
public:
  Thread();
  virtual ~Thread();
  
  void start();
  void wait();
  void terminate();
  
  bool isRunning() { return this->running; }
  virtual void run() {}
  
private:
  pthread_t threadId;
  bool      running;
  friend void* RunThread(void *arg);
};

// ========================= Mutex =========================
class WaitCondition;
class Mutex {
public:
  Mutex();
  ~Mutex();
  void lock();
  void unlock();
  bool tryLock();
  bool timedLock(int seconds);
private:
  pthread_mutex_t mutex;
  friend class WaitCondition;
};

// ========================= WaitCondition =========================
class WaitCondition {
public:
  WaitCondition();
  ~WaitCondition();
  void wait();
  void broadcast();
  void signal();

  void waitNoLock(Mutex &M);

  void selfLock();
  void selfWait();
  void selfUnlock();
private:
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
};

// ========================= WaitCondition =========================
class SafeCounter {
public:
  SafeCounter(int value=0);
  ~SafeCounter();
  int increase(int by=1);
  int decrease(int by=1);
  int getValue();

private:
  pthread_mutex_t mutex;
  int             value;
};

// ========================= Inline functions =========================
// Mutex

inline Mutex::Mutex() {
  pthread_mutex_init(&(this->mutex), NULL);
}

inline Mutex::~Mutex() {
  pthread_mutex_destroy(&(this->mutex));
}

inline void Mutex::lock() {
  pthread_mutex_lock(&(this->mutex));
}

inline void Mutex::unlock() {
  pthread_mutex_unlock(&(this->mutex));
}

inline bool Mutex::tryLock() {
  return pthread_mutex_trylock(&(this->mutex))==0;
} 

inline bool Mutex::timedLock(int seconds) {
  timespec spec;
  memset(&spec, 0, sizeof(spec));
  spec.tv_sec = seconds;
  int result = pthread_mutex_timedlock(&(this->mutex), &spec);
  return result==0;
}

// SafeCounter
inline SafeCounter::SafeCounter(int val) {
  pthread_mutex_init(&(this->mutex), NULL);
  this->value = val;
}

inline SafeCounter::~SafeCounter() {
  pthread_mutex_destroy(&(this->mutex));
}

inline int SafeCounter::increase(int by) {
  int result;
  pthread_mutex_lock(&(this->mutex));
  this->value += by;
  result = this->value;
  pthread_mutex_unlock(&(this->mutex));
  return result;
}

inline int SafeCounter::decrease(int by) {
  int result;
  pthread_mutex_lock(&(this->mutex));
  this->value -= by;
  result = this->value;
  pthread_mutex_unlock(&(this->mutex));
  return value;
}

inline int SafeCounter::getValue() {
  int result;
  pthread_mutex_lock(&(this->mutex));
  result = this->value;
  pthread_mutex_unlock(&(this->mutex));
  return value;
}

// WaitCondition
inline WaitCondition::WaitCondition() {
  pthread_mutex_init(&(this->mutex), NULL);
  pthread_cond_init(&(this->cond), NULL);
}

inline WaitCondition::~WaitCondition() {
  pthread_cond_destroy(&(this->cond));
  pthread_mutex_destroy(&(this->mutex));
}

inline void WaitCondition::wait() {
  pthread_mutex_lock(&(this->mutex));
  pthread_cond_wait(&(this->cond), &(this->mutex));
  pthread_mutex_unlock(&(this->mutex));
}

inline void WaitCondition::waitNoLock(Mutex &m) {
  pthread_cond_wait(&(this->cond), &(m.mutex));
}

inline void WaitCondition::broadcast() {
  pthread_cond_broadcast(&(this->cond));
}

inline void WaitCondition::signal() {
  pthread_cond_signal(&(this->cond));
}

inline void WaitCondition::selfLock() {
  pthread_mutex_lock(&(this->mutex));
}

inline void WaitCondition::selfWait() {
pthread_cond_wait(&(this->cond), &(this->mutex));
}

inline void WaitCondition::selfUnlock() {
  pthread_mutex_unlock(&(this->mutex));
}



END_NAMESPACE

#endif
