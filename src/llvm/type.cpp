#include "llvm/compiler.hpp"

TypeData::TypeInitializer::TypeInitializer(TypeData& tyData, Kind k):
  owner(tyData) {
  if (k == STATIC) {
    ty = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*tyData.cv->context),
      {},
      false
    );
    init = std::make_shared<FunctionWrapper>(
      llvm::Function::Create(
        ty,
        llvm::GlobalValue::InternalLinkage,
        tyData.nameFrom("initializer", "static"),
        tyData.cv->module
      ),
      FunctionSignature(nullptr, {})
    );
  } else {
    ty = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*tyData.cv->context),
      {llvm::PointerType::get(tyData.dataType, 0)},
      false
    );
    init = std::make_shared<FunctionWrapper>(
      llvm::Function::Create(
        ty,
        llvm::GlobalValue::InternalLinkage,
        tyData.nameFrom("initializer", "normal"),
        tyData.cv->module
      ),
      FunctionSignature(nullptr, {
        {"this", StaticTypeInfo(tyData.node->getName())}
      })
    );
    initializerInstance = std::make_shared<InstanceWrapper>(
      getInitStructArg()->getValue(),
      TypeList {tyData.node->getName()},
      &tyData
    );
  }
  initBlock = llvm::BasicBlock::Create(
    *tyData.cv->context,
    tyData.nameFrom("initializer", k == STATIC ? "staticblock" : "normalblock"),
    init->getValue()
  );
}

FunctionWrapper::Link TypeData::TypeInitializer::getInit() const {
  return init;
}

InstanceWrapper::Link TypeData::TypeInitializer::getInitInstance() const {
  return initializerInstance;
}

ValueWrapper::Link TypeData::TypeInitializer::getInitStructArg() const {
  static ValueWrapper::Link thisObject = nullptr;
  // We can't use the function argument, so we create a pointer, and
  // store the arg in it. That pointer we made is returned in the wrapper.
  // This is only done once per initializer, since we only need a pointer, so then
  // we can keep returning that one pointer
  if (thisObject == nullptr) {
    llvm::Argument* thisArgPtr = &*init->getValue()->getArgumentList().begin();
    auto structPtrTy = llvm::PointerType::getUnqual(owner.dataType);
    llvm::Value* thisObjectValue = owner.cv->builder->CreateAlloca(structPtrTy, nullptr, "thisAlloc");
    owner.cv->builder->CreateStore(thisArgPtr, thisObjectValue);
    thisObjectValue = owner.cv->builder->CreateLoad(structPtrTy, thisObjectValue, "this");
    thisObject = std::make_shared<ValueWrapper>(thisObjectValue, owner.node->getName());
  }
  return thisObject;
}

void TypeData::TypeInitializer::insertCode(std::function<void(TypeInitializer&)> what) {
  initsToAdd.push_back(what);
}

void TypeData::TypeInitializer::finalize() {
  // Empty initializer
  if (initsToAdd.size() == 0) {
    initExists = false;
    init->getValue()->eraseFromParent();
    init = nullptr;
    return;
  }
  initExists = true;
  // Remember where we start, so we can return
  auto currentBlock = owner.cv->builder->GetInsertBlock();
  // Enter initializer
  owner.cv->functionStack.push(init);
  owner.cv->builder->SetInsertPoint(initBlock);
  // Run all the codegen functions
  std::for_each(ALL(initsToAdd), [=](std::function<void(TypeInitializer&)> codegenFunc) {
    codegenFunc(*this);
  });
  // Exit initializer
  owner.cv->functionStack.pop();
  owner.cv->builder->SetInsertPoint(currentBlock);
}

MemberMetadata::MemberMetadata(Node<MemberNode>::Link mem, llvm::Type* toAllocate):
  mem(mem),
  toAllocate(toAllocate) {}

DefiniteTypeInfo MemberMetadata::getTypeInfo() const {
  return mem->getTypeInfo();
}

llvm::Type* MemberMetadata::getAllocaType() const {
  return toAllocate;
}

std::string MemberMetadata::getName() const {
  return mem->getIdentifier();
}

bool MemberMetadata::hasInit() const {
  return mem->hasInit();
}

Node<ExpressionNode>::Link MemberMetadata::getInit() const {
  return mem->getInit();
}

Trace MemberMetadata::getTrace() const {
  return mem->getTrace();
}

