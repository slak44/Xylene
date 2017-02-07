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
    TokenParser::parse(lx->getTokens())
  );
  mc->addMainFunction();
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

ModuleCompiler::ModuleCompiler(std::string moduleName, AST ast):
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
  ast(ast) {
  // Insert declarations for these functions in IR, they are linked in later
  constexpr const std::size_t rtFunCount = 4;
  constexpr const std::array<const char*, rtFunCount> runtimeFuncs = {
    "_xyl_checkTypeCompat",
    "_xyl_typeOf",
    "_xyl_typeErrIfIncompatible",
    "_xyl_finish"
  };
  using FT = llvm::FunctionType;
  using FunTyArgs = std::vector<llvm::Type*>;
  const std::array<llvm::FunctionType*, rtFunCount> funTys {
    FT::get(booleanType, FunTyArgs {taggedUnionPtrType, taggedUnionPtrType}, false),
    FT::get(voidPtrType, FunTyArgs {taggedUnionPtrType}, false), // TODO return string type
    FT::get(voidType, FunTyArgs {taggedUnionPtrType, taggedUnionPtrType}, false),
    FT::get(voidType, FunTyArgs {voidPtrType, integerType}, false) // TODO arg1 is string type
  };
  for (std::size_t i = 0; i < rtFunCount; i++) {
    if (module->getFunction(runtimeFuncs[i]) != nullptr) continue;
    auto fun = llvm::Function::Create(
      funTys[i], llvm::Function::ExternalLinkage, runtimeFuncs[i], module);
    fun->deleteBody();
    ast.getRoot()->blockFuncs.insert({
      std::string(runtimeFuncs[i]),
      std::make_shared<FunctionWrapper>(fun, FunctionSignature("", {}), functionTid)
    });
  }
}

ModuleCompiler::Link ModuleCompiler::create(
  const ProgramData::TypeSet& types,
  std::string moduleName,
  AST ast
) {
  auto thisThing =
    std::make_shared<ModuleCompiler>(ModuleCompiler(moduleName, ast));
  thisThing->types = std::make_unique<ProgramData::TypeSet>(types);
  thisThing->ast.getRoot()->blockTypes = {
    thisThing->integerTid,
    thisThing->floatTid,
    thisThing->booleanTid,
    thisThing->functionTid
  };
  thisThing->codegen = std::make_unique<OperatorCodegen>(OperatorCodegen(thisThing));
  return thisThing;
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
}

llvm::Module* ModuleCompiler::getModule() const {
  return module;
}

llvm::Function* ModuleCompiler::getEntryPoint() const {
  return entryPoint->getValue();
}

void ModuleCompiler::visitBlock(Node<BlockNode>::Link node) {
  compileBlock(node, "block");
}

llvm::BasicBlock* ModuleCompiler::compileBlock(Node<BlockNode>::Link node, const std::string& name) {
  llvm::BasicBlock* oldBlock = builder->GetInsertBlock();
  llvm::BasicBlock* newBlock = llvm::BasicBlock::Create(*context, name, functionStack.top()->getValue());
  builder->SetInsertPoint(newBlock);
  for (auto& child : node->getChildren()) child->visit(shared_from_this());
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
          throw Error("SyntaxError", "Function has non-void return type, but no return was found", node->getTrace());
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
  return val->getCurrentType() == booleanTid;
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
  if (result == nullptr) throw Error(
    "TypeError",
    "Can't find type '" + ti.getTypeNameString() + "'",
    node->getTrace()
  );
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
        throw Error("ReferenceError", "Use of uninitialized value", identifier->getToken().trace);
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
      thisObj->getValue(),
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
  throw Error("ReferenceError", "Cannot find '" + name + "' in this scope", identifier->getTrace());
}

