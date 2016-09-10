#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <string>
#include <vector>

#include "utils/util.hpp"
#include "utils/error.hpp"

/**
  \brief Stores info about the type of something.
*/
class TypeInfo {
protected:
  /// Types stored by this object. Can be empty.
  TypeList evalValue;
  /// If this type is actually the void type
  bool isVoidType = false;
  
  inline void throwIfVoid() const {
    if (isVoidType) throw InternalError("Void types have undefined type lists", {METADATA_PAIRS});
  }
public:
  /**
    \brief Store a list of types. If list is empty, the type is dynamic.
  */
  TypeInfo(TypeList evalValue);
  /**
    \brief Construct TypeInfo for a void type.
  */
  TypeInfo(std::nullptr_t voidType);
  /**
    \brief Construct a dynamic TypeInfo (can have any type).
  */
  TypeInfo();
  
  virtual ~TypeInfo();
  
  TypeList getEvalTypeList() const;
  
  bool isDynamic() const;
  bool isVoid() const;
  
  /// Get a string representation of the held types
  std::string getTypeNameString() const;
  
  virtual std::string toString() const;
  
  bool operator==(const TypeInfo& rhs) const;
  bool operator!=(const TypeInfo& rhs) const;
};

/**
  \brief TypeInfo with a singular non-void, non-dynamic type.
*/
class StaticTypeInfo: public TypeInfo {
public:
  /**
    \param type name of type to store
  */
  StaticTypeInfo(std::string type);
  /// \copydoc StaticTypeInfo(std::string)
  StaticTypeInfo(const char* type);
  
  std::string toString() const;
};

/**
  \brief TypeInfo that can't be void.
  
  Has the same constructors as TypeInfo, except for the void one.
*/
class DefiniteTypeInfo: public TypeInfo {
public:
  /// \copydoc TypeInfo::TypeInfo(TypeList)
  DefiniteTypeInfo(TypeList evalValue);
  /// \copydoc TypeInfo::TypeInfo()
  DefiniteTypeInfo();
  DefiniteTypeInfo(std::nullptr_t voidType) = delete;
  
  std::string toString() const;
};

/**
  \brief Signature for a function. Stores return type and argument types.
*/
class FunctionSignature {
public:
  using Arguments = std::map<std::string, DefiniteTypeInfo>;
private:
  TypeInfo returnType;
  Arguments arguments;
public:
  FunctionSignature(TypeInfo returnType, Arguments arguments);
  
  TypeInfo getReturnType() const;
  Arguments getArguments() const;
  
  std::string toString() const;
  
  bool operator==(const FunctionSignature& rhs) const;
  bool operator!=(const FunctionSignature& rhs) const;
};

#endif