TypeData::TypeData(llvm::StructType* type, CompileVisitor::Link cv, Node<TypeNode>::Link tyNode):
  cv(cv),
  node(tyNode),
  dataType(type),
  staticTi(*this, TypeInitializer::Kind::STATIC),
  normalTi(*this, TypeInitializer::Kind::NORMAL) {}
  
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

void TypeData::validateName(std::string name) const {
  // TODO look for functions too
  Trace t = defaultTrace;
  auto matchesName = [&](MemberMetadata::Link m) {
    if (m->getName() == name) {
      t = m->getTrace();
      return true;
    }
    return false;
  };
  auto membersIt = std::find_if(ALL(members), matchesName);
  auto staticMembersIt = std::find_if(ALL(staticMembers), matchesName);
  if (membersIt != members.end() || staticMembersIt != staticMembers.end())
    throw Error(
      "SyntaxError",
      "Duplicate symbol with name '" + name + "' in type " + node->getName(),
      t
    );
}

void TypeData::addMember(MemberMetadata newMember, bool isStatic) {
  validateName(newMember.getName());
  (isStatic ? staticMembers : members).push_back(std::make_shared<MemberMetadata>(newMember));
}

llvm::StructType* TypeData::getStructTy() const {
  return dataType;
}

void TypeData::finalize() {
  std::for_each(ALL(members), [&](MemberMetadata::Link mb) {
    if (!mb->hasInit()) return;
    normalTi.insertCode([&](TypeInitializer& ref) {
      auto initValue = cv->compileExpression(mb->getInit());
      if (!cv->isTypeAllowedIn(mb->getTypeInfo().getEvalTypeList(), initValue->getCurrentTypeName())) {
        throw Error("TypeError", "Member initialization does not match its type", mb->getInit()->getTrace());
      }
      auto memberDecl = ref.getInitInstance()->getMember(mb->getName());
      cv->builder->CreateStore(initValue->getValue(), memberDecl->getValue());
      memberDecl->setValue(memberDecl->getValue(), initValue->getCurrentTypeName());
    });
  });
  normalTi.finalize();
  std::for_each(ALL(staticMembers), [&](MemberMetadata::Link mb) {
    llvm::GlobalVariable* staticVar = new llvm::GlobalVariable(
      *cv->module,
      cv->typeFromInfo(mb->getTypeInfo()),
      false,
      llvm::GlobalValue::InternalLinkage,
      nullptr,
      nameFrom("static_member", mb->getName())
    );
    if (!mb->hasInit()) return;
    auto initValue = cv->compileExpression(mb->getInit());
    if (!cv->isTypeAllowedIn(mb->getTypeInfo().getEvalTypeList(), initValue->getCurrentTypeName())) {
      throw Error("TypeError", "Static member initialization does not match its type", mb->getInit()->getTrace());
    }
    cv->builder->CreateStore(initValue->getValue(), staticVar);
  });
  staticTi.finalize();
  finalized = true;
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

InstanceWrapper::InstanceWrapper(llvm::Value* instance, TypeList tl, TypeData* tyData):
  DeclarationWrapper(instance, tyData->node->getName(), tl),
  tyData(tyData) {}

DeclarationWrapper::Link InstanceWrapper::getMember(std::string name) {
  if (static_cast<llvm::StructType*>(tyData->dataType)->isOpaque())
    throw InternalError("Member was accessed before struct body was added", {
      METADATA_PAIRS,
      {"member name", name},
      {"type name", tyData->node->getName()}
    });
  std::size_t idx = -1;
  auto member = std::find_if(ALL(tyData->members), [&idx, &name](MemberMetadata::Link m) {
    idx++;
    return m->getName() == name;
  });
  if (member == tyData->members.end()) throw Error(
    "ReferenceError",
    "No member named '" + name + "' in type " + tyData->node->getName(),
    tyData->node->getTrace()
  );
  auto declMember = members.find(name);
  if (declMember == members.end()) {
    // Add it if it isn't there
    auto gep = tyData->cv->builder->CreateGEP(
      tyData->dataType,
      this->getValue(),
      {
        llvm::ConstantInt::get(tyData->cv->integerType, 0),
        llvm::ConstantInt::get(tyData->cv->integerType, idx)
      }
    );
    auto decl = std::make_shared<DeclarationWrapper>(
      gep,
      "",
      (*member)->getTypeInfo().getEvalTypeList()
    );
    return members[name] = decl;
  } else {
    return members[name];
  }
}

void InstanceWrapper::changeTypeOfInstance(TypeData* newType) {
  // TODO check if the type allows this
  tyData = newType;
  members = {};
}
