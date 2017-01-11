#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <string>
#include <cstdlib>

#include "utils/util.hpp"
#include "runtime/io.hpp"

extern "C" {
  struct Value {
    void* ptrToData;
    UniqueIdentifier allowedTypeList;
    UniqueIdentifier currentType;
  };

  bool checkTypeCompat(Value val, Value newVal);
  char* typeOf(Value val);
  Value withType(Value toBeConcretized, UniqueIdentifier concreteType);
  
  /**
    \brief This kills the program
    
    This will be temporarily used for errors, before exceptions are implemented.
  */
  void finish(char* message, int exitCode);
}

#endif
