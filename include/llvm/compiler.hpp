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

using TypeName = std::string;

class OperatorCodegen;

/**
  \brief Holds a llvm::Value* and the type it currently carries
*/
class ValueWrapper {
friend class CompileVisitor;
public:
  using Link = std::shared_ptr<ValueWrapper>;
protected:
  llvm::Value* llvmValue;
  TypeName currentType;
public:
  ValueWrapper(llvm::Value* value, TypeName name);
  ValueWrapper(std::pair<llvm::Value*, TypeName> pair);
  
  /// If the value is nullptr
  bool isInitialized() const;
  /// If the llvm::Type of the value is a pointer type
  bool hasPointerValue() const;
  
  llvm::Value* getValue() const;
  TypeName getCurrentTypeName() const;
  void setValue(llvm::Value* newVal, TypeName newType);
  
  /// If the held value is a Boolean or can be converted to one
  bool canBeBooleanValue() const;
};

/**
  \brief Holds a llvm::Function* and its respective FunctionSignature
*/
class FunctionWrapper: public ValueWrapper {
public:
  using Link = std::shared_ptr<FunctionWrapper>;
private:
  FunctionSignature sig;
public:
  FunctionWrapper(llvm::Function* func, FunctionSignature sig);
  
  FunctionSignature getSignature() const;
  
  llvm::Function* getValue() const;
};

/**
  \brief Holds the value pointing to the declared variable, its current type, and its list of allowed types
*/
class DeclarationWrapper: public ValueWrapper {
public:
  using Link = std::shared_ptr<DeclarationWrapper>;
private:
  TypeList tl;
public:
  DeclarationWrapper(llvm::Value* decl, TypeName current, TypeList tl);
  
  TypeList getTypeList() const;
};

/**
  \brief Each CompileVisitor creates a llvm::Module from an AST.
*/
class CompileVisitor: public ASTVisitor, public std::enable_shared_from_this<CompileVisitor> {
friend class TypeData;
friend class OperatorCodegen;
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
  
  // TODO: this might not have to be global, since there will be collisions of types with the same name from different scopes
  /**
    \brief Maps type names to llvm::Type pointers so they can be allocated
    
    Type names must be obtained using scope resolution. This map only assigns those names their respective llvm type.
  */
  std::unordered_map<TypeName, llvm::Type*> typeMap;

  std::unique_ptr<llvm::IRBuilder<>> builder; ///< Used to construct llvm instructions
  llvm::Module* module; ///< The module that is being created
  llvm::Function* entryPoint; ///< Entry point for module
  std::stack<FunctionWrapper::Link> functionStack; ///< Current function stack
  AST ast; ///< Source AST
  
  /// Used for throwing consistent type mismatch errors
  static const std::string typeMismatchErrorString;
  
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
  static Link create(std::string moduleName, AST ast);
  
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
  void visitType(Node<TypeNode>::Link node);
  void visitConstructor(Node<ConstructorNode>::Link node);
  void visitMethod(Node<MethodNode>::Link node);
  void visitMember(Node<MemberNode>::Link node);
  
  /// Utility function for checking if a type is in a TypeList
  static inline bool isTypeAllowedIn(TypeList tl, TypeName type) {
    return std::find(ALL(tl), type) != tl.end();
  }
  
  /// Get the DeclarationWrapper for an ExpressionNode containing an identifier
  DeclarationWrapper::Link findDeclaration(Node<ExpressionNode>::Link node);
  /// Gets the llvm:Type* to be allocated for the given type info
  llvm::Type* typeFromInfo(TypeInfo ti);
  /// How to handle an identifier in compileExpression
  enum IdentifierHandling {
    AS_POINTER, ///< Return a pointer
    AS_VALUE ///< Use a load instruction, and return that value
  };
  /// Gets a ValueWrapper for an ExpressionNode containing an identifier
  ValueWrapper::Link valueFromIdentifier(Node<ExpressionNode>::Link identifier);
  /// Implementation detail of visitExpression
  ValueWrapper::Link compileExpression(Node<ExpressionNode>::Link node, IdentifierHandling how = AS_VALUE);
  /// Implementation detail of visitBranch
  void compileBranch(Node<BranchNode>::Link node, llvm::BasicBlock* surrounding = nullptr);
  /// Implementation detail of visitBlock
  llvm::BasicBlock* compileBlock(Node<BlockNode>::Link node, const std::string& name);
};

