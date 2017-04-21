#include "llvm/compiler.hpp"
#include "llvm/typeData.hpp"

Compiler::Compiler(fs::path rootScript, fs::path output):
  rootScript(rootScript), output(output) {
  std::ifstream file(rootScript);
  std::stringstream buffer;
  buffer << file.rdbuf();
  auto lx = Lexer::tokenize(buffer.str(), rootScript);
  auto mc = ModuleCompiler::create(
    pd.types,
    "temp_module_name",
    TokenParser::parse(lx->getTokens()),
    true
  );
  mc->compile();
  pd.rootModule = std::unique_ptr<llvm::Module>(mc->getModule());
}

Compiler::Compiler(
  std::unique_ptr<llvm::Module> rootModule,
  fs::path rootScript,
  fs::path output
): rootScript(rootScript), output(output) {
  pd.rootModule = std::move(rootModule);
}

void Compiler::compile() {
  using namespace llvm;
  Module* m = pd.rootModule.get();
  
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  
  auto targetTriple = sys::getDefaultTargetTriple();
  
  std::string error;
  auto target = TargetRegistry::lookupTarget(targetTriple, error);

  if (!target) {
    throw InternalError("No target: " + error, {METADATA_PAIRS});
  }
  
  auto cpu = "generic";
  auto features = "";

  TargetOptions opt;
  auto relocModel = Optional<Reloc::Model>();
  auto targetMachine =
    target->createTargetMachine(targetTriple, cpu, features, opt, relocModel);
  
  m->setDataLayout(targetMachine->createDataLayout());
  m->setTargetTriple(targetTriple);

  std::error_code ec;
  raw_fd_ostream dest(output.native(), ec, sys::fs::OpenFlags(0));

  if (ec) {
    throw InternalError("File open: " + ec.message(), {METADATA_PAIRS});
  }
  
  legacy::PassManager pass;
  auto fileType = TargetMachine::CGFT_ObjectFile;

  if (targetMachine->addPassesToEmitFile(pass, dest, fileType)) {
    throw InternalError("TargetMachine can't emit a file of this type", {METADATA_PAIRS});
  }

  pass.run(*m);
  dest.flush();
}

fs::path Compiler::getOutputPath() const {
  return output;
}

void ModuleCompiler::addMainFunction() {
  llvm::FunctionType* mainType = llvm::FunctionType::get(integerType, false);
  functionStack.push(std::make_shared<FunctionWrapper>(
    llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module),
    FunctionSignature("Integer", {}),
    functionTid
  ));
  entryPoint = functionStack.top();
}

ModuleCompiler::ModuleCompiler(std::string moduleName, AST ast, bool isRoot):
  context(new llvm::LLVMContext()),
  integerType(llvm::IntegerType::get(*context, bitsPerInt)),
  floatType(llvm::Type::getDoubleTy(*context)),
  voidType(llvm::Type::getVoidTy(*context)),
  booleanType(llvm::Type::getInt1Ty(*context)),
  voidPtrType(llvm::PointerType::getUnqual(llvm::IntegerType::get(*context, 8))),
  taggedUnionType(llvm::StructType::create(*context, {
    voidPtrType, // Pointer to data
    integerType, // TypeListId with allowed types
    integerType, // TypeId with currently stored type
  }, "tagged_union")),
  taggedUnionPtrType(llvm::PointerType::getUnqual(taggedUnionType)),
  voidTid(TypeId::createBasic("Void", voidType)),
  integerTid(TypeId::createBasic("Integer", integerType)),
  floatTid(TypeId::createBasic("Float", floatType)),
  booleanTid(TypeId::createBasic("Boolean", booleanType)),
  functionTid(TypeId::createBasic("Function", voidPtrType)), // TODO
  builder(std::make_unique<llvm::IRBuilder<>>(llvm::IRBuilder<>(*context))),
  module(new llvm::Module(moduleName, *context)),
  ast(ast),
  isRoot(isRoot) {
  ast.getRoot()->blockTypes.insert({
    integerTid,
    floatTid,
    booleanTid
  });
  insertRuntimeFuncDecls();
}

void ModuleCompiler::insertRuntimeFuncDecls() {
  // Insert declarations for these functions in IR, they are linked in later
  using FT = llvm::FunctionType;
  const std::unordered_map<std::string, llvm::FunctionType*> rtFunMap {
    {"_xyl_checkTypeCompat",
      FT::get(booleanType, {taggedUnionPtrType, taggedUnionPtrType}, false)},
    {"_xyl_typeOf", // TODO return string type, not void ptr
      FT::get(voidPtrType, {taggedUnionPtrType}, false)},
    {"_xyl_typeErrIfIncompatible",
      FT::get(voidType, {taggedUnionPtrType, taggedUnionPtrType}, false)},
    {"_xyl_typeErrIfIncompatibleTid",
      FT::get(voidType, {integerType, taggedUnionPtrType}, false)},
    {"_xyl_finish", // TODO arg1 should be string type
      FT::get(voidType, {voidPtrType, integerType}, false)},
    {"malloc",
      FT::get(voidPtrType, {integerType}, false)},
  };
  for (auto rtFun : rtFunMap) {
    if (module->getFunction(rtFun.first) != nullptr) continue;
    auto fun = llvm::Function::Create(
      rtFun.second, llvm::Function::ExternalLinkage, rtFun.first, module);
    fun->deleteBody();
    ast.getRoot()->blockFuncs.insert({
      rtFun.first,
      std::make_shared<FunctionWrapper>(fun, FunctionSignature("", {}), functionTid)
    });
  }
}

