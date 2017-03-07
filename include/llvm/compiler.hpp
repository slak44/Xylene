#ifndef LLVM_COMPILER_HPP
#define LLVM_COMPILER_HPP

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/BasicBlock.h>
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
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <string>
#include <vector>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <fstream>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "ast.hpp"
#include "operator.hpp"
#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "llvm/typeId.hpp"
#include "llvm/values.hpp"
#include "runtime/runtime.hpp"

class OperatorCodegen;

class ProgramData {
public:
  using TypeSet = std::unordered_set<AbstractId::Link>;
  TypeSet types;
  std::vector<std::unique_ptr<llvm::Module>> modules;
  std::unique_ptr<llvm::Module> rootModule;
};

/**
  \brief Each ModuleCompiler creates a llvm::Module from an AST.
*/
class ModuleCompiler: public ASTVisitor, public std::enable_shared_from_this<ModuleCompiler> {
friend class TypeData;
friend class TypeInitializer;
friend class InstanceWrapper;
friend class OperatorCodegen;
public:
  using Link = PtrUtil<ModuleCompiler>::Link;
private:
  /// LLVM Context for this visitor
  llvm::LLVMContext* context;

  static const unsigned bitsPerInt = 64;

  llvm::IntegerType* integerType;
  llvm::Type* floatType;
  llvm::Type* voidType;
  llvm::IntegerType* booleanType;
  llvm::PointerType* functionType;
  llvm::PointerType* voidPtrType;
  llvm::StructType* taggedUnionType;
  llvm::PointerType* taggedUnionPtrType;
  
  TypeId::Link voidTid;
  TypeId::Link integerTid;
  TypeId::Link floatTid;
  TypeId::Link booleanTid;
  TypeId::Link functionTid;

  /// Pointer to global type set
  std::shared_ptr<ProgramData::TypeSet> types;

  std::unique_ptr<llvm::IRBuilder<>> builder; ///< Used to construct llvm instructions
  llvm::Module* module; ///< The module that is being created
  FunctionWrapper::Link entryPoint; ///< Entry point for module
  std::stack<FunctionWrapper::Link> functionStack; ///< Current function stack. Not a call stack
  AST ast; ///< Source AST
  
  /// Instance of OperatorCodegen
  std::unique_ptr<OperatorCodegen> codegen;
  
  ModuleCompiler(std::string moduleName, AST ast);
public:
  /**
    Create a ModuleCompiler.
    
    This is a static factory because std::enable_shared_from_this<ModuleCompiler>
    requires a shared_ptr to already exist before being used. This method guarantees
    that at least one such pointer exists.
    It also handles creation of the OperatorCodegen, since that also requires a
    shared_ptr of this. 
    \param t reference to a globally shared set of types
  */
  static Link create(
    const ProgramData::TypeSet& t, std::string moduleName, AST ast);
  
  /// Make this the root module, and add a main function
  void addMainFunction();
  
  /// Compile the AST. Call this before trying to retrieve the module.
  void compile();
  
  inline llvm::Module* getModule() const noexcept {
    return module;
  }

  inline llvm::Function* getEntryPoint() const noexcept {
    return entryPoint->getValue();
  }
  
  inline std::shared_ptr<ProgramData::TypeSet> getTypeSetPtr() const noexcept {
    return types;
  }
  
private:
  void visitExpression(Node<ExpressionNode>::Link node) override;
  void visitDeclaration(Node<DeclarationNode>::Link node) override;
  void visitBranch(Node<BranchNode>::Link node) override;
  void visitLoop(Node<LoopNode>::Link node) override;
  void visitReturn(Node<ReturnNode>::Link node) override;
  void visitBlock(Node<BlockNode>::Link node) override;
  void visitBreakLoop(Node<BreakLoopNode>::Link node) override;
  void visitFunction(Node<FunctionNode>::Link node) override;
  void visitType(Node<TypeNode>::Link node) override;
  void visitConstructor(Node<ConstructorNode>::Link node) override;
  void visitMethod(Node<MethodNode>::Link node) override;
  void visitMember(Node<MemberNode>::Link node) override;
  
  /// Inserts declarations for some required runtime functions
  void insertRuntimeFuncDecls();
  
