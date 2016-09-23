#include "llvm/compiler.hpp"

MemberMetadata::MemberMetadata(
  DefiniteTypeInfo allowed,
  llvm::Type* toAllocate,
  std::string name,
  Trace where
):
  allowed(allowed),
  toAllocate(toAllocate),
  name(name),
  where(where) {}

DefiniteTypeInfo MemberMetadata::getTypeInfo() const {
  return allowed;
}

llvm::Type* MemberMetadata::getAllocaType() const {
  return toAllocate;
}

std::string MemberMetadata::getName() const {
  return name;
}

Trace MemberMetadata::getTrace() const {
  return where;
}

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
  
std::vector<MemberMetadata::Link> TypeData::getStructMembers() const {
  return members;
}

std::vector<llvm::Type*> TypeData::getAllocaTypes() const {
  std::vector<llvm::Type*> allocaTypes {};
  std::transform(ALL(members), std::back_inserter(allocaTypes), [](auto a) {
    return a->getAllocaType();
  });
  return allocaTypes;
}

void TypeData::addStructMember(MemberMetadata newMember) {
  // Check if this name isn't already used
  // TODO this is O(n) every time a member is added. If there's issues, use a map for checking in O(1).
  if (std::find_if(ALL(members), [&newMember](MemberMetadata::Link m) {
    return m->getName() == newMember.getName();
  }) != members.end()) throw Error("SyntaxError",
    "Duplicate member with name '" + newMember.getName() + "' in type " + node->getName(),
    newMember.getTrace()
  );
  members.push_back(std::make_shared<MemberMetadata>(newMember));
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
