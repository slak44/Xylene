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

/// \todo implement everything mentioned by this
static const InternalError ni = InternalError("Not Implemented", {METADATA_PAIRS});

/**
  \brief Each CompileVisitor creates a llvm::Module from an AST.
*/
class CompileVisitor: public ASTVisitor, public std::enable_shared_from_this<CompileVisitor> {
private:
  llvm::IRBuilder<> builder; ///< Used to construct llvm instructions
  llvm::Module* module; ///< The module the is being created
  llvm::Function* entryPoint; ///< Entry point for module
  llvm::Function* currentFunction; ///< Current function to insert into
  AST ast; ///< Source AST

  /// Use the static factory \link create \endlink
  CompileVisitor(std::string moduleName, AST ast);
public:
  using Link = PtrUtil<CompileVisitor>::Link;
  
  /**
    \brief Create a CompileVisitor.
    
    This is a static factory because std::enable_shared_from_this<CompileVisitor>
    requires a shared ptr to already exist before being used.
    This method guarantees that at least one such pointer exists.
  */
  static Link create(std::string moduleName, AST ast) {
    return std::make_shared<CompileVisitor>(CompileVisitor(moduleName, ast));
  }
  
  /**
    \brief Compile the AST. Call this before trying to retrieve the module.
  */
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
  void visitBreakLoop(Node<BreakLoopNode>::Link node);
  
  /// Implementation detail
  llvm::Value* compileExpression(Node<ExpressionNode>::Link node, bool requirePointer = false);
  /// Implementation detail
  void compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding = nullptr);
  /// Implementation detail
  llvm::BasicBlock* compileBlock(Node<BlockNode>::Link node, const std::string& name);
};

#endif