ValueWrapper::Link ModuleCompiler::compileExpression(Node<ExpressionNode>::Link node, IdentifierHandling how) {
  Token tok = node->getToken();
  if (how == AS_POINTER && tok.type != TT::IDENTIFIER && tok.type != TT::OPERATOR)
    throw Error("ReferenceError", "Operator requires a mutable type", tok.trace);
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
      // Compute arguments and add them too
      auto lastNode = node->at(0);
      // If it's not an comma, it means this func call only has one argument
      if (!(lastNode->getToken().isOp() && lastNode->getToken().op().hasSymbol(","))) {
        operands.push_back(compileExpression(lastNode, AS_VALUE));
      // Only go through args if it isn't a no-op, because that means we have no args
      } else if (!lastNode->getToken().op().hasName("No-op")) {
        while (lastNode->at(1)->getToken().op().hasSymbol(",")) {
          // TODO might need to change these AS_VALUE for complex objects
          operands.push_back(compileExpression(lastNode->at(0), AS_VALUE));
          lastNode = lastNode->at(1);
        }
        // The last comma's args are not processed in the loop
        operands.push_back(compileExpression(lastNode->at(0), AS_VALUE));
        operands.push_back(compileExpression(lastNode->at(1), AS_VALUE));
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
    return codegen->findAndRunFun(node, operands);
  } else {
    throw InternalError("Malformed expression node", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
}

ValueWrapper::Link ModuleCompiler::boxPrimitive(ValueWrapper::Link p) {
  if (p->getValue()->getType() == llvm::PointerType::getUnqual(taggedUnionType))
    return p;
  auto box = builder->CreateAlloca(taggedUnionType, nullptr, "boxPrimitive");
  // TODO: store value into the box
  return std::make_shared<ValueWrapper>(box, p->getCurrentType());
}

void ModuleCompiler::insertRuntimeTypeCheck(
  DeclarationWrapper::Link target,
  ValueWrapper::Link newValue
) {
  builder->CreateCall(
    module->getFunction("_xyl_typeErrIfIncompatible"),
    {target->getValue(), boxPrimitive(newValue)->getValue()}
  );
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
  }
  auto id = typeIdFromInfo(node->getTypeInfo(), node);
  auto declWrap = std::make_shared<DeclarationWrapper>(decl, id);
  
  // Add to scope
  auto inserted =
    enclosingBlock->blockScope.insert({node->getIdentifier(), declWrap});
  // If it failed, it means the decl already exists
  if (!inserted.second) throw Error("ReferenceError",
      "Redefinition of " + node->getIdentifier(), node->getTrace());
  
  // Handle initialization
  if (!node->hasInit()) return;
  ValueWrapper::Link initValue = compileExpression(node->getInit());
  // Check that the type of the initialization is allowed by the declaration
  if (!isTypeAllowedIn(id, initValue->getCurrentType())) {
    throw Error("TypeError",
      "Type of initialization is not allowed by declaration", node->getTrace());
  }
  if (decl->getType() == llvm::PointerType::getUnqual(taggedUnionType)) {
    insertRuntimeTypeCheck(declWrap, initValue);
    // TODO: at runtime, dynamically allocate the actual data here and store it
  } else {
    builder->CreateStore(initValue->getValue(), decl);
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
  ValueWrapper::Link cond = compileExpression(node->getCondition());
  if (!canBeBoolean(cond)) {
    throw Error("TypeError", "Expected boolean expression in if condition", node->getCondition()->getTrace());
  }
  llvm::BasicBlock* success = compileBlock(node->getSuccessBlock(), "branchSuccess");
  // continueCurrent gets all the current block's instructions after the branch
  // Unless the branch jumps or returns somewhere, continueCurrent is always executed
  llvm::BasicBlock* continueCurrent = surrounding != nullptr ?
    surrounding : llvm::BasicBlock::Create(*context, "branchAfter", functionStack.top()->getValue());
  if (node->getFailiureBlock() == nullptr) {
    // Does not have else clauses
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond->getValue(), success, continueCurrent);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  }
  auto blockFailNode = Node<BlockNode>::dynPtrCast(node->getFailiureBlock());
  if (blockFailNode != nullptr) {
    // Has an else block as failiure
    llvm::BasicBlock* failiure = compileBlock(blockFailNode, "branchFailiure");
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block, unless there already is a terminator
    if (!failiure->getTerminator()) {
      builder->SetInsertPoint(failiure);
      builder->CreateBr(continueCurrent);
      usesBranchAfter = true;
    }
    // Add the branch
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond->getValue(), success, failiure);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  } else {
    // Has else-if as failiure
    llvm::BasicBlock* nextBranch = llvm::BasicBlock::Create(*context, "branchNext", functionStack.top()->getValue());
    builder->SetInsertPoint(nextBranch);
    compileBranch(Node<BranchNode>::dynPtrCast(node->getFailiureBlock()), continueCurrent);
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond->getValue(), success, nextBranch);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  }
}

