#ifndef VALUE_WRAPPERS_HPP
#define VALUE_WRAPPERS_HPP

#include <llvm/IR/Function.h>

#include "llvm/typeId.hpp"
#include "utils/typeInfo.hpp"

/**
  \brief Holds a llvm::Value* and the type it currently carries
*/
class ValueWrapper {
friend class ModuleCompiler;
public:
  using Link = std::shared_ptr<ValueWrapper>;
protected:
  llvm::Value* llvmValue;
  AbstractId::Link currentType;
public:
  ValueWrapper(llvm::Value* value, AbstractId::Link tid);
  ValueWrapper(std::pair<llvm::Value*, AbstractId::Link> pair);
  
  inline virtual ~ValueWrapper() {}
  
  /// If the value is nullptr
  bool isInitialized() const;
  /// If the llvm::Type of the value is a pointer type
  bool hasPointerValue() const;
  
  llvm::Value* getValue() const;
  AbstractId::Link getCurrentType() const;
  void setValue(llvm::Value* newVal, AbstractId::Link newType);
};

/**
  \brief Holds a llvm::Function* and its respective FunctionSignature
*/
class FunctionWrapper: public ValueWrapper {
public:
  using Link = std::shared_ptr<FunctionWrapper>;
private:
  FunctionSignature sig;
public:
  FunctionWrapper(llvm::Function* func, FunctionSignature sig, TypeId::Link funTy);
  
  FunctionSignature getSignature() const;
  
  llvm::Function* getValue() const;
};

/**
  \brief Stores an instance of a user type.
*/
class InstanceWrapper: public ValueWrapper {
public:
  using Link = std::shared_ptr<InstanceWrapper>;
private:
  /// Maps member names to their declaration
  std::map<std::string, ValueWrapper::Link> members {};
public:
  InstanceWrapper(llvm::Value*, TypeId::Link current);
  
  /// Get a pointer to the member with the specified name
  ValueWrapper::Link getMember(std::string name);
};

#endif
