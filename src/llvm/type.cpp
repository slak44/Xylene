#include "llvm/compiler.hpp"
#include "llvm/typeId.hpp"

TypeData::TypeInitializer::TypeInitializer(TypeData& tyData, Kind k): owner(tyData) {
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
      FunctionSignature(nullptr, {}),
      tyData.cv->functionTid
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
      }),
      owner.cv->functionTid
    );
    init->getValue()->getArgumentList().begin()->setName("this");
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

bool TypeData::TypeInitializer::exists() const {
  return initExists;
}

ValueWrapper::Link TypeData::TypeInitializer::getInitStructArg() const {
  static ValueWrapper::Link thisObject = nullptr;
  // This is only done once per initializer, since we only need a pointer and
  // we can keep returning that one pointer
  if (thisObject == nullptr) {
    // Remember where we begin
    auto startingPosition = owner.cv->builder->GetInsertBlock();
    // Enter initializer
    owner.cv->functionStack.push(init);
    owner.cv->builder->SetInsertPoint(initBlock);
    // Actually get the argument
    thisObject = owner.cv->getPtrForArgument(owner.node->getTid(), init, 0);
    // Exit initializer
    owner.cv->functionStack.pop();
    owner.cv->builder->SetInsertPoint(startingPosition);
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
  auto currentBlock = owner.cv->builder->GetInsertBlock();
  // Enter initializer
  owner.cv->functionStack.push(init);
  owner.cv->builder->SetInsertPoint(initBlock);
  // Run all the codegen functions
  std::for_each(ALL(initsToAdd), [=](std::function<void(TypeInitializer&)> codegenFunc) {
    codegenFunc(*this);
  });
  // Return nothing
  owner.cv->builder->CreateRetVoid();
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

Node<MemberNode>::Link MemberMetadata::getNode() const {
  return mem;
}

Trace MemberMetadata::getTrace() const {
  return mem->getTrace();
}

MethodData::MethodData(Node<MethodNode>::Link meth, std::string name, FunctionWrapper::Link fun):
  meth(meth),
  name(name),
  fun(fun) {}

FunctionWrapper::Link MethodData::getFunction() const {
  return fun;
}

std::string MethodData::getName() const {
  return name;
}

Trace MethodData::getTrace() const {
  return meth->getTrace();
}

Node<BlockNode>::Link MethodData::getCodeBlock() const {
  return meth->getCode();
}

bool MethodData::isForeign() const {
  return meth->isForeign();
}

ConstructorData::ConstructorData(Node<ConstructorNode>::Link constr, FunctionWrapper::Link fun):
  constr(constr),
  fun(fun) {}

FunctionWrapper::Link ConstructorData::getFunction() const {
  return fun;
}

Trace ConstructorData::getTrace() const {
  return constr->getTrace();
}

Node<BlockNode>::Link ConstructorData::getCodeBlock() const {
  return constr->getCode();
}

bool ConstructorData::isForeign() const {
  return constr->isForeign();
}

TypeData::TypeData(llvm::StructType* type, CompileVisitor::Link cv, Node<TypeNode>::Link tyNode):
  cv(cv),
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
    normalTi.insertCode([&](TypeInitializer& ref) {
      auto initValue = cv->compileExpression(mb->getInit());
      if (!isTypeAllowedIn(mb->getTypeInfo().getEvalTypeList(), initValue->getCurrentType())) {
        throw Error("TypeError", "Member initialization does not match its type", mb->getInit()->getTrace());
      }
      auto memberDecl = ref.getInitInstance()->getMember(mb->getName());
      cv->builder->CreateStore(initValue->getValue(), memberDecl->getValue());
      memberDecl->setValue(memberDecl->getValue(), initValue->getCurrentType());
    });
  });
  normalTi.finalize();
  std::for_each(ALL(staticMembers), [&](MemberMetadata::Link mb) {
    llvm::GlobalVariable* staticVar = new llvm::GlobalVariable(
      *cv->module,
      cv->typeFromInfo(mb->getTypeInfo(), mb->getNode()),
      false,
      llvm::GlobalValue::InternalLinkage,
      nullptr,
      nameFrom("static_member", mb->getName())
    );
    if (!mb->hasInit()) return;
    auto initValue = cv->compileExpression(mb->getInit());
    if (!isTypeAllowedIn(mb->getTypeInfo().getEvalTypeList(), initValue->getCurrentType())) {
      throw Error("TypeError", "Static member initialization does not match its type", mb->getInit()->getTrace());
    }
    cv->builder->CreateStore(initValue->getValue(), staticVar);
  });
  staticTi.finalize();
  for (auto method : methods) {
    if (!method->isForeign()) {
      cv->functionStack.push(method->getFunction());
      cv->compileBlock(method->getCodeBlock(), "method_" + method->getName() + "_entryBlock");
      cv->functionStack.pop();
    }
  }
  for (auto constr : constructors) {
    if (!constr->isForeign()) {
      auto oldBlock = cv->builder->GetInsertBlock();
      if (staticTi.exists()) {
        cv->functionStack.push(cv->entryPoint);
        cv->builder->SetInsertPoint(&cv->entryPoint->getValue()->front());
        cv->builder->CreateCall(staticTi.getInit()->getValue());
        cv->functionStack.pop();
      }
      cv->functionStack.push(constr->getFunction());
      auto newBlock = llvm::BasicBlock::Create(*cv->context, "constrEntryBlock", cv->functionStack.top()->getValue());
      cv->builder->SetInsertPoint(newBlock);
      auto thisPtrRef = cv->getPtrForArgument(node->getTid(), constr->getFunction(), 0);
      if (normalTi.exists()) {
        cv->builder->CreateCall(
          normalTi.getInit()->getValue(),
          {thisPtrRef->getValue()}
        );
      }
      for (auto& child : constr->getCodeBlock()->getChildren()) child->visit(cv);
      cv->builder->CreateRetVoid();
      cv->functionStack.pop();
      cv->builder->SetInsertPoint(oldBlock);
    }
  }
  finalized = true;
}

