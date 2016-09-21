#include "llvm/compiler.hpp"

CompileVisitor::CompileVisitor(std::string moduleName, AST ast):
  context(new llvm::LLVMContext()),
  integerType(llvm::IntegerType::get(*context, bitsPerInt)),
  floatType(llvm::Type::getDoubleTy(*context)),
  booleanType(llvm::Type::getInt1Ty(*context)),
  typeMap({
    // {"Function", ???}, TODO (opaque ptr) OR (i8* + bitcasts everywhere)
    {"Integer", integerType},
    {"Float", floatType},
    {"Boolean", booleanType}
  }),
  builder(std::make_unique<llvm::IRBuilder<>>(llvm::IRBuilder<>(*context))),
  module(new llvm::Module(moduleName, *context)),
  ast(ast) {
  llvm::FunctionType* mainType = llvm::FunctionType::get(integerType, false);
  entryPoint = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module);
  functionStack.push(std::make_shared<FunctionWrapper>(
    entryPoint,
    FunctionSignature(TypeInfo({"Integer"}), {})
  ));
}

CompileVisitor::Link CompileVisitor::create(std::string moduleName, AST ast) {
  auto thisThing = std::make_shared<CompileVisitor>(CompileVisitor(moduleName, ast));
  thisThing->codegen = std::make_unique<OperatorCodegen>(OperatorCodegen(thisThing));
  return thisThing;
}

const std::string CompileVisitor::typeMismatchErrorString = "No operation available for given operands";
  
void CompileVisitor::visit() {
  ast.getRoot()->visit(shared_from_this());
  // If the current block, which is the one that exits from main, has no terminator, add one
  if (!builder->GetInsertBlock()->getTerminator()) {
    builder->CreateRet(llvm::ConstantInt::get(integerType, 0));
  }
  std::string str;
  llvm::raw_string_ostream rso(str);
  if (llvm::verifyModule(*module, &rso)) {
    throw InternalError("Module failed validation", {
      METADATA_PAIRS,
      {"module name", module->getName()},
      {"error", rso.str()}
    });
  }
}

llvm::Module* CompileVisitor::getModule() const {
  return module;
}

llvm::Function* CompileVisitor::getEntryPoint() const {
  return entryPoint;
}

void CompileVisitor::visitBlock(Node<BlockNode>::Link node) {
  compileBlock(node, "block");
}

llvm::BasicBlock* CompileVisitor::compileBlock(Node<BlockNode>::Link node, const std::string& name) {
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

void CompileVisitor::visitExpression(Node<ExpressionNode>::Link node) {
  compileExpression(node);
}

llvm::Type* CompileVisitor::typeFromInfo(TypeInfo ti) {
  if (ti.isVoid()) return llvm::Type::getVoidTy(*context);
  TypeList tl = ti.getEvalTypeList();
  if (tl.size() == 0) throw InternalError("Not Implemented", {METADATA_PAIRS});
  if (tl.size() == 1) return typeMap[*std::begin(tl)];
  // TODO multiple types
  throw InternalError("Not Implemented", {METADATA_PAIRS});
}

ValueWrapper::Link CompileVisitor::valueFromIdentifier(Node<ExpressionNode>::Link identifier) {
  if (identifier->getToken().type != IDENTIFIER) throw InternalError("This function takes identifies only", {METADATA_PAIRS});
  // Check if it's an argument to this function, return it
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
    if (arg->getName() == identifier->getToken().data) {
      auto currentTypeNameIt = std::find_if(ALL(typeMap), [&arg](auto e) {
        return e.second == arg->getType();
      });
      if (currentTypeNameIt == typeMap.end()) throw InternalError("Can't find type of argument", {
        METADATA_PAIRS,
        {"type ptr", getAddressStringFrom(arg->getType())}
      });
      return std::make_shared<ValueWrapper>(&(*arg), currentTypeNameIt->first);
    }
  }
  // Iterate over all the blocks above this identifier
  for (auto p = identifier->findAbove<BlockNode>(); p != nullptr; p = p->findAbove<BlockNode>()) {
    // If we find this ident in any of their variable scopes, return what we found
    auto it = p->blockScope.find(identifier->getToken().data);
    if (it != p->blockScope.end()) {
      ValueWrapper::Link vr = it->second;
      if (!vr->isInitialized())
        throw Error("ReferenceError", "Use of uninitialized value", identifier->getToken().trace);
      return vr;
    }
    // If it's a function, return it
    auto fIt = p->blockFuncs.find(identifier->getToken().data);
    if (fIt != p->blockFuncs.end()) {
      return fIt->second;
    }
  }
  throw Error("ReferenceError", "Cannot find '" + identifier->getToken().data + "' in this scope", identifier->getTrace());
}

