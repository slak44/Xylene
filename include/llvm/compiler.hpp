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
#include <stack>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "ast.hpp"
#include "operator.hpp"

/**
  \brief Each CompileVisitor creates a llvm::Module from an AST.
*/
class CompileVisitor: public ASTVisitor, public std::enable_shared_from_this<CompileVisitor> {
public:
  using Link = PtrUtil<CompileVisitor>::Link;
private:
  /// LLVM Context for this visitor
  llvm::LLVMContext* context;

  static const uint bitsPerInt = 64;
  /// Integral type
  llvm::IntegerType* integerType;
  /// Floating-point type
  llvm::Type* floatType;
  /// Boolean type
  llvm::IntegerType* booleanType;

  std::unique_ptr<llvm::IRBuilder<>> builder; ///< Used to construct llvm instructions
  llvm::Module* module; ///< The module that is being created
  llvm::Function* entryPoint; ///< Entry point for module
  std::stack<llvm::Function*> functionStack; ///< Current function stack
  AST ast; ///< Source AST
  
  /// Used for throwing consistent type mismatch errors
  static const std::string typeMismatchErrorString;
  
  /// Used to generate IR for operators
  class OperatorCodegen {
  public:
    /// Generates IR using provided arguments
    using CodegenFunction = std::function<llvm::Value*(std::vector<llvm::Value*>, uint)>;
    /// Signature for a lambda representing a CodegenFunction
    #define CODEGEN_SIG (std::vector<llvm::Value*> operands, __attribute__((unused)) uint line) -> llvm::Value*
    /// Maps operand types to a func that generates code from them
    using TypeMap = std::unordered_map<std::vector<TokenType>, CodegenFunction, VectorHash<TokenType>>;
  private:
    CompileVisitor::Link cv;
    
    std::unordered_map<Operator::Name, TypeMap> codegenMap;
    std::unordered_map<Operator::Name, CodegenFunction> specialCodegenMap;
    
    inline llvm::Value* SItoFP(llvm::Value* integer) {
      return cv->builder->CreateSIToFP(integer, cv->floatType, "SItoFPconv");
    }
  public:
    OperatorCodegen(CompileVisitor::Link cv);
    
    /// Search for a CodegenFunction everywhere
    CodegenFunction findAndGetFun(Token tok, std::vector<llvm::Value*>& operands);
    /// Search for a CodegenFunction in the specialCodegenMap
    CodegenFunction getSpecialFun(Token tok) noexcept;
    /// Search for a CodegenFunction in the codegenMap
    CodegenFunction getNormalFun(Token tok, const std::vector<TokenType>& types);
  };
  
  /// Instance of OperatorCodegen
  std::unique_ptr<OperatorCodegen> codegen;

  /// Use the static factory \link create \endlink
  CompileVisitor(std::string moduleName, AST ast);
public:
  /**
    \brief Create a CompileVisitor.
    
    This is a static factory because std::enable_shared_from_this<CompileVisitor>
    requires a shared_ptr to already exist before being used.
    This method guarantees that at least one such pointer exists.
    It also handles creation of the OperatorCodegen, since that also requires a shared_ptr of this.
  */
  static Link create(std::string moduleName, AST ast) {
    auto thisThing = std::make_shared<CompileVisitor>(CompileVisitor(moduleName, ast));
    thisThing->codegen = std::make_unique<OperatorCodegen>(OperatorCodegen(thisThing));
    return thisThing;
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
  void visitFunction(Node<FunctionNode>::Link node);
  
  /// Implementation detail
  TokenType getFromValueType(llvm::Type* ty);
  /// Implementation detail
  llvm::Value* compileExpression(Node<ExpressionNode>::Link node, bool requirePointer = false);
  /// Implementation detail
  void compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding = nullptr);
  /// Implementation detail
  llvm::BasicBlock* compileBlock(Node<BlockNode>::Link node, const std::string& name);
};

#endif