ModuleCompiler::Link ModuleCompiler::create(
  const ProgramData::TypeSet& types,
  std::string moduleName,
  AST ast,
  bool isRoot
) {
  auto self = std::make_shared<ModuleCompiler>(ModuleCompiler(moduleName, ast, isRoot));
  self->codegenMap = {
    {"Add", self->arithmOpBuilder(&llvm::IRBuilder<>::CreateAdd, &llvm::IRBuilder<>::CreateFAdd, "", false, false)},
    {"Substract", self->arithmOpBuilder(&llvm::IRBuilder<>::CreateSub, &llvm::IRBuilder<>::CreateFSub, "", false, false)},
    {"Multiply", self->arithmOpBuilder(&llvm::IRBuilder<>::CreateMul, &llvm::IRBuilder<>::CreateFMul, "", false, false)},
    {"Divide", self->arithmOpBuilder(&llvm::IRBuilder<>::CreateSDiv, &llvm::IRBuilder<>::CreateFDiv, "", false)},
    {"Modulo", self->arithmOpBuilder(&llvm::IRBuilder<>::CreateSRem, &llvm::IRBuilder<>::CreateFRem, "")},
    {"Equality", self->cmpOpBuilder(llvm::CmpInst::Predicate::ICMP_EQ, llvm::CmpInst::Predicate::FCMP_OEQ)},
    {"Inequality", self->cmpOpBuilder(llvm::CmpInst::Predicate::ICMP_NE, llvm::CmpInst::Predicate::FCMP_ONE)},
    {"Less", self->cmpOpBuilder(llvm::CmpInst::Predicate::ICMP_SLT, llvm::CmpInst::Predicate::FCMP_OLT)},
    {"Less or equal", self->cmpOpBuilder(llvm::CmpInst::Predicate::ICMP_SLE, llvm::CmpInst::Predicate::FCMP_OLE)},
    {"Greater", self->cmpOpBuilder(llvm::CmpInst::Predicate::ICMP_SGT, llvm::CmpInst::Predicate::FCMP_OGT)},
    {"Greater or equal", self->cmpOpBuilder(llvm::CmpInst::Predicate::ICMP_SGE, llvm::CmpInst::Predicate::FCMP_OGE)},
    {"Bitwise NOT", self->notFunction(BITWISE)},
    {"Logical NOT", self->notFunction(LOGICAL)},
    {"Bitwise AND", self->bitwiseOpBuilder(BITWISE, &llvm::IRBuilder<>::CreateAnd)},
    {"Logical AND", self->bitwiseOpBuilder(LOGICAL, &llvm::IRBuilder<>::CreateAnd)},
    {"Bitwise OR", self->bitwiseOpBuilder(BITWISE, &llvm::IRBuilder<>::CreateOr)},
    {"Logical OR", self->bitwiseOpBuilder(LOGICAL, &llvm::IRBuilder<>::CreateOr)},
    {"Bitwise XOR", self->bitwiseOpBuilder(BITWISE, &llvm::IRBuilder<>::CreateXor)},
    {"Postfix ++", self->preOrPostfixOpBuilder(POSTFIX, &llvm::IRBuilder<>::CreateAdd)},
    {"Postfix --", self->preOrPostfixOpBuilder(POSTFIX, &llvm::IRBuilder<>::CreateSub)},
    {"Prefix ++", self->preOrPostfixOpBuilder(PREFIX, &llvm::IRBuilder<>::CreateAdd)},
    {"Prefix --", self->preOrPostfixOpBuilder(PREFIX, &llvm::IRBuilder<>::CreateSub)},
    {"Unary +", CodegenFun(objBind(&ModuleCompiler::unaryPlus, self.get()))},
    {"Unary -", CodegenFun(objBind(&ModuleCompiler::unaryMinus, self.get()))},
    {"Bitshift >>", CodegenFun(objBind(&ModuleCompiler::shiftLeft, self.get()))},
    {"Bitshift <<", CodegenFun(objBind(&ModuleCompiler::shiftRight, self.get()))},
    {"Call", CodegenFun(objBind(&ModuleCompiler::call, self.get()))},
    {"Assignment", CodegenFun(objBind(&ModuleCompiler::assignment, self.get()))}
  };
  self->types = std::make_unique<ProgramData::TypeSet>(types);
  self->ast.getRoot()->blockTypes = {
    self->integerTid,
    self->floatTid,
    self->booleanTid,
    self->functionTid
  };
  if (isRoot) self->addMainFunction();
  return self;
}

void ModuleCompiler::compile() {
  ast.getRoot()->visit(shared_from_this());
  // If the current block, which is the one that exits from main, has no terminator, add one
  if (!builder->GetInsertBlock()->getTerminator()) {
    builder->CreateRet(llvm::ConstantInt::get(integerType, 0));
  }
  std::string str;
  llvm::raw_string_ostream rso(str);
  if (llvm::verifyModule(*module, &rso)) {
    module->dump();
    throw InternalError("Module failed validation", {
      METADATA_PAIRS,
      {"module name", module->getName()},
      {"error", rso.str()}
    });
  }
  serializeTypeSet();
}

