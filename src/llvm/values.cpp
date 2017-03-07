#include "llvm/values.hpp"
#include "llvm/typeData.hpp"
#include "llvm/compiler.hpp"

FunctionWrapper::FunctionWrapper(
  llvm::Function* func,
  FunctionSignature sig,
  TypeId::Link funTy
): ValueWrapper(func, funTy), sig(sig) {}

FunctionSignature FunctionWrapper::getSignature() const {
  return sig;
}

llvm::Function* FunctionWrapper::getValue() const {
  return static_cast<llvm::Function*>(val);
}

InstanceWrapper::InstanceWrapper(
  llvm::Value* instance,
  TypeId::Link tid
): ValueWrapper(instance, tid) {
  if (tid->getTyData() == nullptr) throw InternalError(
    "Can't create instance of non-struct type",
    {METADATA_PAIRS}
  );
}

ValueWrapper::Link InstanceWrapper::getMember(std::string name) {
  TypeData* tyd = PtrUtil<TypeId>::staticPtrCast(ty)->getTyData();
  if (tyd->dataType->isOpaque())
    throw InternalError("Member was accessed before struct body was added", {
      METADATA_PAIRS,
      {"member name", name},
      {"type name", ty->getName()}
    });
  int idx = -1;
  auto member = std::find_if(ALL(tyd->members),
  [&idx, &name](MemberMetadata::Link m) {
    idx++;
    return m->getName() == name;
  });
  if (member == tyd->members.end())
    throw "Cannot find member '{0}' in type '{1}'"_syntax(name, ty->getName()) + tyd->node->getTrace();
  auto declMember = members.find(name);
  if (declMember == members.end()) {
    // Add it if it isn't there
    auto gep = tyd->mc->builder->CreateStructGEP(
      tyd->dataType,
      this->val,
      static_cast<uint>(idx),
      "gep_" + name
    );
    auto id = tyd->mc->typeIdFromInfo((*member)->getTypeInfo(), tyd->node);
    return members[name] = std::make_shared<ValueWrapper>(gep, id);
  } else {
    return members[name];
  }
}