  /// If the held value is a Boolean or can be converted to one
  bool canBeBoolean(ValueWrapper::Link) const;
  /// Convert simple value to tagged union with 1 type
  ValueWrapper::Link boxPrimitive(ValueWrapper::Link p);
  /// Checks if assignment is allowed. Inserts IR
  void typeCheck(AbstractId::Link, ValueWrapper::Link, Error);
  /// Inserts a call to the runtime '_xyl_typeErrIfIncompatible' function
  void insertRuntimeTypeCheck(ValueWrapper::Link target, ValueWrapper::Link);
  /// Inserts a call to the runtime '_xyl_typeErrIfIncompatibleTid' function
  void insertRuntimeTypeCheck(AbstractId::Link, ValueWrapper::Link);
  /// Inserts a call to malloc and returns a pointer with the ValueWrapper's type
  llvm::Value* insertDynAlloc(uint64_t, ValueWrapper::Link);
  // Store in the allowed type list field of a union
  llvm::Value* storeTypeList(llvm::Value* taggedUnion, UniqueIdentifier typeList);
  // Update all required fields in a union when assigning a new value
  void assignToUnion(ValueWrapper::Link to, ValueWrapper::Link from);
  /**
    \brief Creates a pointer pointing to a specific function's argument
    
    Be careful where and when this gets called, since it inserts IR,
    and it assumes it is already in the right place.
  */
  ValueWrapper::Link getPtrForArgument(
    TypeId::Link argType,
    FunctionWrapper::Link fun,
    std::size_t which
  );
  /// Gets the llvm:Type* to be allocated for the given type info
  llvm::Type* typeFromInfo(TypeInfo ti, ASTNode::Link node);
  /// Gets an id for the given type info
  AbstractId::Link typeIdFromInfo(TypeInfo ti, ASTNode::Link node);
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

/**
  \brief Compiles an entire program.
  
  LLVM modules should be nothing more than implementation details to this, but the
  linker might care about them.
*/
class Compiler final {
friend class ModuleCompiler;
  // TODO: this things is gonna build a dependency graph
  // then look for all the referenced files
  // figure out if we can get some parallel compilation going on
  // it'll make sure every import/export is handled correctly
  // then hand over the compiled modules to the linker
  // send over the precompiled runtime as well
  // and pull out an executable
  
  // TODO: figure out how the interpreter's gonna work
  // might need to have some shared code with this one
  
  // TODO: handle cyclic dependencies in that graph
  
  // TODO: add more compile options, opt level, opt passes, linker, etc.
private:
  fs::path rootScript;
  fs::path output;
  
  ProgramData pd;
public:
  Compiler(fs::path rootScript, fs::path output);
  /// Same as \link Compiler(fs::path,fs::path) \endlink, but with a precompiled module
  Compiler(std::unique_ptr<llvm::Module>, fs::path rootScript, fs::path output);
  
  // This might be expensive to copy, so don't
  Compiler(const Compiler&) = delete; /// No copy-constructor
  Compiler& operator=(const Compiler&) = delete; /// No copy-assignment
  
  void compile();
  fs::path getOutputPath() const;
};

/// Used to generate IR for operators
class OperatorCodegen {
friend class ModuleCompiler;
public:
  using ValueList = std::vector<ValueWrapper::Link>;
  /// Signature for a lambda representing a CodegenFunction
  #define CODEGEN_SIG (\
    __attribute__((unused)) ValueList operands,\
    __attribute__((unused)) ValueList rawOperands,\
    __attribute__((unused)) Trace trace\
  ) -> ValueWrapper::Link
  /// Generates IR using provided arguments
  using CodegenFunction = std::function<ValueWrapper::Link(ValueList, ValueList, Trace)>;
  /// Signature for a lambda representing a SpecialCodegenFunction
  #define SPECIAL_CODEGEN_SIG (\
    ValueList operands,\
    __attribute__((unused)) ASTNode::Link node,\
    __attribute__((unused)) Trace trace\
  ) -> ValueWrapper::Link
  /// Generates IR using provided arguments
  using SpecialCodegenFunction = std::function<ValueWrapper::Link(ValueList, ASTNode::Link, Trace)>;
  /// Maps operand types to a func that generates code from them
  using TypeMap = std::unordered_map<std::vector<AbstractId::Link>, CodegenFunction>;
private:
  ModuleCompiler::Link mc;
  
  /// All operands are values; pointers are loaded
  std::unordered_map<Operator::Name, TypeMap> codegenMap;
  /// Operands are not processed
  std::unordered_map<Operator::Name, SpecialCodegenFunction> specialCodegenMap;
  
  /// Only ModuleCompiler can construct this
  OperatorCodegen(ModuleCompiler::Link mc);
public:
  /// Search for a CodegenFunction everywhere, and run it
  ValueWrapper::Link findAndRunFun(Node<ExpressionNode>::Link node, ValueList operands);
  /// Search for a CodegenFunction in the specialCodegenMap
  SpecialCodegenFunction getSpecialFun(Node<ExpressionNode>::Link node);
  /// Search for a CodegenFunction in the codegenMap
  CodegenFunction getNormalFun(Node<ExpressionNode>::Link node, std::vector<AbstractId::Link> types);
};

#endif