void ModuleCompiler::serializeTypeSet() {
  /*
    TODO: do we really want all of this?
    when doing a runtime type check, the type list to compare against should be constant
    so theoretically, we can hardcode typelists where we do the checks
    
    is rtti even necessary?
  */
  for (auto tid : *types) {
    // Array is null terminated
    auto arrayType = llvm::ArrayType::get(integerType, tid->storedTypeCount());
    // Stores tid value + array of tids in case it's a type list
    auto rttiInfoType = llvm::StructType::create(*context, {
      integerType,
      arrayType
    });
    std::vector<llvm::Constant*> containedTys;
    containedTys.reserve(tid->storedTypeCount() + 1); // +1 for the null termination
    if (tid->storedTypeCount() > 1) {
      auto tlid = PtrUtil<TypeListId>::dynPtrCast(tid);
      std::transform(ALL(tlid->getTypes()), std::begin(containedTys), [&](auto id) {
        return llvm::ConstantInt::get(integerType, id->getId(), false);
      });
    }
    containedTys.push_back(llvm::ConstantInt::get(integerType, 0, false));
    auto rtti = new llvm::GlobalVariable(
      *module,
      rttiInfoType,
      true, // Global is constant
      llvm::GlobalValue::LinkageTypes::ExternalLinkage,
      0, // Only set initializer if in root
      fmt::format("_xyl_rtti_{0}", tid->getName()), // TODO: name mangling for collisions
      nullptr,
      llvm::GlobalValue::ThreadLocalMode::NotThreadLocal,
      0,
      !isRoot // isExternallyInitialized is true for non-roots
    );
    if (isRoot) {
      auto id = llvm::ConstantInt::get(integerType, tid->getId(), false);
      auto array = llvm::ConstantArray::get(arrayType, containedTys);
      auto rttiStruct = llvm::ConstantStruct::get(rttiInfoType, id, array, nullptr);
      rtti->setInitializer(rttiStruct);
    }
  }  
}

void ModuleCompiler::visitBlock(Node<BlockNode>::Link node) {
  compileBlock(node, "block");
}

llvm::BasicBlock* ModuleCompiler::compileBlock(Node<BlockNode>::Link node, const std::string& name) {
  llvm::BasicBlock* oldBlock = builder->GetInsertBlock();
  llvm::BasicBlock* newBlock = llvm::BasicBlock::Create(*context, name, functionStack.top()->getValue());
  builder->SetInsertPoint(newBlock);
  for (auto& child : node->getChildren()) child->visit(shared_from_this());
  // TODO: reduce indentation of this if
  // If the block lacks a terminator instruction, add one
  if (!newBlock->getTerminator()) {
    switch (node->getType()) {
      case ROOT_BLOCK:
        builder->CreateRet(llvm::ConstantInt::get(integerType, 0));
        break;
      case IF_BLOCK:
        // Technically should never happen because visitBranch *should* be properly implemented
        throw InternalError("visitBranch not implemented properly", {METADATA_PAIRS});
      case CODE_BLOCK: {
        // Attempt to merge into predecessor
        bool hasMerged = llvm::MergeBlockIntoPredecessor(newBlock);
        if (hasMerged) {
          // We can insert in the old block if it was merged
          builder->SetInsertPoint(oldBlock);
        } else {
          // TODO: what do we do with a block that has no terminator, but can't be merged?
          throw InternalError("Not Implemented", {METADATA_PAIRS});
        }
        break;
      }
      case FUNCTION_BLOCK:
        if (functionStack.top()->getValue()->getReturnType()->isVoidTy()) {
          builder->CreateRetVoid();
        } else {
          auto retTyString = functionStack.top()->getSignature().getReturnType().getTypeNameString();
          throw "Function has non-void return type '{}', but no return was found"_type(retTyString) + node->getTrace();
        }
        break;
    }
  }
  // After functions are done, start inserting in the old block, not in the function
  if (node->getType() == FUNCTION_BLOCK) {
    builder->SetInsertPoint(oldBlock);
  }
  return newBlock;
}

void ModuleCompiler::visitExpression(Node<ExpressionNode>::Link node) {
  compileExpression(node);
}

bool ModuleCompiler::canBeBoolean(ValueWrapper::Link val) const {
  // TODO: value might be convertible to boolean, check for that as well
  return val->ty == booleanTid;
}

llvm::Type* ModuleCompiler::typeFromInfo(TypeInfo ti, ASTNode::Link node) {
  if (ti.isVoid()) return llvm::Type::getVoidTy(*context);
  // TODO: do we even allow no type checking?
  if (ti.isDynamic()) throw InternalError("Not Implemented", {METADATA_PAIRS});
  AbstractId::Link ty = typeIdFromInfo(ti, node);
  return ty->getAllocaType();
}

AbstractId::Link ModuleCompiler::typeIdFromInfo(TypeInfo ti, ASTNode::Link node) {
  AbstractId::Link result = nullptr;
  ASTNode::Link defBlock = node->findAbove([&](ASTNode::Link n) {
    auto b = Node<BlockNode>::dynPtrCast(n);
    if (!b) return false;
    auto it = std::find_if(ALL(b->blockTypes), [&](AbstractId::Link id) {
      if (ti.getEvalTypeList().size() != id->storedTypeCount()) return false;
      return ti.getEvalTypeList() == id->storedNames();
    });
    if (it != b->blockTypes.end()) {
      result = *it;
      return true;
    }
    return false;
  });
  if (result == nullptr)
    throw "Can't find type '{}'"_type(ti.getTypeNameString()) + node->getTrace();
  return result;
}

