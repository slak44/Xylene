#include "llvm/compiler.hpp"

CompileVisitor::CompileVisitor(llvm::LLVMContext& context, std::string moduleName, AST ast):
  contextRef(context),
  builder(context),
  module(new llvm::Module(moduleName, context)),
  ast(ast) {
  llvm::FunctionType* mainType = llvm::FunctionType::get(integerType, false);
  currentFunction = entryPoint = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module);
}
  
void CompileVisitor::visit() {
  ast.getRoot()->visit(shared_from_this());
  // If the current block, which is the one that exits from main, has no terminator, add one
  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateRet(llvm::ConstantInt::get(integerType, 0));
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
  llvm::BasicBlock* oldBlock = builder.GetInsertBlock();
  llvm::BasicBlock* newBlock = llvm::BasicBlock::Create(contextRef, name, currentFunction);
  builder.SetInsertPoint(newBlock);
  for (auto& child : node->getChildren()) child->visit(shared_from_this());
  // If the block lacks a terminator instruction, add one
  if (!newBlock->getTerminator()) {
    switch (node->getType()) {
      case ROOT_BLOCK:
        builder.CreateRet(llvm::ConstantInt::get(integerType, 0));
        break;
      case IF_BLOCK:
        // Technically should never happen because visitBranch *should* be properly implemented
        throw InternalError("visitBranch not implemented properly", {METADATA_PAIRS});
      case CODE_BLOCK:
        // Attempt to merge into predecessor
        bool hasMerged = llvm::MergeBlockIntoPredecessor(newBlock);
        if (hasMerged) {
          // We can insert in the old block if it was merged
          builder.SetInsertPoint(oldBlock);
        } else {
          // TODO: what do we do with a block that has no terminator, but can't be merged?
          throw ni;
        }
        break;
      // TODO: add FUNCTION_BLOCK to this enum, and create a ret instruction for it
    }
  }
  return newBlock;
}

void CompileVisitor::visitExpression(Node<ExpressionNode>::Link node) {
  compileExpression(node);
}

TokenType getFromValueType(llvm::Type* ty) {
  if (ty == integerType) return L_INTEGER;
  if (ty == floatType) return L_FLOAT;
  if (ty == booleanType) return L_BOOLEAN;
  // TODO the rest of the types
  throw ni;
}

