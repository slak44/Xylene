#include "llvm/compiler.hpp"

TypeData::TypeData(llvm::StructType* type, CompileVisitor::Link cv, Node<TypeNode>::Link tyNode):
  dataType(type),
  cv(cv),
  node(tyNode),
  staticInitializerTy(llvm::FunctionType::get(
    llvm::Type::getVoidTy(*cv->context),
    {},
    false
  )),
  initializerTy(llvm::FunctionType::get(
    llvm::Type::getVoidTy(*cv->context),
    {llvm::PointerType::get(dataType, 0)},
    false
  )),
  staticInitializer(std::make_shared<FunctionWrapper>(
    llvm::Function::Create(
      staticInitializerTy,
      llvm::GlobalValue::InternalLinkage,
      nameFrom("initializer", "static"),
      cv->module
    ),
    FunctionSignature(nullptr, {})
  )),
  initializer(std::make_shared<FunctionWrapper>(
    llvm::Function::Create(
      initializerTy,
      llvm::GlobalValue::InternalLinkage,
      nameFrom("initializer", "normal"),
      cv->module
    ),
    FunctionSignature(nullptr, {
      {"this", StaticTypeInfo(tyNode->getName())}
    })
  )),
  staticInitBlock(llvm::BasicBlock::Create(
    *cv->context,
    nameFrom("initializer", "staticblock"),
    staticInitializer->getValue()
  )),
  initBlock(llvm::BasicBlock::Create(
    *cv->context,
    nameFrom("initializer", "normalblock"),
    initializer->getValue()
  )) {}
  
std::vector<llvm::Type*> TypeData::getStructMembers() const {
  return structMembers;
}

void TypeData::addStructMember(llvm::Type* newTy, std::string memberName) {
  structMembers.push_back(newTy);
  structMemberNames.push_back(memberName);
}

llvm::StructType* TypeData::getStructTy() const {
  return dataType;
}

void TypeData::builderToStaticInit() {
  hasStaticInit = true;
  cv->functionStack.push(staticInitializer);
  cv->builder->SetInsertPoint(staticInitBlock);
}

void TypeData::builderToInit() {
  hasNormalInit = true;
  cv->functionStack.push(initializer);
  cv->builder->SetInsertPoint(initBlock);
}

llvm::Argument* TypeData::getInitStructArg() const {
  return &(*initializer->getValue()->getArgumentList().begin());
}

std::size_t TypeData::getStructMemberIdx() const {
  return structMembers.size() - 1;
}

std::size_t TypeData::getStructMemberIdxFrom(std::string memberName) const {
  return std::find(ALL(structMemberNames), memberName) - structMemberNames.begin();
}

ValueWrapper::ValueWrapper(llvm::Value* value, TypeName name):
  llvmValue(value),
  currentType(name) {}
ValueWrapper::ValueWrapper(std::pair<llvm::Value*, TypeName> pair):
  ValueWrapper(pair.first, pair.second) {}

bool ValueWrapper::isInitialized() const {
  return llvmValue != nullptr || currentType == "";
}

bool ValueWrapper::hasPointerValue() const {
  return isInitialized() && llvmValue->getType()->isPointerTy();
}

llvm::Value* ValueWrapper::getValue() const {
  return llvmValue;
}

void ValueWrapper::setValue(llvm::Value* newVal, TypeName newName) {
  llvmValue = newVal;
  currentType = newName;
}

TypeName ValueWrapper::getCurrentTypeName() const {
  return currentType;
}

bool ValueWrapper::canBeBooleanValue() const {
  // TODO: it might be convertible to boolean, check for that as well
  return currentType == "Boolean";
}

FunctionWrapper::FunctionWrapper(llvm::Function* func, FunctionSignature sig):
  ValueWrapper(func, "Function"),
  sig(sig) {}

FunctionSignature FunctionWrapper::getSignature() const {
  return sig;
}

llvm::Function* FunctionWrapper::getValue() const {
  return static_cast<llvm::Function*>(llvmValue);
}

DeclarationWrapper::DeclarationWrapper(llvm::Value* decl, TypeName current, TypeList tl):
  ValueWrapper(decl, current),
  tl(tl) {}

TypeList DeclarationWrapper::getTypeList() const {
  return tl;
}