/// Used to generate IR for operators
class OperatorCodegen {
friend class CompileVisitor;
public:
  /// Generates IR using provided arguments
  using CodegenFunction = std::function<ValueWrapper::Link(std::vector<ValueWrapper::Link>&, std::vector<ValueWrapper::Link>&, Trace)>;
  /// Generates IR using provided arguments
  using SpecialCodegenFunction = std::function<ValueWrapper::Link(std::vector<ValueWrapper::Link>&, ASTNode::Link, Trace)>;
  /// Signature for a lambda representing a CodegenFunction
  #define CODEGEN_SIG (\
    __attribute__((unused)) std::vector<ValueWrapper::Link>& operands,\
    __attribute__((unused)) std::vector<ValueWrapper::Link>& rawOperands,\
    __attribute__((unused)) Trace trace\
  ) -> ValueWrapper::Link
  /// Signature for a lambda representing a SpecialCodegenFunction
  #define SPECIAL_CODEGEN_SIG (\
    std::vector<ValueWrapper::Link>& operands,\
    __attribute__((unused)) ASTNode::Link node,\
    __attribute__((unused)) Trace trace\
  ) -> ValueWrapper::Link
  /// Maps operand types to a func that generates code from them
  using TypeMap = std::unordered_map<std::vector<TypeName>, CodegenFunction, VectorHash<TypeName>>;
private:
  CompileVisitor::Link cv;
  
  /// All operands are values; pointers are loaded
  std::unordered_map<Operator::Name, TypeMap> codegenMap;
  /// Operands are not processed
  std::unordered_map<Operator::Name, SpecialCodegenFunction> specialCodegenMap;
  
  /// Only CompileVisitor can construct this
  OperatorCodegen(CompileVisitor::Link cv);
public:
  /// Search for a CodegenFunction everywhere, and run it
  ValueWrapper::Link findAndRunFun(Node<ExpressionNode>::Link node, std::vector<ValueWrapper::Link>& operands);
  /// Search for a CodegenFunction in the specialCodegenMap
  SpecialCodegenFunction getSpecialFun(Node<ExpressionNode>::Link node) noexcept;
  /// Search for a CodegenFunction in the codegenMap
  CodegenFunction getNormalFun(Node<ExpressionNode>::Link node, const std::vector<TypeName>& types);
};

/**
  \brief Stores information about a field in a type
*/
class MemberMetadata {
public:
  using Link = std::shared_ptr<MemberMetadata>;
private:
  DefiniteTypeInfo allowed;
  llvm::Type* toAllocate;
  std::string name;
  Trace where;
public:
  MemberMetadata(DefiniteTypeInfo allowed, llvm::Type* toAllocate, std::string name, Trace where);
  
  DefiniteTypeInfo getTypeInfo() const;
  llvm::Type* getAllocaType() const;
  std::string getName() const;
  Trace getTrace() const;
};

/**
  \brief Stores data about a type.
*/
class TypeData {
private:
  llvm::StructType* dataType;
  CompileVisitor::Link cv;
  Node<TypeNode>::Link node;
  std::vector<MemberMetadata::Link> members;
  
  inline std::string nameFrom(std::string prefix, std::string nameOfThing) {
    return prefix + "_" + node->getName() + "_" + nameOfThing;
  }
  
  // The static init signature does not change for different types
  llvm::FunctionType* staticInitializerTy;
  // The normal init, however, does change with the type
  llvm::FunctionType* initializerTy;

  FunctionWrapper::Link staticInitializer = nullptr;
  // TODO: Might be a good candidate for inlining
  FunctionWrapper::Link initializer = nullptr;
  
  llvm::BasicBlock* staticInitBlock;
  llvm::BasicBlock* initBlock;
  
  bool hasStaticInit = false;
  bool hasNormalInit = false;
public:
  /**
    \brief Create a TypeData
  */
  TypeData(llvm::StructType* type, CompileVisitor::Link cv, Node<TypeNode>::Link tyNode);
  
  /// Gets a list of members
  std::vector<MemberMetadata::Link> getStructMembers() const;
  /// Gets a list of types so we know what to allocate for the struct
  std::vector<llvm::Type*> getAllocaTypes() const;
  /// Add a new member to the type
  void addStructMember(MemberMetadata newMember);
  
  /// Get the llvm::StructType backing this type
  llvm::StructType* getStructTy() const;
  
  /// Set the attached CompileVisitor's builder to insert in the static initializer
  void builderToStaticInit();
  /// Set the attached CompileVisitor's builder to insert in the normal initializer
  void builderToInit();
  /// Get the first argument of the normal initializer, the pointer to the struct to manipulate
  llvm::Argument* getInitStructArg() const;
};

#endif