CodegenFunction identifyCodegenFunction(Token tok, llvm::IRBuilder<>& builder, std::vector<llvm::Value*>& operands) {
  // Name to look for
  const OperatorName& toFind = operatorNameFrom(tok.idx);
  // Function to return
  CodegenFunction func;
  // Check if it's a special case
  auto it = specialCodegenMap.find(toFind);
  if (it != specialCodegenMap.end()) return it->second;
  // Look in the normal map
  auto opMapIt = codegenMap.find(toFind);
  if (opMapIt == codegenMap.end()) {
    throw InternalError("No such operator", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
  // Get a list of types
  std::vector<TokenType> operandTypes;
  operandTypes.resize(operands.size(), UNPROCESSED);
  // Map an operand to a TokenType representing it
  std::size_t idx = -1;
  std::transform(ALL(operands), operandTypes.begin(), [=, &idx, &builder, &operands](llvm::Value* val) -> TokenType {
    idx++;
    llvm::Type* opType = val->getType();
    if (llvm::dyn_cast_or_null<llvm::PointerType>(opType)) {
      auto load = builder.CreateLoad(operands[idx], "loadIdentifier");
      operands[idx] = load;
      opType = operands[idx]->getType();
    }
    return getFromValueType(opType);
  });
  // Try to find the function in the TypeMap using the operand types
  auto funIt = opMapIt->second.find(operandTypes);
  if (funIt == opMapIt->second.end()) {
    throw Error("TypeError", "No operation available for given operands", tok.line);
  }
  func = funIt->second;
  return func;
}

llvm::Value* CompileVisitor::compileExpression(Node<ExpressionNode>::Link node, bool requirePointer) {
  Token tok = node->getToken();
  if (requirePointer && tok.type != IDENTIFIER && tok.type != OPERATOR) 
    throw Error("ReferenceError", "Operator requires a mutable type", tok.line);
  if (tok.isTerminal()) {
    switch (tok.type) {
      case L_INTEGER: return llvm::ConstantInt::getSigned(integerType, std::stoll(tok.data));
      case L_FLOAT: return llvm::ConstantFP::get(floatType, tok.data);
      case L_STRING: throw ni;
      case L_BOOLEAN: return (tok.data == "true" ? llvm::ConstantInt::getTrue(booleanType) : llvm::ConstantInt::getFalse(booleanType));
      case IDENTIFIER: {
        llvm::Value* ptr = builder.GetInsertBlock()->getValueSymbolTable()->lookup(tok.data); // TODO check globals and types, not only locals
        if (requirePointer) return ptr;
        else return builder.CreateLoad(ptr, "identifierExpressionLoad");
      }
      default: throw InternalError("Unhandled terminal symbol in switch case", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    };
  } else if (tok.isOperator()) {
    std::vector<llvm::Value*> operands {};
    // Recursively compute all the operands
    std::size_t idx = 0;
    for (auto& child : node->getChildren()) {
      bool requirePointer = tok.getOperator().getRefList()[idx];
      operands.push_back(compileExpression(Node<ExpressionNode>::dynPtrCast(child), requirePointer));
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
    auto func = identifyCodegenFunction(tok, builder, operands);
    // Call the code generating function, and return its result
    return func(builder, operands);
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
    std::string typeName = *declTypes.begin();
    llvm::Type* toAllocate;
    if (typeName == "Boolean") toAllocate = booleanType;
    else if (typeName == "Integer") toAllocate = integerType;
    else if (typeName == "Float") toAllocate = floatType;
    else throw ni; // TODO user-defined types are handled here
    builder.CreateAlloca(toAllocate, nullptr, node->getIdentifier());
  // If this variable has 1+ or dynamic type, allocate a pointer + type data
  // The actual data will be allocated on initialization
  } else {
    // TODO
    throw ni;
  }
  // Handle initialization
  if (node->hasInit()) {
    llvm::Value* initValue = compileExpression(node->getInit());
    llvm::Value* allocated = builder.GetInsertBlock()->getValueSymbolTable()->lookup(node->getIdentifier());
    // if (allocated->getType() == /* type of boxed stuff */) {/* update type metadata in box and do a store on the actual data */}
    builder.CreateStore(initValue, allocated); // TODO only for primitives, move to else block of above comment
  }
}

void CompileVisitor::visitBranch(Node<BranchNode>::Link node) {
  compileBranch(node);
}

// The BasicBlock surrounding is the block where control returns after dealing with branches, only specify for recursive case
void CompileVisitor::compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding) {
  static const auto handleBranchExit = [this](llvm::BasicBlock* continueCurrent, llvm::BasicBlock* success, bool usesBranchAfter) -> void {
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block
    // Unless the block already goes somewhere else
    if (!success->getTerminator()) {
      builder.SetInsertPoint(success);
      builder.CreateBr(continueCurrent);
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
      builder.SetInsertPoint(continueCurrent);
    }
    return;
  };
  bool usesBranchAfter = false;
  llvm::BasicBlock* current = builder.GetInsertBlock();
  llvm::Value* cond = compileExpression(node->getCondition());
  llvm::BasicBlock* success = compileBlock(node->getSuccessBlock(), "branchSuccess");
  // continueCurrent gets all the current block's instructions after the branch
  // Unless the branch jumps or returns somewhere, continueCurrent is always executed
  llvm::BasicBlock* continueCurrent = surrounding != nullptr ? surrounding : llvm::BasicBlock::Create(contextRef, "branchAfter", currentFunction);
  if (node->getFailiureBlock() == nullptr) {
    // Does not have else clauses
    builder.SetInsertPoint(current);
    builder.CreateCondBr(cond, success, continueCurrent);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  }
  auto blockFailNode = Node<BlockNode>::dynPtrCast(node->getFailiureBlock());
  if (blockFailNode) {
    // Has an else block as failiure
    llvm::BasicBlock* failiure = compileBlock(blockFailNode, "branchFailiure");
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block, unless there already is a terminator
    if (!failiure->getTerminator()) {
      builder.SetInsertPoint(failiure);
      builder.CreateBr(continueCurrent);
      usesBranchAfter = true;
    }
    // Add the branch
    builder.SetInsertPoint(current);
    builder.CreateCondBr(cond, success, failiure);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  } else {
    // Has else-if as failiure
    llvm::BasicBlock* nextBranch = llvm::BasicBlock::Create(contextRef, "branchNext", currentFunction);
    builder.SetInsertPoint(nextBranch);
    compileBranch(Node<BranchNode>::dynPtrCast(node->getFailiureBlock()), continueCurrent);
    builder.SetInsertPoint(current);
    builder.CreateCondBr(cond, success, nextBranch);
    return handleBranchExit(continueCurrent, success, usesBranchAfter);
  }
}

void CompileVisitor::visitLoop(Node<LoopNode>::Link node) {
  UNUSED(node);
  throw ni;
}

void CompileVisitor::visitReturn(Node<ReturnNode>::Link node) {
  auto result = compileExpression(node->getValue());
  if (currentFunction->getReturnType() != result->getType()) {
    // TODO: this is a very indiscriminate check
    // only simple primitives like ints should pass
    // make sure that more complex types throw
    llvm::StoreInst* store = llvm::dyn_cast_or_null<llvm::StoreInst>(result);
    if (store) {
      result = builder.CreateLoad(store->getPointerOperand(), "forceLoadForReturn");
    } else {
      throw Error("TypeError", "Function return type does not match return value", node->getLineNumber());
    }
  }
  builder.CreateRet(result);
}