ValueWrapper::Link ModuleCompiler::valueFromIdentifier(Node<ExpressionNode>::Link identifier) {
  auto name = identifier->getToken().data;
  if (identifier->getToken().type != TT::IDENTIFIER)
    throw InternalError("This function takes identifies only", {METADATA_PAIRS});
  // Check if it's an argument to this function, return it
  // TODO might have to load it
  // LLVM arguments
  auto& argList = functionStack.top()->getValue()->getArgumentList();
  // FunctionSignature arguments
  auto sigArgList = functionStack.top()->getSignature().getArguments();
  // Make sure the signature matches the arg list, otherwise something is really wrong
  if (argList.size() != sigArgList.size()) throw InternalError(
    "LLVM argument count does not match FunctionSignature argument count",
    {
      METADATA_PAIRS,
      {"llvm arg count", std::to_string(argList.size())},
      {"signature arg count", std::to_string(sigArgList.size())}
    }
  );
  // Iterate over them at the same time
  auto sigArg = sigArgList.begin();
  for (auto arg = argList.begin(); arg != argList.end(); arg++, sigArg++) {
    // Complain if arg names aren't the same
    if (arg->getName() != sigArg->first) {
      throw InternalError("Func arg name mismatch", {
        METADATA_PAIRS,
        {"llvm arg name", arg->getName()},
        {"func sig arg name", sigArg->first}
      });
    }
    if (name == sigArg->first) return std::make_shared<ValueWrapper>(
      &(*arg),
      typeIdFromInfo(sigArg->second, identifier)
    );
  }
  // Iterate over all the blocks above this identifier
  for (auto p = identifier->findAbove<BlockNode>(); p != nullptr; p = p->findAbove<BlockNode>()) {
    // If we find this ident in any of their variable scopes, return what we found
    auto it = p->blockScope.find(name);
    if (it != p->blockScope.end()) {
      ValueWrapper::Link vr = it->second;
      if (!vr->isInitialized())
        throw "Use of uninitialized value '{}'"_ref(name) + identifier->getToken().trace;
      return vr;
    }
    // If it's a function, return it
    auto fIt = p->blockFuncs.find(name);
    if (fIt != p->blockFuncs.end()) {
      return fIt->second;
    }
  }
  // Maybe the identifier is part of the enclosing type
  // If we're in a constructor or method, attempt to obtain the 'this' object
  ASTNode::Link parentFun = identifier->findAbove<ConstructorNode>();
  if (parentFun == nullptr) parentFun = identifier->findAbove<MethodNode>();
  if (parentFun != nullptr) {
    Node<TypeNode>::Link type = Node<TypeNode>::staticPtrCast(parentFun->getParent().lock());
    auto thisObj = getPtrForArgument(type->getTid(), functionStack.top(), 0);
    InstanceWrapper::Link thisInstance = std::make_shared<InstanceWrapper>(
      thisObj->val,
      type->getTid()
    );
    try {
      return thisInstance->getMember(name);
    } catch (const Error& err) {
      // getMember throws Error instances only when it can't find the member
      // So this means the identifier we're looking for isn't here
      // Which means that this catch block can be safely ignored
      UNUSED(err);
    }
  }
  throw "Cannot find '{}' in this scope"_syntax(name) + identifier->getTrace();
}

