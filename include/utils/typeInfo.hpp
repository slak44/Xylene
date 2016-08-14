#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <string>
#include <vector>

#include "utils/util.hpp"
#include "utils/error.hpp"

class TypeInfo {
protected:
  /*
    This is what gets returned when the object is evaluated.
  */
  TypeList evalValue;
  bool isVoidType = false;
  
  inline void throwIfVoid() const {
    if (isVoidType) throw InternalError("Void types have undefined type lists", {METADATA_PAIRS});
  }
public:
  // Definite list of types (if list is empty, equivalent to the no-arg constructor)
  TypeInfo(TypeList evalValue);
  // Is void type
  TypeInfo(std::nullptr_t voidType);
  // Dynamic list of types
  TypeInfo();
  
  virtual ~TypeInfo();
  
  // Get list of held types
  TypeList getEvalTypeList() const;
  
  bool isDynamic() const;
  bool isVoid() const;
  
  // Get a string representation of the held types
  std::string getTypeNameString() const;
  
  // Stringify this object
  virtual std::string toString() const;
  
  bool operator==(const TypeInfo& rhs) const;
  bool operator!=(const TypeInfo& rhs) const;
};

/*
  TypeInfo with a singular non-void, non-dynamic type.
*/
class StaticTypeInfo: public TypeInfo {
public:
  StaticTypeInfo(std::string type);
  StaticTypeInfo(const char* type);
  
  std::string toString() const;
};

/*
  TypeInfo that can't be void.
*/
class DefiniteTypeInfo: public TypeInfo {
public:
  using TypeInfo::TypeInfo;
  DefiniteTypeInfo(std::nullptr_t voidType) = delete;
  
  std::string toString() const;
};

class FunctionTypeInfo {
private:
  TypeInfo returnType;
  std::vector<TypeInfo> argumentTypes;
public:
  FunctionTypeInfo(TypeInfo returnType, std::vector<TypeInfo> argumentTypes);
  
  std::string toString() const;
};

#endif
