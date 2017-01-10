#include "llvm/typeData.hpp"
#include "llvm/compiler.hpp"

TypeData::TypeData(llvm::StructType* type, ModuleCompiler::Link mc, Node<TypeNode>::Link tyNode):
  mc(mc),
  node(tyNode),
  dataType(type),
  staticTi(*this, TypeInitializer::Kind::STATIC),
  normalTi(*this, TypeInitializer::Kind::NORMAL) {}
  
TypeData::TypeInitializer TypeData::getInit() const {
  return normalTi;
}

TypeData::TypeInitializer TypeData::getStaticInit() const {
  return staticTi;
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

void TypeData::addMethod(MethodData func, bool isStatic) {
  validateName(func.getName());
  (isStatic ? staticFunctions : methods).push_back(std::make_shared<MethodData>(func));
}

void TypeData::addConstructor(ConstructorData c) {
  auto duplicateIt = std::find_if(ALL(constructors), [=](auto constr) {
    if (constr->getFunction()->getValue()->getType() == c.getFunction()->getValue()->getType()) return true;
    return false;
  });
  if (duplicateIt != constructors.end()) {
    throw Error("ReferenceError", "Multiple constructors with same signature", c.getTrace());
  }
  constructors.push_back(std::make_shared<ConstructorData>(c));
}

llvm::StructType* TypeData::getStructTy() const {
  return dataType;
}

std::string TypeData::getName() const {
  return node->getName();
}

void TypeData::finalize() {
  std::for_each(ALL(members), [&](MemberMetadata::Link mb) {
    if (!mb->hasInit()) return;
    normalTi.insertCode([=](TypeInitializer& ref) {
      auto initValue = mc->compileExpression(mb->getInit());
      if (!isTypeAllowedIn(mc->typeIdFromInfo(mb->getTypeInfo(), mb->getInit()), initValue->getCurrentType())) {
        throw Error("TypeError", "Member initialization does not match its type", mb->getInit()->getTrace());
      }
      auto memberDecl = ref.getInitInstance()->getMember(mb->getName());
      mc->builder->CreateStore(initValue->getValue(), memberDecl->getValue());
      memberDecl->setValue(memberDecl->getValue(), initValue->getCurrentType());
    });
  });
  normalTi.finalize();
  std::for_each(ALL(staticMembers), [&](MemberMetadata::Link mb) {
    llvm::GlobalVariable* staticVar = new llvm::GlobalVariable(
      *mc->module,
      mc->typeFromInfo(mb->getTypeInfo(), mb->getNode()),
      false,
      llvm::GlobalValue::InternalLinkage,
      nullptr,
      nameFrom("static_member", mb->getName())
    );
    if (!mb->hasInit()) return;
    auto initValue = mc->compileExpression(mb->getInit());
    if (!isTypeAllowedIn(mc->typeIdFromInfo(mb->getTypeInfo(), mb->getInit()), initValue->getCurrentType())) {
      throw Error("TypeError", "Static member initialization does not match its type", mb->getInit()->getTrace());
    }
    mc->builder->CreateStore(initValue->getValue(), staticVar);
  });
  staticTi.finalize();
  for (auto method : methods) {
    if (!method->isForeign()) {
      mc->functionStack.push(method->getFunction());
      mc->compileBlock(method->getCodeBlock(), "method_" + method->getName() + "_entryBlock");
      mc->functionStack.pop();
    }
  }
  for (auto constr : constructors) {
    if (!constr->isForeign()) {
      auto oldBlock = mc->builder->GetInsertBlock();
      if (staticTi.exists()) {
        mc->functionStack.push(mc->entryPoint);
        mc->builder->SetInsertPoint(&mc->entryPoint->getValue()->front());
        mc->builder->CreateCall(staticTi.getInit()->getValue());
        mc->functionStack.pop();
      }
      mc->functionStack.push(constr->getFunction());
      auto newBlock = llvm::BasicBlock::Create(*mc->context, "constrEntryBlock", mc->functionStack.top()->getValue());
      mc->builder->SetInsertPoint(newBlock);
      auto thisPtrRef = mc->getPtrForArgument(node->getTid(), constr->getFunction(), 0);
      if (normalTi.exists()) {
        mc->builder->CreateCall(
          normalTi.getInit()->getValue(),
          {thisPtrRef->getValue()}
        );
      }
      for (auto& child : constr->getCodeBlock()->getChildren()) child->visit(mc);
      mc->builder->CreateRetVoid();
      mc->functionStack.pop();
      mc->builder->SetInsertPoint(oldBlock);
    }
  }
  finalized = true;
}

TypeData::TypeInitializer::TypeInitializer(TypeData& tyData, Kind k): owner(tyData) {
  if (k == STATIC) {
    ty = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*tyData.mc->context),
      {},
      false
    );
    init = std::make_shared<FunctionWrapper>(
      llvm::Function::Create(
        ty,
        llvm::GlobalValue::InternalLinkage,
        tyData.nameFrom("initializer", "static"),
        tyData.mc->module
      ),
      FunctionSignature(nullptr, {}),
      tyData.mc->functionTid
    );
  } else {
    ty = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*tyData.mc->context),
      {llvm::PointerType::get(tyData.dataType, 0)},
      false
    );
    init = std::make_shared<FunctionWrapper>(
      llvm::Function::Create(
        ty,
        llvm::GlobalValue::InternalLinkage,
        tyData.nameFrom("initializer", "normal"),
        tyData.mc->module
      ),
      FunctionSignature(nullptr, {
        {"this", StaticTypeInfo(tyData.node->getName())}
      }),
      owner.mc->functionTid
    );
    init->getValue()->getArgumentList().begin()->setName("this");
  }
  initBlock = llvm::BasicBlock::Create(
    *tyData.mc->context,
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

bool TypeData::TypeInitializer::exists() const {
  return initExists;
}

ValueWrapper::Link TypeData::TypeInitializer::getInitStructArg() const {
  // TODO: make this a class member!!!!!!!!
  static ValueWrapper::Link thisObject = nullptr;
  // This is only done once per initializer, since we only need a pointer and
  // we can keep returning that one pointer
  if (thisObject == nullptr) {
    // Remember where we begin
    auto startingPosition = owner.mc->builder->GetInsertBlock();
    // Enter initializer
    owner.mc->functionStack.push(init);
    owner.mc->builder->SetInsertPoint(initBlock);
    // Actually get the argument
    thisObject = owner.mc->getPtrForArgument(owner.node->getTid(), init, 0);
    // Exit initializer
    owner.mc->functionStack.pop();
    owner.mc->builder->SetInsertPoint(startingPosition);
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
  // If normal init
  if (init->getValue()->getArgumentList().size() > 0) {
    initializerInstance = std::make_shared<InstanceWrapper>(
      getInitStructArg()->getValue(),
      owner.node->getTid()
    );
  }
  initExists = true;
  // Remember where we start, so we can return
  auto currentBlock = owner.mc->builder->GetInsertBlock();
  // Enter initializer
  owner.mc->functionStack.push(init);
  owner.mc->builder->SetInsertPoint(initBlock);
  // Run all the codegen functions
  std::for_each(ALL(initsToAdd), [=](std::function<void(TypeInitializer&)> codegenFunc) {
    codegenFunc(*this);
  });
  // Return nothing
  owner.mc->builder->CreateRetVoid();
  // Exit initializer
  owner.mc->functionStack.pop();
  owner.mc->builder->SetInsertPoint(currentBlock);
}
