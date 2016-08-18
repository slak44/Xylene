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
#include <llvm/Support/raw_os_ostream.h>
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
  llvm::Function* entryPoint;
  llvm::Function* currentFunction;
  AST ast;

  CompileVisitor(llvm::LLVMContext& context, std::string moduleName, AST ast);
public:
  using Link = PtrUtil<CompileVisitor>::Link;
 
  static Link create(llvm::LLVMContext& context, std::string moduleName, AST ast) {
    return std::make_shared<CompileVisitor>(CompileVisitor(context, moduleName, ast));
  }
  
  void visit();
  
  llvm::Module* getModule() const;
  llvm::Function* getEntryPoint() const;
  
private:
  void visitExpression(Node<ExpressionNode>::Link node);
  void visitDeclaration(Node<DeclarationNode>::Link node);
  void visitBranch(Node<BranchNode>::Link node);
  void visitLoop(Node<LoopNode>::Link node);  
  void visitReturn(Node<ReturnNode>::Link node);
  void visitBlock(Node<BlockNode>::Link node);
  
  llvm::Value* compileExpression(Node<ExpressionNode>::Link node, bool requirePointer = false);
  // The BasicBlock surrounding is the block where control returns after dealing with branches, only specify for recursive case
  void compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding = nullptr);
  llvm::BasicBlock* compileBlock(Node<BlockNode>::Link node, const std::string& name);
};

#endif
