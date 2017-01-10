#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <string>

#include "llvm/typeId.hpp"
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
}

#endif