void ModuleCompiler::visitLoop(Node<LoopNode>::Link node) {
  auto init = node->getInit();
  if (init != nullptr) this->visitDeclaration(init);
  // Make the block where we go after we're done with the loopBlock
  auto loopAfter = llvm::BasicBlock::Create(*context, "loopAfter", functionStack.top()->getValue());
  // Make sure break statements know where to go
  node->setExitBlock(loopAfter);
  // Make the block that will be looped
  auto loopBlock = llvm::BasicBlock::Create(*context, "loopBlock", functionStack.top()->getValue());
  // Make the block that checks the loop condition
  auto loopCondition = llvm::BasicBlock::Create(*context, "loopCondition", functionStack.top()->getValue());
  // Go to the condtion
  builder->CreateBr(loopCondition);
  builder->SetInsertPoint(loopCondition);
  auto cond = node->getCondition();
  if (cond != nullptr) {
    auto condValue = compileExpression(cond);
    if (!canBeBoolean(condValue)) {
      throw Error("TypeError", "Expected boolean expression in loop condition", cond->getTrace());
    }
    // Go to the loop if true, end the loop otherwise
    builder->CreateCondBr(condValue->getValue(), loopBlock, loopAfter);
  } else {
    // There is no condition; unconditionally jump
    builder->CreateBr(loopBlock);
  }
  // Add the code to the loopBlock
  builder->SetInsertPoint(loopBlock);
  auto code = node->getCode();
  for (auto& child : code->getChildren()) {
    child->visit(shared_from_this());
  }
  // Also add the update expr at the end of the loopBlock
  auto update = node->getUpdate();
  if (update != nullptr) compileExpression(update);
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
  if (parentLoopNode == nullptr) throw Error("SyntaxError", "Found break statement outside loop", node->getTrace());
  llvm::BasicBlock* exitBlock = Node<LoopNode>::staticPtrCast(parentLoopNode)->getExitBlock();
  builder->CreateBr(exitBlock);
}

void ModuleCompiler::visitReturn(Node<ReturnNode>::Link node) {
  static const auto funRetTyMismatch = "Function return type does not match return value";
  auto func = node->findAbove<FunctionNode>();
  // TODO: for now, don't strictly enforce this. Instead, these returns will exit from the main llvm func.
  // There must be a better way to return an exit code than just using "return 0" in main
  // Maybe link in "abort" and terminate the program with that,
  // and keep this wonderful hack for tests, so it doesn't terminate the test executable
  // if (func == nullptr) throw Error("SyntaxError", "Found return statement outside of function", node->getTrace());
  if (func == nullptr) func = Node<FunctionNode>::make(FunctionSignature("Integer", {}));
  
  auto returnType = func->getSignature().getReturnType();
  
  // If both return types are void, return void
  if (node->getValue() == nullptr && returnType.isVoid()) {
    builder->CreateRetVoid();
    return;
  }
  // If the sig and the value don't agree whether or not this is void, it means type mismatch
  if ((node->getValue() == nullptr) != returnType.isVoid()) {
    throw Error("TypeError", funRetTyMismatch, node->getTrace());
  }
  
  auto returnedValue = compileExpression(node->getValue());
  // If the returnedValue's type can't be found in the list of possible return types, get mad
  if (!isTypeAllowedIn(typeIdFromInfo(returnType, node), returnedValue->getCurrentType())) {
    throw Error("TypeError", funRetTyMismatch, node->getTrace());
  }
  // This makes sure primitives get passed by value
  if (functionStack.top()->getValue()->getReturnType() != returnedValue->getValue()->getType()) {
    auto ty = returnedValue->getCurrentType();
    // TODO: there may be other types that need forceful pointer loading
    // TODO: actually anything not an object should kinda be passed by value
    bool needsLoading = ty == integerTid || ty == booleanTid || ty == floatTid;
    if (returnedValue->hasPointerValue() && needsLoading) {
      returnedValue->setValue(
        builder->CreateLoad(returnedValue->getValue(), "loadForPrimitiveReturn"),
        returnedValue->getCurrentType()
      );
    }
  }
  builder->CreateRet(returnedValue->getValue());
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
    throw Error("ReferenceError", "Can't have 2 functions with the same name", node->getTrace());
  }
  // Only non-foreign functions have a block after them
  if (!node->isForeign()) compileBlock(node->getCode(), "fun_" + node->getIdentifier() + "_entryBlock");
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
    throw Error(
      "SyntaxError",
      "Redefinition of type " + node->getName(),
      node->getTrace()
    );
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
  if (auto val = fun->getValue()->getValueSymbolTable().lookup(argName)) {
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
      std::string(node->isStatic() ? "static" : "") + "method_" + tyData->getName() + "_" + node->getIdentifier(),
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
    compileBlock(node->getCode(), "fun_" + node->getIdentifier() + "_entryBlock");
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
