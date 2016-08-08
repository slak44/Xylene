#ifndef LLVM_COMPILER_HPP
#define LLVM_COMPILER_HPP

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <string>
#include <vector>

#include "utils/util.hpp"
#include "ast.hpp"

llvm::LLVMContext& globalContext(llvm::getGlobalContext());

static const uint BITS_PER_INT = 64;
static llvm::IntegerType* integerType = llvm::IntegerType::get(globalContext, BITS_PER_INT);

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

  CompileVisitor(llvm::LLVMContext& context, std::string moduleName, AST ast):
    contextRef(context),
    builder(context),
    module(new llvm::Module(moduleName, context)),
    ast(ast) {
    llvm::FunctionType* mainType = llvm::FunctionType::get(integerType, false);
    llvm::Function* mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module);
    currentFunction = mainFunc;
  }
  
public:
  using Link = PtrUtil<CompileVisitor>::Link;
  
  static Link create(llvm::LLVMContext& context, std::string moduleName, AST ast) {
    return PtrUtil<CompileVisitor>::make(CompileVisitor(context, moduleName, ast));
  }
  
  void visit() {
    ast.getRootAsLink()->visit(shared_from_this());
    builder.CreateRet(llvm::ConstantInt::getSigned(integerType, 32));
  }
  
  llvm::Module* getModule() const {
    return module;
  }
private:
  void visitBlock(BlockNode* node) {
    llvm::BasicBlock* block = llvm::BasicBlock::Create(contextRef, "block", currentFunction);
    builder.SetInsertPoint(block);
    for (auto& child : node->getChildren()) child->visit(shared_from_this());
  }
  
  void visitExpression(ExpressionNode* node) {
    compileExpression(Node<ExpressionNode>::make(*node));
  }
  
  InternalError ni = InternalError("Not Implemented", {METADATA_PAIRS});
  
  llvm::Value* compileExpression(Node<ExpressionNode>::Link node) {
    Token tok = node->getToken();
    if (tok.isTerminal()) {
      switch (tok.type) {
        case L_INTEGER: return llvm::ConstantInt::getSigned(integerType, std::stoll(tok.data));
        case L_FLOAT: throw ni;
        case L_STRING: throw ni;
        case L_BOOLEAN: throw ni;
        case IDENTIFIER: throw ni;
        default: throw InternalError("Unhandled terminal symbol in switch case", {
          METADATA_PAIRS,
          {"token", tok.toString()}
        });
      };
    } else if (tok.isOperator()) {
      std::vector<llvm::Value*> operands {};
      for (auto& child : node->getChildren()) {
        operands.push_back(compileExpression(Node<ExpressionNode>::dynPtrCast(child)));
      }
      // TODO use proper operator
      return builder.CreateAdd(operands[0], operands[1], "tmpadd");
    } else {
      throw InternalError("Malformed expression node", {
        METADATA_PAIRS,
        {"token", tok.toString()}
      });
    }
  }
  
  void visitDeclaration(DeclarationNode* node) {
    UNUSED(node);
    throw ni;
  }
  
  void visitBranch(BranchNode* node) {
    UNUSED(node);
    throw ni;
  }
  
  void visitLoop(LoopNode* node) {
    UNUSED(node);
    throw ni;
  }
  
};

#endif
