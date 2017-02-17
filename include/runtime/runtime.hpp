#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <string>
#include <cstdlib>

#include "utils/util.hpp"
#include "runtime/io.hpp"

extern "C" {
  struct _xyl_Value {
    void* ptrToData;
    UniqueIdentifier allowedTypeList;
    UniqueIdentifier currentType;
  };

  bool _xyl_checkTypeCompat(_xyl_Value* val, _xyl_Value* newVal);
  const char* _xyl_typeOf(_xyl_Value* val);
  _xyl_Value _xyl_withType(_xyl_Value* toConcretize, UniqueIdentifier concreteType);
  
  void _xyl_typeErrIfIncompatible(_xyl_Value* val, _xyl_Value* newVal);
  /**
    \brief This kills the program
    
    This will be temporarily used for errors, before exceptions are implemented.
  */
  void _xyl_finish(const char* message, int exitCode) __attribute__((noreturn));
}

#endif