ValueWrapper::Link ModuleCompiler::compileExpression(Node<ExpressionNode>::Link node, IdentifierHandling how) {
  Token tok = node->getToken();
  if (how == AS_POINTER && tok.type != TT::IDENTIFIER && tok.type != TT::OPERATOR)
    throw "Operator requires a mutable type"_syntax + tok.trace;
  if (tok.isTerminal()) {
    switch (tok.type) {
      case TT::INTEGER: return std::make_shared<ValueWrapper>(
        llvm::ConstantInt::getSigned(integerType, std::stoll(tok.data)),
        integerTid
      );
      case TT::FLOAT: return std::make_shared<ValueWrapper>(
        llvm::ConstantFP::get(floatType, tok.data),
        floatTid
      );
      case TT::STRING: throw InternalError("Not Implemented", {METADATA_PAIRS});
      case TT::BOOLEAN: {
        auto b = tok.data == "true" ?
          llvm::ConstantInt::getTrue(booleanType) :
          llvm::ConstantInt::getFalse(booleanType);
        return std::make_shared<ValueWrapper>(b, booleanTid);
      }
      case TT::IDENTIFIER: return valueFromIdentifier(node);
      default: throw InternalError("Unhandled terminal symbol in switch case", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    };
  } else if (tok.isOp()) {
    std::vector<ValueWrapper::Link> operands {};
    // Do some magic for function calls
    if (tok.op().hasSymbol("()")) {
      // Second arg to calls is the thing being called
      // Should be a pointer
      operands.push_back(compileExpression(node->at(1), AS_POINTER));
      auto args = node->at(0);
      // Add args
      for (std::size_t i = 0; i < args->getChildren().size(); i++) {
        // TODO might need to change these AS_VALUE for complex objects
        operands.push_back(compileExpression(args->at(static_cast<int64_t>(i)), AS_VALUE));
      }
    } else {
      // Recursively compute all the operands
      std::size_t idx = 0;
      for (auto& child : node->getChildren()) {
        bool requirePointer = tok.op().getRefList()[idx];
        operands.push_back(compileExpression(Node<ExpressionNode>::staticPtrCast(child), requirePointer ? AS_POINTER : AS_VALUE));
        idx++;
      }
      // Make sure we have the correct amount of operands
      if (static_cast<int>(operands.size()) != tok.op().getArity()) {
        throw InternalError("Operand count does not match operator arity", {
          METADATA_PAIRS,
          {"operator token", tok.toString()},
          {"operand count", std::to_string(operands.size())}
        });
      }
    }
    // Call the code generating function, and return its result
    return codegenMap[tok.op().getName()](operands, node);
  } else {
    throw InternalError("Malformed expression node", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
}

// TODO: get rid of this, insertRuntimeTypeCheck should just do manual stuff for those
ValueWrapper::Link ModuleCompiler::boxPrimitive(ValueWrapper::Link p) {
  if (p->val->getType()->getPointerElementType() == taggedUnionType) return p;
  auto box = builder->CreateAlloca(taggedUnionType, nullptr, "boxPrimitive");
  // TODO: store value into the box
  return std::make_shared<ValueWrapper>(box, p->ty);
}

void ModuleCompiler::insertRuntimeTypeCheck(
  ValueWrapper::Link target,
  ValueWrapper::Link newValue
) {
  builder->CreateCall(
    module->getFunction("_xyl_typeErrIfIncompatible"),
    {target->val, boxPrimitive(newValue)->val}
  );
}

void ModuleCompiler::insertRuntimeTypeCheck(
  AbstractId::Link target,
  ValueWrapper::Link newValue
) {
  builder->CreateCall(
    module->getFunction("_xyl_typeErrIfIncompatibleTid"),
    {llvm::ConstantInt::get(integerType, target->getId()), boxPrimitive(newValue)->val}
  );
}

llvm::Value* ModuleCompiler::insertDynAlloc(uint64_t size, ValueWrapper::Link target) {
  auto i8Ptr = builder->CreateCall(
    module->getFunction("malloc"),
    {llvm::ConstantInt::get(integerType, size)}
  );
  auto properTypePtr = builder->CreateBitCast(
    i8Ptr, target->ty->getAllocaType()->getPointerTo());
  return properTypePtr;
}

llvm::Value* ModuleCompiler::storeTypeList(
  llvm::Value* taggedUnion,
  UniqueIdentifier typeList
) {
  if (taggedUnion->getType() != taggedUnionType->getPointerTo()) {
    throw InternalError("Illegal argument, expected tagged union as value", {
      METADATA_PAIRS,
      {"wrong type", getAddressStringFrom(taggedUnion->getType())}
    });
  }
  return builder->CreateStore(
    llvm::ConstantInt::get(integerType, typeList, false),
    builder->CreateBitCast(
      builder->CreateConstGEP1_32(taggedUnion, 1),
      integerType->getPointerTo()
    )
  );
}

void ModuleCompiler::assignToUnion(
  ValueWrapper::Link unionWrapper,
  ValueWrapper::Link newValue
) {
  llvm::DataLayout d(module);
  auto size = d.getTypeAllocSize(newValue->ty->getAllocaType());
  auto dataPtr = insertDynAlloc(size, newValue);
  // Store data in memory
  builder->CreateStore(newValue->val, dataPtr);
  // Store ptr to data in union
  builder->CreateStore(
    dataPtr,
    builder->CreateBitCast(
      builder->CreateConstGEP1_32(unionWrapper->val, 0),
      dataPtr->getType()->getPointerTo()
    )
  );
  // Update union current type
  builder->CreateStore(
    llvm::ConstantInt::get(integerType, newValue->ty->getId(), false),
    builder->CreateBitCast(
      builder->CreateConstGEP1_32(unionWrapper->val, 2),
      integerType->getPointerTo()
    )
  );
}

void ModuleCompiler::typeCheck(AbstractId::Link allowedLhs, ValueWrapper::Link rhs, Error err) {
  TypeCompat isCompat = allowedLhs->isCompat(rhs->ty);
  // Statically check that the type of the initialization is allowed by the declaration
  if (isCompat == INCOMPATIBLE) {
    throw err;
  } else if (isCompat == DYNAMIC) {
    insertRuntimeTypeCheck(allowedLhs, rhs);
  } else {
    // If they're compatible, great
    return;
  }
}

void ModuleCompiler::visitDeclaration(Node<DeclarationNode>::Link node) {
  Node<BlockNode>::Link enclosingBlock = node->findAbove<BlockNode>();
  llvm::Value* decl;
  // If this variable allows only one type, allocate it immediately
  if (node->getTypeInfo().getEvalTypeList().size() == 1) {
    llvm::Type* ty = typeFromInfo(node->getTypeInfo(), node);
    decl = builder->CreateAlloca(ty, nullptr, node->getIdentifier());
  } else {
    std::unordered_set<AbstractId::Link> declTypes;
    std::for_each(ALL(node->getTypeInfo().getEvalTypeList()), [&](TypeName name) {
      declTypes.insert(typeIdFromInfo(StaticTypeInfo(name), node));
    });
    auto list = TypeListId::create(
      collate(node->getTypeInfo().getEvalTypeList()),
      declTypes,
      taggedUnionType
    );
    declTypes.insert(list);
    enclosingBlock->blockTypes.insert(list);
    decl = builder->CreateAlloca(taggedUnionType, nullptr, node->getIdentifier());
    storeTypeList(decl, list->getId());
  }
  auto id = typeIdFromInfo(node->getTypeInfo(), node);
  auto declWrap = std::make_shared<ValueWrapper>(decl, id);
  
  // Add to scope
  auto inserted =
    enclosingBlock->blockScope.insert({node->getIdentifier(), declWrap});
  // If it failed, it means the decl already exists
  if (!inserted.second)
    throw "Redefinition of identifier '{}'"_ref(node->getIdentifier()) + node->getTrace();
  
  // Handle initialization
  if (!node->hasInit()) return;
  ValueWrapper::Link initValue = compileExpression(node->init());
  
  typeCheck(id, initValue,
    "Type of initialization ({0}) is not allowed by declaration ({1})"_type(
      initValue->ty->typeNames(), id->typeNames()) + node->getTrace());

  if (decl->getType() == llvm::PointerType::getUnqual(taggedUnionType)) {
    assignToUnion(declWrap, initValue);
  } else {
    builder->CreateStore(initValue->val, decl);
  }
}

void ModuleCompiler::visitBranch(Node<BranchNode>::Link node) {
  compileBranch(node);
}

// The BasicBlock surrounding is the block where control returns after dealing with branches, only specified for recursive case
void ModuleCompiler::compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding) {
  const auto handleBranchExit = [&](llvm::BasicBlock* continueCurrent, llvm::BasicBlock* success, bool usesBranchAfter) -> void {
    // Unless the block already goes somewhere else
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block
    if (!success->getTerminator()) {
      builder->SetInsertPoint(success);
      builder->CreateBr(continueCurrent);
      usesBranchAfter = true;
    }
    // If the branchAfter block is not jumped to by anyone, get rid of it
    // Otherwise, the rest of the block's instructions should be added to it
    // Only for non-recursive case, the recursive calls should not touch continueCurrent
    if (
      surrounding == nullptr &&
      !usesBranchAfter &&
      llvm::pred_begin(continueCurrent) == llvm::pred_end(continueCurrent) &&
      continueCurrent->getParent() != nullptr
    ) {
      continueCurrent->eraseFromParent();
    } else {
      builder->SetInsertPoint(continueCurrent);
    }
    return;
  };
  bool usesBranchAfter = false;
  llvm::BasicBlock* current = builder->GetInsertBlock();
  ValueWrapper::Link cond = compileExpression(node->condition());
  if (!canBeBoolean(cond)) {
    throw "Expected boolean expression in if condition"_type + node->condition()->getTrace();
  }
  llvm::BasicBlock* success = compileBlock(node->success(), "branchSuccess");
  // continueCurrent gets all the current block's instructions after the branch
  // Unless the branch jumps or returns somewhere, continueCurrent is always executed
  llvm::BasicBlock* continueCurrent = surrounding != nullptr ?
    surrounding : llvm::BasicBlock::Create(*context, "branchAfter", functionStack.top()->getValue());
  if (mpark::holds_alternative<std::nullptr_t>(node->failiure())) {
    // Failiure is nullptr, does not have else clauses
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond->val, success, continueCurrent);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  } else if (mpark::holds_alternative<Node<BlockNode>::Link>(node->failiure())) {
    // Failiure is BlockNode, has an else block
    llvm::BasicBlock* failiure = compileBlock(
      mpark::get<Node<BlockNode>::Link>(node->failiure()),
      "branchFailiure"
    );
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block, unless there already is a terminator
    if (!failiure->getTerminator()) {
      builder->SetInsertPoint(failiure);
      builder->CreateBr(continueCurrent);
      usesBranchAfter = true;
    }
    // Add the branch
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond->val, success, failiure);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  } else if (mpark::holds_alternative<Node<BranchNode>::Link>(node->failiure())) {
    // Failiure is BranchNode, has else-if
    llvm::BasicBlock* nextBranch = llvm::BasicBlock::Create(*context, "branchNext", functionStack.top()->getValue());
    builder->SetInsertPoint(nextBranch);
    compileBranch(mpark::get<Node<BranchNode>::Link>(node->failiure()), continueCurrent);
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond->val, success, nextBranch);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  } else {
    throw InternalError("Unhandled type for variant", {METADATA_PAIRS});
  }
}

