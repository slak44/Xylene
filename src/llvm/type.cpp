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
  staticInitializer(llvm::Function::Create(
    staticInitializerTy,
    llvm::GlobalValue::InternalLinkage,
    nameFrom("initializer", "static"),
    cv->module
  )),
  initializer(llvm::Function::Create(
    initializerTy,
    llvm::GlobalValue::InternalLinkage,
    nameFrom("initializer", "normal"),
    cv->module
  )),
  staticInitBlock(llvm::BasicBlock::Create(
    *cv->context,
    nameFrom("initializer", "staticblock"),
    staticInitializer
  )),
  initBlock(llvm::BasicBlock::Create(
    *cv->context,
    nameFrom("initializer", "normalblock"),
    initializer
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
  return &(*initializer->getArgumentList().begin());
}

std::size_t TypeData::getStructMemberIdx() const {
  return structMembers.size() - 1;
}

std::size_t TypeData::getStructMemberIdxFrom(std::string memberName) const {
  return std::find(ALL(structMemberNames), memberName) - structMemberNames.begin();
}
