#ifndef STANDARD_HPP
#define STANDARD_HPP

#include <unordered_map>
#include <string>

#include "llvm/typeId.hpp"
#include "io.hpp"

extern "C" {
  struct Value {
    void* ptrToData;
    UniqueIdentifier allowedTypeList;
    UniqueIdentifier currentType;
  };

  bool checkTypeCompat(Value val, UniqueIdentifier newType);
  std::string typeOf(Value val);
  Value withType(Value toBeConcretized, UniqueIdentifier concreteType);
}

/**
  \brief Maps standard library function names to pointers to those functions.
*/
const std::unordered_map<std::string, void*> nameToFunPtr {
  {"printC", reinterpret_cast<void*>(printC)}
};

#endif
