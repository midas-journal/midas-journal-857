#ifndef CALLBACK_H
#define CALLBACK_H

#include "global.h"
#include "stltypes.h"

BEGIN_NAMESPACE

class Callback {
public:
  void add(VoidPtrFunction function, void *arg);
  void clear();
  int  count() const;
  void trigger() const;
private:
  VoidPtrFunctionVector functions;
  VoidPtrVector         arguments;
};

inline void Callback::add(VoidPtrFunction function, void *arg) {
  this->functions.push_back(function);
  this->arguments.push_back(arg);
}

inline void Callback::clear() {
  this->functions.clear();
  this->arguments.clear();
}

inline int Callback::count() const {
  return this->functions.size();
}

inline void Callback::trigger() const {
  for (size_t i=0; i<this->functions.size(); i++)
    this->functions[i](this->arguments[i]);
}

END_NAMESPACE

#endif