ValueWrapper::ValueWrapper(llvm::Value* value, AbstractId::Link tid):
  llvmValue(value),
  currentType(tid) {}
ValueWrapper::ValueWrapper(std::pair<llvm::Value*, AbstractId::Link> pair):
  ValueWrapper(pair.first, pair.second) {}

bool ValueWrapper::isInitialized() const {
  return llvmValue != nullptr && currentType->getAllocaType() != nullptr;
}

bool ValueWrapper::hasPointerValue() const {
  return isInitialized() && llvmValue->getType()->isPointerTy();
}

llvm::Value* ValueWrapper::getValue() const {
  return llvmValue;
}

void ValueWrapper::setValue(llvm::Value* newVal, AbstractId::Link newType) {
  llvmValue = newVal;
  currentType = newType;
}

AbstractId::Link ValueWrapper::getCurrentType() const {
  return currentType;
}

FunctionWrapper::FunctionWrapper(
  llvm::Function* func,
  FunctionSignature sig,
  TypeId::Link funTy
): ValueWrapper(func, funTy), sig(sig) {}

FunctionSignature FunctionWrapper::getSignature() const {
  return sig;
}

llvm::Function* FunctionWrapper::getValue() const {
  return static_cast<llvm::Function*>(llvmValue);
}

DeclarationWrapper::DeclarationWrapper(
  llvm::Value* decl,
  AbstractId::Link current,
  AbstractId::Link id
): ValueWrapper(decl, current) {
  if (id->storedTypeCount() == 1) {
    // We already assigned the type in the ValueWrapper constructor, just check the
    // user of this class isn't misled
    if (current != id) throw InternalError(
      "Current type must match allowed type",
      {METADATA_PAIRS}
    );
  } else if (id->storedTypeCount() > 1) {
    tlid = PtrUtil<TypeListId>::staticPtrCast(id);
  } else {
    // Should never hit this code path
    throw InternalError("Unhandled AbstractId derivative", {METADATA_PAIRS});
  }
}

TypeListId::Link DeclarationWrapper::getTypeList() const {
  if (tlid == nullptr) throw InternalError(
    "This decl only has one type, use getCurrentType to get it",
    {METADATA_PAIRS}
  );
  return tlid;
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

DeclarationWrapper::Link InstanceWrapper::getMember(std::string name) {
  TypeData* tyd = PtrUtil<TypeId>::staticPtrCast(currentType)->getTyData();
  if (tyd->dataType->isOpaque())
    throw InternalError("Member was accessed before struct body was added", {
      METADATA_PAIRS,
      {"member name", name},
      {"type name", currentType->getName()}
    });
  std::size_t idx = -1;
  auto member = std::find_if(ALL(tyd->members),
  [&idx, &name](MemberMetadata::Link m) {
    idx++;
    return m->getName() == name;
  });
  if (member == tyd->members.end()) throw Error(
    "ReferenceError",
    "No member named '" + name + "' in type " + currentType->getName(),
    tyd->node->getTrace()
  );
  auto declMember = members.find(name);
  if (declMember == members.end()) {
    // Add it if it isn't there
    auto gep = tyd->cv->builder->CreateStructGEP(
      tyd->dataType,
      this->getValue(),
      idx,
      "gep_" + name
    );
    auto id = tyd->cv->typeIdFromInfo((*member)->getTypeInfo(), tyd->node);
    return members[name] = std::make_shared<DeclarationWrapper>(gep, id, id);
  } else {
    return members[name];
  }
}

int AbstractId::getId() const {
  return id;
}

TypeName AbstractId::getName() const {
  return name;
}

TypeId::TypeId(TypeData* tyData): tyData(tyData) {
  name = tyData->getName();
}
TypeId::TypeId(TypeName name, llvm::Type* ty): basicTy(ty) {
  this->name = name;
}

std::shared_ptr<TypeId> TypeId::create(TypeData* tyData) {
  return std::make_shared<TypeId>(TypeId(tyData));
}

std::shared_ptr<TypeId> TypeId::createBasic(TypeName name, llvm::Type* ty) {
  return std::make_shared<TypeId>(TypeId(name, ty));
}

TypeData* TypeId::getTyData() const {
  return tyData;
}

llvm::Type* TypeId::getAllocaType() const {
  if (basicTy) return basicTy;
  else return tyData->getStructTy();
}

TypeList TypeId::storedNames() const {
  return {name};
}

TypeListId::TypeListId(TypeName name, std::unordered_set<AbstractId::Link> types):
  // FIXME this nullptr should actually be the tagged union type
  taggedUnionTy(nullptr), types(types) {
    this->name = name;
    if (types.size() <= 1) {
      throw InternalError(
        "Trying to make a list of 1 or less elements (use TypeId for 1 element)",
        {METADATA_PAIRS, {"size", std::to_string(types.size())}}
      );
    }
  }
  
TypeListId::Link TypeListId::create(TypeName n, std::unordered_set<AbstractId::Link> v) {
  return std::make_shared<TypeListId>(TypeListId(n, v));
}

std::unordered_set<AbstractId::Link> TypeListId::getTypes() const {
  return types;
}

TypeList TypeListId::storedNames() const {
  TypeList names;
  std::for_each(ALL(types), [&](AbstractId::Link id) {
    names.insert(id->getName());
  });
  return names;
}

llvm::Type* TypeListId::getAllocaType() const {
  return taggedUnionTy;
}
