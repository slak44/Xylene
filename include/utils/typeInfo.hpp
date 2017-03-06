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
  /// Store only one type. Convenience constructor.
  TypeInfo(TypeName type) noexcept: evalValue(TypeList({type})) {}
  /// \copydoc TypeInfo(TypeName)
  TypeInfo(const char* type) noexcept: TypeInfo(std::string(type)) {}
  /// Store a list of types. If list is empty, the type is dynamic.
  TypeInfo(TypeList evalValue) noexcept: evalValue(evalValue) {}
  /// Construct TypeInfo for a void type.
  TypeInfo(std::nullptr_t voidType) noexcept: isVoidType(true) {UNUSED(voidType);}
  /// Construct a dynamic TypeInfo (can have any type).
  TypeInfo() noexcept: evalValue({}) {}
  
  virtual ~TypeInfo() {}
  TypeInfo(const TypeInfo&) = default;
  TypeInfo& operator=(const TypeInfo&) = default;
  
  inline TypeList getEvalTypeList() const {
    throwIfVoid();
    return evalValue;
  }
  
  inline bool isDynamic() const {
    throwIfVoid();
    return evalValue.size() == 0;
  }

  inline bool isVoid() const noexcept {
    return isVoidType;
  }
  
  /// Get a string representation of the held types
  std::string getTypeNameString() const noexcept;
  
  virtual std::string toString() const noexcept;
  
  bool operator==(const TypeInfo& rhs) const noexcept;
  bool operator!=(const TypeInfo& rhs) const noexcept;
};

inline std::ostream& operator<<(std::ostream& os, const TypeInfo& ti) noexcept {
  return os << ti.toString();
}

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
  
  std::string toString() const noexcept override;
};

inline std::ostream& operator<<(std::ostream& os, const DefiniteTypeInfo& dti) noexcept {
  return os << dti.toString();
}

/**
  \brief TypeInfo with a singular non-void, non-dynamic type.
*/
class StaticTypeInfo: public DefiniteTypeInfo {
public:
  /**
    \param type name of type to store
  */
  StaticTypeInfo(std::string type);
  /// \copydoc StaticTypeInfo(std::string)
  StaticTypeInfo(const char* type);
  
  std::string toString() const noexcept override;
};

inline std::ostream& operator<<(std::ostream& os, const StaticTypeInfo& sti) noexcept {
  return os << sti.toString();
}

/**
  \brief Signature for a function. Stores return type and argument types.
*/
class FunctionSignature {
public:
  using Arguments = std::vector<std::pair<std::string, DefiniteTypeInfo>>;
private:
  TypeInfo returnType;
  Arguments arguments;
public:
  FunctionSignature(TypeInfo returnType, Arguments arguments);
  
  inline TypeInfo getReturnType() const noexcept {
    return returnType;
  }

  inline Arguments getArguments() const noexcept {
    return arguments;
  }
  
  std::string toString() const noexcept;
  
  inline bool operator==(const FunctionSignature& rhs) const noexcept {
    return returnType == rhs.returnType && arguments == rhs.arguments;
  }
  inline bool operator!=(const FunctionSignature& rhs) const noexcept {
    return !operator==(rhs);
  }};

inline std::ostream& operator<<(std::ostream& os, const FunctionSignature& fs) noexcept {
  return os << fs.toString();
}

#endif