DeclarationWrapper::Link CompileVisitor::findDeclaration(Node<ExpressionNode>::Link node) {
  if (node->getToken().type != IDENTIFIER) throw InternalError("This function takes identifies only", {METADATA_PAIRS});
  // Iterate over all the blocks above this identifier
  for (auto p = node->findAbove<BlockNode>(); p != nullptr; p = p->findAbove<BlockNode>()) {
    // If we find this ident in any of their scopes, return what we found
    auto it = p->blockScope.find(node->getToken().data);
    if (it != p->blockScope.end()) {
      return it->second;
    }
  }
  throw Error("ReferenceError", "Cannot find '" + node->getToken().data + "' in this scope", node->getToken().trace);
}

ValueWrapper::Link CompileVisitor::compileExpression(Node<ExpressionNode>::Link node, IdentifierHandling how) {
  Token tok = node->getToken();
  if (how == AS_POINTER && tok.type != IDENTIFIER && tok.type != OPERATOR)
    throw Error("ReferenceError", "Operator requires a mutable type", tok.trace);
  if (tok.isTerminal()) {
    switch (tok.type) {
      case L_INTEGER: return std::make_shared<ValueWrapper>(llvm::ConstantInt::getSigned(integerType, std::stoll(tok.data)), "Integer");
      case L_FLOAT: return std::make_shared<ValueWrapper>(llvm::ConstantFP::get(floatType, tok.data), "Float");
      case L_STRING: throw InternalError("Not Implemented", {METADATA_PAIRS});
      case L_BOOLEAN: {
        auto b = tok.data == "true" ?
          llvm::ConstantInt::getTrue(booleanType) :
          llvm::ConstantInt::getFalse(booleanType);
        return std::make_shared<ValueWrapper>(b, "Boolean");
      }
      case IDENTIFIER: return valueFromIdentifier(node);
      default: throw InternalError("Unhandled terminal symbol in switch case", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    };
  } else if (tok.isOperator()) {
    std::vector<ValueWrapper::Link> operands {};
    // Do some magic for function calls
    if (tok.hasOperatorSymbol("()")) {
      // First arg to calls is the thing being called
      // Should be a pointer
      operands.push_back(compileExpression(node->at(0), AS_POINTER));
      // Compute arguments and add them too
      auto lastNode = node->at(1);
      // If it's not an operator, it means this func call only has one argument
      if (!lastNode->getToken().isOperator()) {
        operands.push_back(compileExpression(lastNode, AS_VALUE));
      // Only go through args if it isn't a no-op, because that means we have no args
      } else if (operatorNameFrom(lastNode->getToken().idx) != "No-op") {
        while (lastNode->at(1)->getToken().hasOperatorSymbol(",")) {
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
        bool requirePointer = tok.getOperator().getRefList()[idx];
        operands.push_back(compileExpression(Node<ExpressionNode>::staticPtrCast(child), requirePointer ? AS_POINTER : AS_VALUE));
        idx++;
      }
      // Make sure we have the correct amount of operands
      if (static_cast<int>(operands.size()) != tok.getOperator().getArity()) {
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

void CompileVisitor::visitDeclaration(Node<DeclarationNode>::Link node) {
  Node<BlockNode>::Link enclosingBlock = Node<BlockNode>::staticPtrCast(node->findAbove<BlockNode>());
  // TODO make sure dynamic vars and user types are boxed
  TypeList declTypes = node->getTypeInfo().getEvalTypeList();
  llvm::Value* decl;
  // If this variable allows only one type, allocate it immediately
  if (declTypes.size() == 1) {
    TypeName name = *declTypes.begin();
    decl = builder->CreateAlloca(typeMap[name], nullptr, node->getIdentifier());
  // If this variable has 1+ or dynamic type, allocate a pointer + type data
  // The actual data will be allocated on initialization
  } else {
    // TODO
    throw InternalError("Not Implemented", {METADATA_PAIRS});
  }
  // Handle initialization
  ValueWrapper::Link initValue;
  if (node->hasInit()) {
    initValue = compileExpression(node->getInit());
    // Check that the type of the initialization is allowed by the declaration
    if (!isTypeAllowedIn(declTypes, initValue->getCurrentTypeName())) {
      throw Error("TypeError", typeMismatchErrorString, node->getTrace());
    }
    // if (allocated->getType() == /* type of boxed stuff */) {/* update type metadata in box and do a store on the actual data */}
    builder->CreateStore(initValue->getValue(), decl); // TODO only for primitives, move to else block of above comment
  }
  // Add to scope
  auto inserted = enclosingBlock->blockScope.insert({
    node->getIdentifier(),
    std::make_shared<DeclarationWrapper>(
      decl,
      node->hasInit() ? initValue->getCurrentTypeName() : "",
      declTypes
    )
  });
  // If it failed, it means the decl already exists
  if (!inserted.second) {
    throw Error("ReferenceError", "Can't have 2 declarations with the same name", node->getTrace());
  }
}

void CompileVisitor::visitBranch(Node<BranchNode>::Link node) {
  compileBranch(node);
}

// The BasicBlock surrounding is the block where control returns after dealing with branches, only specify for recursive case
void CompileVisitor::compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding) {
  const auto handleBranchExit = [this](llvm::BasicBlock* continueCurrent, llvm::BasicBlock* success, bool usesBranchAfter) -> void {
    // Unless the block already goes somewhere else
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block
    if (!success->getTerminator()) {
      builder->SetInsertPoint(success);
      builder->CreateBr(continueCurrent);
      usesBranchAfter = true;
    }
    // If the branchAfter block is not jumped to by anyone, get rid of it
    // Otherwise, the rest of the block's instructions should be added to it
    if (
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
  if (!cond->canBeBooleanValue()) {
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

void CompileVisitor::visitLoop(Node<LoopNode>::Link node) {
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
    if (!condValue->canBeBooleanValue()) {
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

void CompileVisitor::visitBreakLoop(Node<BreakLoopNode>::Link node) {
  auto parentLoopNode = node->findAbove([](ASTNode::Link n) {
    if (Node<LoopNode>::dynPtrCast(n) != nullptr) return true;
    return false;
  });
  if (parentLoopNode == nullptr) throw Error("SyntaxError", "Found break statement outside loop", node->getTrace());
  llvm::BasicBlock* exitBlock = Node<LoopNode>::staticPtrCast(parentLoopNode)->getExitBlock();
  builder->CreateBr(exitBlock);
}

void CompileVisitor::visitReturn(Node<ReturnNode>::Link node) {
  static const auto funRetTyMismatch = "Function return type does not match return value";
  auto func = node->findAbove<FunctionNode>();
  // TODO: for now, don't strictly enforce this. Instead, these returns will exit from the main llvm func.
  // There must be a better way to return an exit code than just using "return 0" in main
  // Maybe link in "abort" and terminate the program with that,
  // and keep this wonderful hack for tests, so it doesn't terminate the test executable
  // if (func == nullptr) throw Error("SyntaxError", "Found return statement outside of function", node->getTrace());
  if (func == nullptr) func = Node<FunctionNode>::make(FunctionSignature(TypeInfo({"Integer"}), {}));
  
  // If the sig and the value don't agree whether or not this is void, it means type mismatch
  // (yes, the ^ is xor)
  auto returnType = func->getSignature().getReturnType();
  if ((node->getValue() == nullptr) ^ returnType.isVoid()) {
    throw Error("TypeError", funRetTyMismatch, node->getTrace());
  }
  // If both return types are void, return void
  if (node->getValue() == nullptr && returnType.isVoid()) {
    builder->CreateRetVoid();
    return;
  }
  auto returnedValue = compileExpression(node->getValue());
  // If the returnedValue's type can't be found in the list of possible return types, get mad
  if (!isTypeAllowedIn(returnType.getEvalTypeList(), returnedValue->getCurrentTypeName())) {
    throw Error("TypeError", funRetTyMismatch, node->getTrace());
  }
  // This makes sure primitives get passed by value
  if (functionStack.top()->getValue()->getReturnType() != returnedValue->getValue()->getType()) {
    auto tyName = returnedValue->getCurrentTypeName();
    // TODO: there may be other types that need forceful pointer loading
    if (returnedValue->hasPointerValue() && (tyName == "Integer" || tyName == "Float" || tyName == "Boolean")) {
      returnedValue->setValue(
        builder->CreateLoad(returnedValue->getValue(), "forceLoadForReturningPrimitives"),
        tyName
      );
    }
  }
  builder->CreateRet(returnedValue->getValue());
}

void CompileVisitor::visitFunction(Node<FunctionNode>::Link node) {
  // TODO anon functions
  const FunctionSignature& sig = node->getSignature();
  std::vector<llvm::Type*> argTypes {};
  std::vector<std::string> argNames {};
  for (std::pair<std::string, DefiniteTypeInfo> p : sig.getArguments()) {
    argTypes.push_back(typeFromInfo(p.second));
    argNames.push_back(p.first);
  }
  llvm::FunctionType* funType = llvm::FunctionType::get(typeFromInfo(sig.getReturnType()), argTypes, false);
  functionStack.push(std::make_shared<FunctionWrapper>(
    llvm::Function::Create(funType, llvm::Function::ExternalLinkage, node->getIdentifier(), module),
    sig
  ));
  std::size_t nameIdx = 0;
  for (auto& arg : functionStack.top()->getValue()->getArgumentList()) {
    arg.setName(argNames[nameIdx]);
    nameIdx++;
  }
  // Add the function to the enclosing block's scope
  Node<BlockNode>::Link enclosingBlock = Node<BlockNode>::staticPtrCast(node->findAbove<BlockNode>());
  auto inserted = enclosingBlock->blockFuncs.insert({node->getIdentifier(), functionStack.top()});
  // If it failed, it means the function already exists
  if (!inserted.second) {
    throw Error("ReferenceError", "Can't have 2 functions with the same name", node->getTrace());
  }
  // Only non-foreign functions have a block after them
  if (!node->isForeign()) compileBlock(node->getCode(), "fun_" + node->getIdentifier() + "_entryBlock");
  functionStack.pop();
}

void CompileVisitor::visitType(Node<TypeNode>::Link node) {
  // Opaque struct, body gets added after members are processed
  auto structTy = llvm::StructType::create(*context, node->getName());
  node->setTyData(new TypeData(structTy, shared_from_this(), node));
  for (auto& child : node->getChildren()) child->visit(shared_from_this());
  structTy->setBody(node->getTyData()->getStructMembers());
  typeMap.insert({node->getName(), structTy});
}

void CompileVisitor::visitConstructor(Node<ConstructorNode>::Link node) {
  UNUSED(node);
  throw InternalError("Unimplemented", {METADATA_PAIRS});
}

void CompileVisitor::visitMethod(Node<MethodNode>::Link node) {
  UNUSED(node);
  throw InternalError("Unimplemented", {METADATA_PAIRS});
}

void CompileVisitor::visitMember(Node<MemberNode>::Link node) {
  auto tyNode = Node<TypeNode>::staticPtrCast(node->getParent().lock());
  auto nameFrom = [&tyNode](std::string prefix, std::string nameOfThing) -> std::string {
    return prefix + "_" + tyNode->getName() + "_" + nameOfThing;
  };
  auto tyData = tyNode->getTyData();
  if (node->isStatic()) {
    llvm::GlobalVariable* staticVar = new llvm::GlobalVariable(
      *module,
      typeFromInfo(node->getTypeInfo()),
      false,
      llvm::GlobalValue::InternalLinkage,
      nullptr,
      nameFrom("static_member", node->getIdentifier())
    );
    if (node->hasInit()) {
      auto currentBlock = builder->GetInsertBlock();
      tyData->builderToStaticInit();
      // Do the initialization
      auto initValue = compileExpression(node->getInit());
      if (!isTypeAllowedIn(node->getTypeInfo().getEvalTypeList(), initValue->getCurrentTypeName())) {
        throw Error("TypeError", "Static member initialization does not match its type", node->getInit()->getTrace());
      }
      builder->CreateStore(initValue->getValue(), staticVar);
      // Exit static initializer
      functionStack.pop();
      builder->SetInsertPoint(currentBlock);
    }
    return;
  }
  tyData->addStructMember(typeFromInfo(node->getTypeInfo()), node->getIdentifier());
  if (node->hasInit()) {
    auto currentBlock = builder->GetInsertBlock();
    tyData->builderToInit();
    // Do the initialization
    auto initValue = compileExpression(node->getInit());
    if (!isTypeAllowedIn(node->getTypeInfo().getEvalTypeList(), initValue->getCurrentTypeName())) {
      throw Error("TypeError", "Member initialization does not match its type", node->getInit()->getTrace());
    }
    auto structElemPtr = builder->CreateGEP(
      tyData->getStructTy(),
      tyData->getInitStructArg(),
      llvm::ConstantInt::getSigned(integerType, tyData->getStructMemberIdx())
    );
    builder->CreateStore(initValue->getValue(), structElemPtr);
    // Exit initializer
    functionStack.pop();
    builder->SetInsertPoint(currentBlock);
  }
}