void ModuleCompiler::visitLoop(Node<LoopNode>::Link node) {
  for (auto init : node->inits()) {
    visitDeclaration(init);
  }
  // Make the block where we go after we're done with the loopBlock
  auto loopAfter = llvm::BasicBlock::Create(*context, "loopAfter", functionStack.top()->getValue());
  // Make sure break statements know where to go
  node->exitBlock = loopAfter;
  // Make the block that will be looped
  auto loopBlock = llvm::BasicBlock::Create(*context, "loopBlock", functionStack.top()->getValue());
  // Make the block that checks the loop condition
  auto loopCondition = llvm::BasicBlock::Create(*context, "loopCondition", functionStack.top()->getValue());
  // Go to the condtion
  builder->CreateBr(loopCondition);
  builder->SetInsertPoint(loopCondition);
  if (auto cond = node->condition()) {
    auto condValue = compileExpression(cond);
    if (!canBeBoolean(condValue)) {
      throw "Expected boolean expression in loop condition"_type + cond->getTrace();
    }
    // Go to the loop if true, end the loop otherwise
    builder->CreateCondBr(condValue->val, loopBlock, loopAfter);
  } else {
    // There is no condition; unconditionally jump
    builder->CreateBr(loopBlock);
  }
  // Add the code to the loopBlock
  builder->SetInsertPoint(loopBlock);
  for (auto& child : node->code()->getChildren()) {
    child->visit(shared_from_this());
  }
  // Also add the update exprs at the end of the loopBlock
  for (auto update : node->updates()) {
    compileExpression(update);
  }
  // Jump back to the condition to see what we do next
  builder->CreateBr(loopCondition);
  // Keep inserting instructions after the loop
  builder->SetInsertPoint(loopAfter);
}

