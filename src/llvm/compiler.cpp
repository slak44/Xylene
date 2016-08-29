#include "llvm/compiler.hpp"

CompileVisitor::CompileVisitor(std::string moduleName, AST ast):
  context(new llvm::LLVMContext()),
  integerType(llvm::IntegerType::get(*context, bitsPerInt)),
  floatType(llvm::Type::getDoubleTy(*context)),
  booleanType(llvm::Type::getInt1Ty(*context)),
  builder(std::make_unique<llvm::IRBuilder<>>(llvm::IRBuilder<>(*context))),
  module(new llvm::Module(moduleName, *context)),
  ast(ast) {
  llvm::FunctionType* mainType = llvm::FunctionType::get(integerType, false);
  functionStack.push(entryPoint = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module));
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
  llvm::BasicBlock* newBlock = llvm::BasicBlock::Create(*context, name, functionStack.top());
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
        if (functionStack.top()->getReturnType()->isVoidTy()) {
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

llvm::Type* CompileVisitor::typeFromName(TypeName name) {
  if (name == "Boolean") return booleanType;
  else if (name == "Integer") return integerType;
  else if (name == "Float") return floatType;
  else throw InternalError("Not Implemented", {METADATA_PAIRS});
  // TODO user-defined types are handled here ^^
  // Should have a map of user types to look into
}

llvm::Type* CompileVisitor::typeFromInfo(TypeInfo ti) {
  if (ti.isVoid()) return llvm::Type::getVoidTy(*context);
  TypeList tl = ti.getEvalTypeList();
  if (tl.size() == 0) throw InternalError("Not Implemented", {METADATA_PAIRS});
  if (tl.size() == 1) return typeFromName(*std::begin(tl));
  // TODO multi type
  throw InternalError("Not Implemented", {METADATA_PAIRS});
}

llvm::Value* CompileVisitor::valueFromIdentifier(Node<ExpressionNode>::Link identifier, IdentifierHandling how) {
  llvm::Value* val;
  // If it's a function arg, return it like this
  for (auto& arg : functionStack.top()->getArgumentList()) {
    if (arg.getName() == identifier->getToken().data) {
      val = &arg;
      if (AS_VALUE) return val;
      // TODO: prob alloc a copy and give that for primitives
      // for complex types, use the llvm::Argument to get ptr to parent function, then recursively search for where the damn thing was allocated, so we can get a pointer
      // also be careful so that anything that uses the thing changes the right data, not a temporary copy
      if (AS_POINTER) throw InternalError("Not Implemented", {METADATA_PAIRS});
    }
  }
  // TODO check more places for this thing, globals etc
  // See if it's in the current block
  val = builder->GetInsertBlock()->getValueSymbolTable()->lookup(identifier->getToken().data);
  // See if it's a function in this module
  if (val == nullptr) val = module->getFunction(identifier->getToken().data);
  // Complain to the user if it still can't be found
  if (val == nullptr) throw Error("ReferenceError", "Cannot find '" + identifier->getToken().data + "' in this scope", identifier->getTrace());
  switch (how) {
    case AS_POINTER: return val;
    case AS_VALUE: return builder->CreateLoad(val, "identifierExpressionLoad");
    default: throw InternalError("Unimplemented ident handler", {METADATA_PAIRS});
  }
}

llvm::Value* CompileVisitor::compileExpression(Node<ExpressionNode>::Link node, IdentifierHandling how) {
  Token tok = node->getToken();
  if (how == AS_POINTER && tok.type != IDENTIFIER && tok.type != OPERATOR) 
    throw Error("ReferenceError", "Operator requires a mutable type", tok.trace);
  if (tok.isTerminal()) {
    switch (tok.type) {
      case L_INTEGER: return llvm::ConstantInt::getSigned(integerType, std::stoll(tok.data));
      case L_FLOAT: return llvm::ConstantFP::get(floatType, tok.data);
      case L_STRING: throw InternalError("Not Implemented", {METADATA_PAIRS});
      case L_BOOLEAN: return (tok.data == "true" ? llvm::ConstantInt::getTrue(booleanType) : llvm::ConstantInt::getFalse(booleanType));
      case IDENTIFIER: return valueFromIdentifier(node, how);
      default: throw InternalError("Unhandled terminal symbol in switch case", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    };
  } else if (tok.isOperator()) {
    std::vector<llvm::Value*> operands {};
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
    auto func = codegen->findAndGetFun(tok, operands);
    // Call the code generating function, and return its result
    return func(operands, tok.trace);
  } else {
    throw InternalError("Malformed expression node", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
}

void CompileVisitor::visitDeclaration(Node<DeclarationNode>::Link node) {
  // TODO make sure dynamic vars and user types are boxed
  TypeList declTypes = node->getTypeInfo().getEvalTypeList();
  // If this variable allows only one type, allocate it immediately
  if (declTypes.size() == 1) {
    TypeName name = *declTypes.begin();
    builder->CreateAlloca(typeFromName(name), nullptr, node->getIdentifier());
  // If this variable has 1+ or dynamic type, allocate a pointer + type data
  // The actual data will be allocated on initialization
  } else {
    // TODO
    throw InternalError("Not Implemented", {METADATA_PAIRS});
  }
  // Handle initialization
  if (node->hasInit()) {
    llvm::Value* initValue = compileExpression(node->getInit());
    llvm::Value* allocated = builder->GetInsertBlock()->getValueSymbolTable()->lookup(node->getIdentifier());
    // if (allocated->getType() == /* type of boxed stuff */) {/* update type metadata in box and do a store on the actual data */}
    builder->CreateStore(initValue, allocated); // TODO only for primitives, move to else block of above comment
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
  llvm::Value* cond = compileExpression(node->getCondition());
  llvm::BasicBlock* success = compileBlock(node->getSuccessBlock(), "branchSuccess");
  // continueCurrent gets all the current block's instructions after the branch
  // Unless the branch jumps or returns somewhere, continueCurrent is always executed
  llvm::BasicBlock* continueCurrent = surrounding != nullptr ? surrounding : llvm::BasicBlock::Create(*context, "branchAfter", functionStack.top());
  if (node->getFailiureBlock() == nullptr) {
    // Does not have else clauses
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond, success, continueCurrent);
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
    builder->CreateCondBr(cond, success, failiure);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  } else {
    // Has else-if as failiure
    llvm::BasicBlock* nextBranch = llvm::BasicBlock::Create(*context, "branchNext", functionStack.top());
    builder->SetInsertPoint(nextBranch);
    compileBranch(Node<BranchNode>::dynPtrCast(node->getFailiureBlock()), continueCurrent);
    builder->SetInsertPoint(current);
    builder->CreateCondBr(cond, success, nextBranch);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  }
}

void CompileVisitor::visitLoop(Node<LoopNode>::Link node) {
  auto init = node->getInit();
  if (init != nullptr) this->visitDeclaration(init);
  // Make the block where we go after we're done with the loopBlock
  auto loopAfter = llvm::BasicBlock::Create(*context, "loopAfter", functionStack.top());
  // Make sure break statements know where to go
  node->setExitBlock(loopAfter);
  // Make the block that will be looped
  auto loopBlock = llvm::BasicBlock::Create(*context, "loopBlock", functionStack.top());
  // Make the block that checks the loop condition
  auto loopCondition = llvm::BasicBlock::Create(*context, "loopCondition", functionStack.top());
  // Go to the condtion
  builder->CreateBr(loopCondition);
  builder->SetInsertPoint(loopCondition);
  auto cond = node->getCondition();
  if (cond != nullptr) {
    auto condValue = compileExpression(cond);
    // Go to the loop if true, end the loop otherwise
    builder->CreateCondBr(condValue, loopBlock, loopAfter);
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
  // TODO get rid of this garbage iteration, make sure this node has a reference to its loop node from the parser, don't search for it like an ape
  auto lastParent = node->getParent();
  for (; ; lastParent = lastParent.lock()->getParent()) {
    if (lastParent.lock() == nullptr) throw Error("SyntaxError", "Found break statement outside loop", node->getTrace());
    // Make sure that at least somewhere up the tree, we have a loop, otherwise the above error is thrown
    if (Node<LoopNode>::dynPtrCast(lastParent.lock()) != nullptr) break;
  }
  llvm::BasicBlock* exitBlock = Node<LoopNode>::dynPtrCast(lastParent.lock())->getExitBlock();
  builder->CreateBr(exitBlock);
}

void CompileVisitor::visitReturn(Node<ReturnNode>::Link node) {
  if (node->getValue() == nullptr) {
    builder->CreateRetVoid();
    return;
  }
  auto result = compileExpression(node->getValue());
  if (functionStack.top()->getReturnType() != result->getType()) {
    // TODO: this is a very indiscriminate check
    // only simple primitives like ints should pass
    // make sure that more complex types throw
    llvm::StoreInst* store = llvm::dyn_cast_or_null<llvm::StoreInst>(result);
    if (store) {
      result = builder->CreateLoad(store->getPointerOperand(), "forceLoadForReturn");
    } else {
      throw Error("TypeError", "Function return type does not match return value", node->getTrace());
    }
  }
  builder->CreateRet(result);
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
  functionStack.push(llvm::Function::Create(funType, llvm::Function::ExternalLinkage, node->getIdentifier(), module));
  std::size_t nameIdx = 0;
  for (auto& arg : functionStack.top()->getArgumentList()) {
    arg.setName(argNames[nameIdx]);
    nameIdx++;
  }
  compileBlock(node->getCode(), "fun_" + node->getIdentifier() + "_entryBlock");
  functionStack.pop();
}
