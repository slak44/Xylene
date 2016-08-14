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
  TypeInfo(TypeList evalValue): evalValue(evalValue) {}
  // Void type
  TypeInfo(std::nullptr_t voidType): isVoidType(true) {
    UNUSED(voidType);
  }
  // Dynamic list of types
  TypeInfo(): evalValue({}) {}
  
  virtual ~TypeInfo() {}
  
  TypeList getEvalTypeList() const {
    throwIfVoid();
    return evalValue;
  }
  
  bool isDynamic() const {
    throwIfVoid();
    return evalValue.size() == 0;
  }
  
  bool isVoid() const {
    return isVoidType;
  }
  
  std::string getTypeNameString() const {
    if (isVoidType) return "[void]";
    return isDynamic() ? "[dynamic]" : collateTypeList(evalValue);
  }
  
  virtual std::string toString() const {
    return "TypeInfo: " + getTypeNameString();
  };
  
  bool operator==(const TypeInfo& rhs) const {
    if (isVoidType != rhs.isVoidType) return false;
    return evalValue == rhs.evalValue;
  }
  
  bool operator!=(const TypeInfo& rhs) const {
    return !operator==(rhs);
  }
};

/*
  TypeInfo with a singular non-void, non-dynamic type.
*/
class StaticTypeInfo: public TypeInfo {
public:
  StaticTypeInfo(std::string type): TypeInfo({type}) {}
  StaticTypeInfo(const char* type): StaticTypeInfo(std::string(type)) {}
  
  std::string toString() const {
    return "StaticTypeInfo: " + *std::begin(evalValue);
  };
};

/*
  TypeInfo that can't be void.
*/
class DefiniteTypeInfo: public TypeInfo {
public:
  using TypeInfo::TypeInfo;
  DefiniteTypeInfo(std::nullptr_t voidType) = delete;
  
  std::string toString() const {
    return "DefiniteTypeInfo: " + getTypeNameString();
  };
};

class FunctionTypeInfo {
private:
  TypeInfo returnType;
  std::vector<TypeInfo> argumentTypes;
public:
  FunctionTypeInfo(TypeInfo returnType, std::vector<TypeInfo> argumentTypes): returnType(returnType), argumentTypes(argumentTypes) {}
  
  std::string toString() const {
    std::string str = "FunctionInfo (";
    str += "return: " + returnType.getTypeNameString();
    str += ", arguments: " + collate<decltype(argumentTypes)>(argumentTypes, [](TypeInfo ti) {return "(arg " + ti.getTypeNameString() + ")";});
    str += ")";
    return str;
  };
};

#endif