void ModuleCompiler::visitBreakLoop(Node<BreakLoopNode>::Link node) {
  auto parentLoopNode = node->findAbove([](ASTNode::Link n) {
    if (Node<LoopNode>::dynPtrCast(n) != nullptr) return true;
    return false;
  });
  if (parentLoopNode == nullptr)
    throw "Found break statement outside loop"_syntax + node->getTrace();
  llvm::BasicBlock* exitBlock = Node<LoopNode>::staticPtrCast(parentLoopNode)->exitBlock;
  builder->CreateBr(exitBlock);
}

void ModuleCompiler::visitReturn(Node<ReturnNode>::Link node) {
  static const auto funRetTyMismatch = "Function return type does not match return value";
  
  auto func = node->findAbove<FunctionNode>();
  // If defined, allow return statements outside functions, that end the program and
  // return an exit code
  #ifdef XYLENE_BLOCK_RETURN
    if (func == nullptr)
      func = Node<FunctionNode>::make(FunctionSignature("Integer", {}));
  #else
    if (func == nullptr)
      throw "Found return statement outside of function"_syntax + node->getTrace();
  #endif
  
  auto returnType = func->getSignature().getReturnType();
  // If both return types are void, return void
  if (node->value() == nullptr && returnType.isVoid()) {
    builder->CreateRetVoid();
    return;
  }
  // If the sig and the value don't agree whether or not this is void, it means type mismatch
  if ((node->value() == nullptr) != returnType.isVoid()) {
    throw "{}"_type(funRetTyMismatch) + node->getTrace();
  }
  
  auto returnedValue = compileExpression(node->value());
  auto retId = typeIdFromInfo(returnType, node);
  
  // If the returnedValue's type can't be found in the list of possible return types, get mad
  typeCheck(retId, returnedValue,
    "Function return type ({0}) does not match return value ({1})"_type(
      retId->typeNames(), returnedValue->ty->typeNames()) + node->getTrace());
    
  if (functionStack.top()->getValue()->getReturnType() != returnedValue->val->getType()) {
    bool isUnion = returnedValue->val->getType() == taggedUnionType->getPointerTo();
    if (isUnion) {
      returnedValue->val = builder->CreateBitCast(
        builder->CreateConstGEP1_32(returnedValue->val, 0),
        functionStack.top()->getValue()->getReturnType()->getPointerTo()
      );
    }
    if (returnedValue->hasPointerValue()) {
      returnedValue->val = builder->CreateLoad(returnedValue->val, "loadPtrForReturn");
    }
  }
  builder->CreateRet(returnedValue->val);
}

void ModuleCompiler::visitFunction(Node<FunctionNode>::Link node) {
  // TODO anon functions
  const FunctionSignature& sig = node->getSignature();
  std::vector<llvm::Type*> argTypes {};
  std::vector<std::string> argNames {};
  for (std::pair<std::string, DefiniteTypeInfo> p : sig.getArguments()) {
    argTypes.push_back(typeFromInfo(p.second, node));
    argNames.push_back(p.first);
  }
  llvm::FunctionType* funType = llvm::FunctionType::get(
    typeFromInfo(sig.getReturnType(), node),
    argTypes,
    false
  );
  functionStack.push(std::make_shared<FunctionWrapper>(
    llvm::Function::Create(funType, llvm::Function::ExternalLinkage, node->getIdentifier(), module),
    sig,
    functionTid
  ));
  std::size_t nameIdx = 0;
  for (auto& arg : functionStack.top()->getValue()->getArgumentList()) {
    arg.setName(argNames[nameIdx]);
    nameIdx++;
  }
  // Add the function to the enclosing block's scope
  Node<BlockNode>::Link enclosingBlock = node->findAbove<BlockNode>();
  auto inserted = enclosingBlock->blockFuncs.insert({node->getIdentifier(), functionStack.top()});
  // If it failed, it means the function already exists
  if (!inserted.second) {
    throw "Redefinition of function '{}'"_syntax(node->getIdentifier()) + node->getTrace();
  }
  // Only non-foreign functions have a block after them
  if (!node->isForeign()) compileBlock(node->code(), fmt::format("fun_{}_entryBlock", node->getIdentifier()));
  functionStack.pop();
}

void ModuleCompiler::visitType(Node<TypeNode>::Link node) {
  auto structTy = module->getTypeByName(node->getName());
  // If it's already defined, get the block where it is stored
  ASTNode::Link defBlock = node->findAbove([=](ASTNode::Link n) {
    auto b = Node<BlockNode>::dynPtrCast(n);
    if (!b) return false;
    auto it = std::find_if(ALL(b->blockTypes), [=](AbstractId::Link id) {
      return id->getName() == node->getName();
    });
    if (it != b->blockTypes.end()) return true;
    else return false;
  });
  if (defBlock) {
    throw "Redefinition of type '{}'"_syntax(node->getName()) + node->getTrace();
  }
  // Opaque struct, body gets added after members are processed
  // TODO: warning, these stupid StructTypes aren't uniqued, but their names are at
  // the context level. So make sure we don't do collisions with names
  structTy = llvm::StructType::create(*context, node->getName());
  auto tid = TypeId::create(new TypeData(structTy, shared_from_this(), node));
  node->setTid(tid);
  for (auto& child : node->getChildren()) child->visit(shared_from_this());
  structTy->setBody(tid->getTyData()->getAllocaTypes());
  tid->getTyData()->finalize();
  // We already checked for redefinitions above, so we know it's safe to insert
  Node<BlockNode>::Link enclosingBlock = node->findAbove<BlockNode>();
  enclosingBlock->blockTypes.insert(tid);
  types->insert(tid);
}

