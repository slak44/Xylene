#ifndef LLVM_COMPILER_HPP
#define LLVM_COMPILER_HPP

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "ast.hpp"
#include "operator.hpp"
#include "operatorCodegen.hpp"

static const InternalError ni = InternalError("Not Implemented", {METADATA_PAIRS});

/*
  Each CompileVisitor creates a LLVM Module from an AST.
*/
class CompileVisitor: public ASTVisitor, public std::enable_shared_from_this<CompileVisitor> {
private:
  llvm::LLVMContext& contextRef;
  llvm::IRBuilder<> builder;
  llvm::Module* module;
  llvm::Function* currentFunction;
  AST ast;

  CompileVisitor(llvm::LLVMContext& context, std::string moduleName, AST ast);
public:
  using Link = PtrUtil<CompileVisitor>::Link;
  
  static Link create(llvm::LLVMContext& context, std::string moduleName, AST ast) {
    return PtrUtil<CompileVisitor>::make(CompileVisitor(context, moduleName, ast));
  }
  
  void visit();
  
  llvm::Module* getModule() const;
  
private:
  void visitExpression(ExpressionNode* node);
  void visitDeclaration(DeclarationNode* node);
  void visitBranch(BranchNode* node);
  void visitLoop(LoopNode* node);  
  void visitReturn(ReturnNode* node);
  void visitBlock(BlockNode* node);
  
  llvm::Value* loadLocal(const std::string& name, uint line);
  
  llvm::Value* compileExpression(Node<ExpressionNode>::Link node);
  // The BasicBlock surrounding is the block where control returns after dealing with branches, only specify for recursive case
  void compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding = nullptr);
  llvm::BasicBlock* compileBlock(Node<BlockNode>::Link node, const std::string& name);
};

#endif
