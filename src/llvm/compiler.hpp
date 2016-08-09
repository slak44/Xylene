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
#include "utils/error.hpp"
#include "ast.hpp"
#include "operator.hpp"
#include "globalTypes.hpp"
#include "operatorCodegen.hpp"

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
  }
  
  llvm::Module* getModule() const {
    return module;
  }
  
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
        case L_FLOAT: return llvm::ConstantFP::get(floatType, tok.data);
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
        map = codegenMap.at(operatorNameFrom(tok.operatorIndex));
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
  
  void visitReturn(ReturnNode* node) {
    builder.CreateRet(compileExpression(node->getValue()));
  }
  
};

#endif