ValueWrapper::Link ModuleCompiler::getPtrForArgument(
  TypeId::Link argType,
  FunctionWrapper::Link fun,
  std::size_t which
) {
  if (argType == nullptr) throw InternalError(
    "Called too early; move caller inside or after TypeData::finalize",
    {METADATA_PAIRS}
  );
  if (which >= fun->getValue()->getArgumentList().size()) {
    throw InternalError("Bad argument index", {
      METADATA_PAIRS,
      {"which index", std::to_string(which)}
    });
  }
  // Obtain desired argument + its type's name
  llvm::Argument* argPtr = nullptr;
  std::size_t i = 0;
  for (auto& arg : fun->getValue()->getArgumentList()) {
    if (i == which) {
      argPtr = &arg;
      break;
    }
    i++;
  }
  auto argName = "arg_" + argPtr->getName().str();
  // If we already obtained this arg, fetch it and return it
  if (auto val = fun->getValue()->getValueSymbolTable()->lookup(argName)) {
    return std::make_shared<ValueWrapper>(val, argType);
  }
  // We can't use the function argument as is, so we create a pointer, and
  // store the arg in it, and return the loaded value.
  auto structPtrTy = llvm::PointerType::getUnqual(
    argType->getTyData()->getStructTy()
  );
  llvm::Value* objValue = builder->CreateAlloca(
    structPtrTy,
    nullptr,
    "argAlloc_" + argPtr->getName()
  );
  builder->CreateStore(argPtr, objValue);
  auto value = builder->CreateLoad(structPtrTy, objValue, argName);
  return std::make_shared<ValueWrapper>(value, argType);
}

void ModuleCompiler::visitConstructor(Node<ConstructorNode>::Link node) {
  TypeData* tyData = 
    Node<TypeNode>::staticPtrCast(node->getParent().lock())->getTid()->getTyData();
  auto sig = node->getSignature();
  if (!sig.getReturnType().isVoid()) throw InternalError(
    "Constructor return type not void",
    {METADATA_PAIRS, {"type", tyData->getName()}}
  );
  std::vector<llvm::Type*> argTypes {
    llvm::PointerType::getUnqual(tyData->getStructTy())
  };
  std::vector<std::string> argNames {"this"};
  for (std::pair<std::string, DefiniteTypeInfo> p : sig.getArguments()) {
    argTypes.push_back(typeFromInfo(p.second, node));
    argNames.push_back(p.first);
  }
  FunctionSignature::Arguments newArgs = sig.getArguments();
  newArgs.insert(newArgs.begin(), {"this", StaticTypeInfo(tyData->getName())});
  auto updatedSignature = FunctionSignature(sig.getReturnType(), newArgs);
  llvm::FunctionType* funType = llvm::FunctionType::get(
    llvm::Type::getVoidTy(*context),
    argTypes,
    false
  );
  auto funWrapper = std::make_shared<FunctionWrapper>(
    llvm::Function::Create(
      funType,
      llvm::Function::InternalLinkage,
      "constructor_" + tyData->getName(),
      module
    ),
    updatedSignature,
    functionTid
  );
  std::size_t nameIdx = 0;
  for (auto& arg : funWrapper->getValue()->getArgumentList()) {
    arg.setName(argNames[nameIdx]);
    nameIdx++;
  }
  tyData->addConstructor(ConstructorData(node, funWrapper));
}

void ModuleCompiler::visitMethod(Node<MethodNode>::Link node) {
  TypeData* tyData = Node<TypeNode>::staticPtrCast(node->getParent().lock())->getTid()->getTyData();
  auto sig = node->getSignature();
  std::vector<llvm::Type*> argTypes {};
  std::vector<std::string> argNames {};
  // Non-static methods' first arg is a ptr to their object
  if (!node->isStatic()) {
    argTypes.push_back(tyData->getStructTy());
    argNames.push_back("this");
  }
  for (std::pair<std::string, DefiniteTypeInfo> p : sig.getArguments()) {
    argTypes.push_back(typeFromInfo(p.second, node));
    argNames.push_back(p.first);
  }
  llvm::FunctionType* funType = llvm::FunctionType::get(
    typeFromInfo(sig.getReturnType(), node),
    argTypes,
    false
  );
  auto funWrapper = std::make_shared<FunctionWrapper>(
    llvm::Function::Create(
      funType,
      llvm::Function::InternalLinkage,
      fmt::format("{0}method_{1}_{2}",
        node->isStatic() ? "static_" : "", tyData->getName(), node->getIdentifier()),
      module
    ),
    sig,
    functionTid
  );
  std::size_t nameIdx = 0;
  for (auto& arg : funWrapper->getValue()->getArgumentList()) {
    arg.setName(argNames[nameIdx]);
    nameIdx++;
  }
  // Static methods bodies can be dealt with in here
  // Normal methods are done in TypeData::finalize
  if (node->isStatic() && !node->isForeign()) {
    functionStack.push(funWrapper);
    compileBlock(node->code(), fmt::format("fun_{}_entryBlock", node->getIdentifier()));
    functionStack.pop();
  }
  tyData->addMethod(MethodData(node, node->getIdentifier(), funWrapper), node->isStatic());
}

void ModuleCompiler::visitMember(Node<MemberNode>::Link node) {
  auto tyNode = Node<TypeNode>::staticPtrCast(node->getParent().lock());
  const auto& tyData = tyNode->getTid()->getTyData();
  // This method also makes sure the members are initialized when appropriate
  tyData->addMember(
    MemberMetadata(node, typeFromInfo(node->getTypeInfo(), node)),
    node->isStatic()
  );
}
