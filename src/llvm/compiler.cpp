#include "llvm/compiler.hpp"

CompileVisitor::CompileVisitor(llvm::LLVMContext& context, std::string moduleName, AST ast):
  contextRef(context),
  builder(context),
  module(new llvm::Module(moduleName, context)),
  ast(ast) {
  llvm::FunctionType* mainType = llvm::FunctionType::get(integerType, false);
  llvm::Function* mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module);
  currentFunction = mainFunc;
}
  
void CompileVisitor::visit() {
  ast.getRoot()->visit(shared_from_this());
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

void CompileVisitor::visitBlock(BlockNode* node) {
  compileBlock(Node<BlockNode>::make(*node), "block");
}

llvm::BasicBlock* CompileVisitor::compileBlock(Node<BlockNode>::Link node, const std::string& name) {
  llvm::BasicBlock* prevBlock = builder.GetInsertBlock();
  llvm::BasicBlock* newBlock = llvm::BasicBlock::Create(contextRef, name, currentFunction);
  builder.SetInsertPoint(newBlock);
  for (auto& child : node->getChildren()) child->visit(shared_from_this());
  // Go back to previous insertion point
  if (prevBlock) builder.SetInsertPoint(prevBlock);
  return newBlock;
}

void CompileVisitor::visitExpression(ExpressionNode* node) {
  compileExpression(Node<ExpressionNode>::make(*node));
}

llvm::Value* CompileVisitor::loadLocal(const std::string& name, uint line) {
  llvm::Value* localThing = builder.GetInsertBlock()->getValueSymbolTable()->lookup(name);
  if (localThing != nullptr) {
    return builder.CreateLoad(localThing, name);
  } else {
    throw Error("ReferenceError", "Could not find variable " + name, line);
  }
}

llvm::Value* CompileVisitor::compileExpression(Node<ExpressionNode>::Link node) {
  Token tok = node->getToken();
  if (tok.isTerminal()) {
    switch (tok.type) {
      case L_INTEGER: return llvm::ConstantInt::getSigned(integerType, std::stoll(tok.data));
      case L_FLOAT: return llvm::ConstantFP::get(floatType, tok.data);
      case L_STRING: throw ni;
      case L_BOOLEAN: return (tok.data == "true" ? llvm::ConstantInt::getTrue(booleanType) : llvm::ConstantInt::getFalse(booleanType));
      case IDENTIFIER: return loadLocal(tok.data, tok.line); // TODO check globals and types, not only locals
      default: throw InternalError("Unhandled terminal symbol in switch case", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    };
  } else if (tok.isOperator()) {
    std::vector<llvm::Value*> operands {};
    // Recursively compute all the operands
    for (auto& child : node->getChildren()) {
      operands.push_back(compileExpression(Node<ExpressionNode>::dynPtrCast(child)));
    }
    // Make sure we have the correct amount of operands
    if (static_cast<int>(operands.size()) != tok.getOperator().getArity()) {
      throw InternalError("Operand count does not match operator arity", {
        METADATA_PAIRS,
        {"operator token", tok.toString()},
        {"operand count", std::to_string(operands.size())}
      });
    }
    TypeMap map;
    // Try to find the operator in the map
    try {
      map = codegenMap.at(operatorNameFrom(tok.idx));
    } catch (std::out_of_range& oor) {
      throw InternalError("No such operator", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    }
    std::vector<TokenType> operandTypes;
    operandTypes.resize(operands.size(), UNPROCESSED);
    // Map an operand to a TokenType representing it
    std::transform(ALL(operands), operandTypes.begin(), [=](llvm::Value* val) -> TokenType {
      llvm::Type* opType = val->getType();
      if (opType == integerType) return L_INTEGER;
      else if (opType == floatType) return L_FLOAT;
      else if (opType == booleanType) return L_BOOLEAN;
      else throw ni;
      // TODO the rest of the types
    });
    CodegenFunction func;
    // Try to find the function in the TypeMap using the operand types
    try {
      func = map.at(operandTypes);
    } catch (std::out_of_range& oor) {
      throw Error("TypeError", "No operation available for given operands", tok.line);
    }
    // Call the code generating function, and return its result
    return func(builder, operands);
  } else {
    throw InternalError("Malformed expression node", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
}

void CompileVisitor::visitDeclaration(DeclarationNode* node) {
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

void CompileVisitor::visitBranch(BranchNode* node) {
  compileBranch(Node<BranchNode>::make(*node));
}

// The BasicBlock surrounding is the block where control returns after dealing with branches, only specify for recursive case
void CompileVisitor::compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding) {
  static const auto handleBranchExit = [this](llvm::BasicBlock* continueCurrent) -> void {
    builder.SetInsertPoint(continueCurrent);
    return;
  };
  llvm::BasicBlock* current = builder.GetInsertBlock();
  llvm::Value* cond = compileExpression(node->getCondition());
  llvm::BasicBlock* success = compileBlock(node->getSuccessBlock(), "branchSuccess");
  // continueCurrent gets all the current block's instructions after the branch
  // Unless the branch jumps or returns somewhere, continueCurrent is always executed
  llvm::BasicBlock* continueCurrent = surrounding != nullptr ? surrounding : llvm::BasicBlock::Create(contextRef, "branchAfter", currentFunction);
  // Jump back to continueCurrent after the branch is done, to execute the rest of the block
  builder.SetInsertPoint(success);
  builder.CreateBr(continueCurrent);
  if (node->getFailiureBlock() == nullptr) {
    // Does not have else clauses
    builder.SetInsertPoint(current);
    builder.CreateCondBr(cond, success, continueCurrent);
    return handleBranchExit(continueCurrent);
  }
  auto blockFailNode = Node<BlockNode>::dynPtrCast(node->getFailiureBlock());
  if (blockFailNode) {
    // Has an else block as failiure
    llvm::BasicBlock* failiure = compileBlock(blockFailNode, "branchFailiure");
    // Jump back to continueCurrent after the branch is done, to execute the rest of the block
    builder.SetInsertPoint(failiure);
    builder.CreateBr(continueCurrent);
    // Add the branch
    builder.SetInsertPoint(current);
    builder.CreateCondBr(cond, success, failiure);
    return handleBranchExit(continueCurrent);
  } else {
    // Has else-if as failiure
    llvm::BasicBlock* nextBranch = llvm::BasicBlock::Create(contextRef, "branchNext", currentFunction);
    builder.SetInsertPoint(nextBranch);
    compileBranch(Node<BranchNode>::dynPtrCast(node->getFailiureBlock()), continueCurrent);
    builder.SetInsertPoint(current);
    builder.CreateCondBr(cond, success, nextBranch);
    return handleBranchExit(continueCurrent);
  }
}

void CompileVisitor::visitLoop(LoopNode* node) {
  UNUSED(node);
  throw ni;
}

void CompileVisitor::visitReturn(ReturnNode* node) {
  builder.CreateRet(compileExpression(node->getValue()));
}
