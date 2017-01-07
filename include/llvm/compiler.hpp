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
#include "llvm/IR/LegacyPassManager.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <experimental/filesystem>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "ast.hpp"
#include "operator.hpp"
#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "llvm/typeId.hpp"

namespace fs = std::experimental::filesystem;

class OperatorCodegen;

/**
  \brief Holds a llvm::Value* and the type it currently carries
*/
class ValueWrapper {
friend class ModuleCompiler;
public:
  using Link = std::shared_ptr<ValueWrapper>;
protected:
  llvm::Value* llvmValue;
  AbstractId::Link currentType;
public:
  ValueWrapper(llvm::Value* value, AbstractId::Link tid);
  ValueWrapper(std::pair<llvm::Value*, AbstractId::Link> pair);
  
  inline virtual ~ValueWrapper() {}
  
  /// If the value is nullptr
  bool isInitialized() const;
  /// If the llvm::Type of the value is a pointer type
  bool hasPointerValue() const;
  
  llvm::Value* getValue() const;
  AbstractId::Link getCurrentType() const;
  void setValue(llvm::Value* newVal, AbstractId::Link newType);
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
  FunctionWrapper(llvm::Function* func, FunctionSignature sig, TypeId::Link funTy);
  
  FunctionSignature getSignature() const;
  
  llvm::Function* getValue() const;
};

/**
  \brief Holds the value pointing to the declared variable, its current type, and its
  list of allowed types
*/
class DeclarationWrapper: public ValueWrapper {
public:
  using Link = std::shared_ptr<DeclarationWrapper>;
private:
  TypeListId::Link tlid = nullptr;
public:
  /**
    \brief Create a DeclarationWrapper
    \param decl an AllocaInst
    \param allowed either a TypeListId or a TypeId of what is allowed to be assigned
    to this
  */
  DeclarationWrapper(llvm::Value* decl, AbstractId::Link allowed);
  
  TypeListId::Link getTypeList() const;
};

/**
  \brief Stores an instance of a user type.
*/
class InstanceWrapper: public ValueWrapper {
public:
  using Link = std::shared_ptr<InstanceWrapper>;
private:
  /// Maps member names to their declaration
  std::map<std::string, DeclarationWrapper::Link> members {};
public:
  InstanceWrapper(llvm::Value*, TypeId::Link current);
  
  /// Get a pointer to the member with the specified name
  DeclarationWrapper::Link getMember(std::string name);
};

class ProgramData {
public:
  using TypeSet = std::unordered_set<AbstractId::Link>;
  std::unordered_set<AbstractId::Link> types;
  std::vector<llvm::Module*> modules;
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

  static const uint bitsPerInt = 64;

  llvm::IntegerType* integerType;
  llvm::Type* floatType;
  llvm::IntegerType* booleanType;
  llvm::PointerType* functionType;
  llvm::PointerType* voidPtrType;
  llvm::StructType* taggedUnionType;
  
  TypeId::Link voidTid;
  TypeId::Link integerTid;
  TypeId::Link floatTid;
  TypeId::Link booleanTid;
  TypeId::Link functionTid;

  /// Pointer to global type set
  std::unique_ptr<ProgramData::TypeSet> types;

  std::unique_ptr<llvm::IRBuilder<>> builder; ///< Used to construct llvm instructions
  llvm::Module* module; ///< The module that is being created
  FunctionWrapper::Link entryPoint; ///< Entry point for module
  std::stack<FunctionWrapper::Link> functionStack; ///< Current function stack. Not a call stack
  std::unique_ptr<AST> ast; ///< Source AST
  
  /// Used for throwing consistent type mismatch errors
  static const std::string typeMismatchErrorString;
  
  /// Instance of OperatorCodegen
  std::unique_ptr<OperatorCodegen> codegen;

  // TODO: eventually this and its create() should be dropped
  /// Use the static factory \link create \endlink
  ModuleCompiler(std::string moduleName, AST& ast);
  
  /// This constructor expects the caller to initialize members manually
  ModuleCompiler();
  
  /// Impl detail
  void init(std::string moduleName, AST& ast);
public:
  // TODO: should be removed along with its constructor
  /**
    \brief Create a ModuleCompiler.
    
    This is a static factory because std::enable_shared_from_this<ModuleCompiler>
    requires a shared_ptr to already exist before being used.
    This method guarantees that at least one such pointer exists.
    It also handles creation of the OperatorCodegen, since that also requires a
    shared_ptr of this. Calls addMainFunction.
  */
  static Link create(std::string moduleName, AST);
  /**
    Create a ModuleCompiler.
    
    This is a static factory because std::enable_shared_from_this<ModuleCompiler>
    requires a shared_ptr to already exist before being used.
    This method guarantees that at least one such pointer exists.
    It also handles creation of the OperatorCodegen, since that also requires a
    shared_ptr of this. 
    \param t reference to a globally shared set of types
  */
  static Link create(ProgramData::TypeSet& t, std::string moduleName, AST);
  
  /// Add a main function and set it as the entry point for this module
  void addMainFunction();
  
  /// Compile the AST. Call this before trying to retrieve the module.
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
  
  /// If the held value is a Boolean or can be converted to one
  bool canBeBoolean(ValueWrapper::Link) const;
  /**
    \brief Inserts a call to the runtime 'checkTypeCompat' function
    \returns a value with boolean type, whether or not the new type is compatible
  */
  ValueWrapper::Link insertRuntimeTypeCheck(DeclarationWrapper::Link, ValueWrapper::Link);
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

/**
  \brief Stores information about a field in a type
*/
class MemberMetadata {
public:
  using Link = std::shared_ptr<MemberMetadata>;
private:
  Node<MemberNode>::Link mem;
  llvm::Type* toAllocate;
public:
  MemberMetadata(Node<MemberNode>::Link mem, llvm::Type* toAllocate);
  
  DefiniteTypeInfo getTypeInfo() const;
  llvm::Type* getAllocaType() const;
  std::string getName() const;
  bool hasInit() const;
  Node<ExpressionNode>::Link getInit() const;
  Node<MemberNode>::Link getNode() const;
  Trace getTrace() const;
};

/**
  \brief Stores a method in a type
*/
class MethodData {
public:
  using Link = std::shared_ptr<MethodData>;
private:
  Node<MethodNode>::Link meth;
  std::string name;
  FunctionWrapper::Link fun;
public:
  MethodData(Node<MethodNode>::Link meth, std::string name, FunctionWrapper::Link fun);
  
  bool isForeign() const;
  Node<BlockNode>::Link getCodeBlock() const;
  FunctionWrapper::Link getFunction() const;
  std::string getName() const;
  Trace getTrace() const;
};

/**
  \brief Stores a constructor in a type
*/
class ConstructorData {
public:
  using Link = std::shared_ptr<ConstructorData>;
private:
  Node<ConstructorNode>::Link constr;
  FunctionWrapper::Link fun;
public:
  ValueWrapper::Link thisPtrRef;
  ConstructorData(Node<ConstructorNode>::Link constr, FunctionWrapper::Link fun);
  
  bool isForeign() const;
  Node<BlockNode>::Link getCodeBlock() const;
  FunctionWrapper::Link getFunction() const;
  Trace getTrace() const;
};

/**
  \brief Maintans state for a type.
*/
class TypeData {
friend class InstanceWrapper;
private:
  /**
    \brief Contains information about an initializer of a type
    
    Static initializers are don't take parameters.
    Normal initializers take a pointer to the object to be initialized,
    which has the type stored by the associated TypeData.
  */
  class TypeInitializer {
  private:
    /// Owner of this initializer
    TypeData& owner;
    /// Initializer type
    llvm::FunctionType* ty;
    /// Initializer function
    FunctionWrapper::Link init;
    /// BasicBlock in the initializer function
    llvm::BasicBlock* initBlock;
    /// If the initializer initializes anything
    bool initExists = false;
    /// Codegen functions to be called inside the initializer
    std::vector<std::function<void(TypeInitializer&)>> initsToAdd;
    /**
      \brief The this object passed to be initialized
      
      Only for normal initializers.
    */
    InstanceWrapper::Link initializerInstance = nullptr;
    
    /**
      \brief Gets the pointer passed to the initializerInstance
      
      Only for normal initializers.
    */
    ValueWrapper::Link getInitStructArg() const;
  public:
    enum Kind {STATIC, NORMAL};
    TypeInitializer(TypeData& ty, Kind k);
    
    /// Signals that all codegen functions have been added, and builds the initializer
    void finalize();
    /// Adds a new codegen function to be called inside the initializer
    void insertCode(std::function<void(TypeInitializer&)> what);
    
    /// The initializer
    FunctionWrapper::Link getInit() const;
    /**
      \brief The InstanceWrapper used
      
      Only for normal initializers.
    */
    InstanceWrapper::Link getInitInstance() const;
    /// \copydoc initExists
    bool exists() const;
  };
  
  /// Pointer to the owning ModuleCompiler
  ModuleCompiler::Link mc;
  /// Pointer to the TypeNode where this type was defined
  Node<TypeNode>::Link node;
  /// llvm::Type of the llvm struct for this type
  llvm::StructType* dataType;
  /// Metadata for normal members of this type
  std::vector<MemberMetadata::Link> members;
  /// Metadata for static members of this type
  std::vector<MemberMetadata::Link> staticMembers;
  /// List of methods
  std::vector<MethodData::Link> methods;
  /// List of static functions
  std::vector<MethodData::Link> staticFunctions;
  /// List of constructors
  std::vector<ConstructorData::Link> constructors;
  /// The static initializer for this type. Can be empty
  TypeInitializer staticTi;
  /// The normal initializer for this type. Can be empty
  TypeInitializer normalTi;
  /// If this object is ready to be instantiated or have its static members accessed
  bool finalized = false;

  /// Utility function for creating some identifiers
  inline std::string nameFrom(std::string prefix, std::string nameOfThing) {
    return prefix + "_" + node->getName() + "_" + nameOfThing;
  }
public:
  /**
    \brief Create a TypeData
  */
  TypeData(llvm::StructType* type, ModuleCompiler::Link mc, Node<TypeNode>::Link tyNode);
  inline ~TypeData() {}
  
  /// Disallow copy constructor
  TypeData(const TypeData&) = delete;
  /// Disallow copy assignment
  TypeData& operator=(const TypeData&) = delete;

  /// Get the llvm::StructType backing this type
  llvm::StructType* getStructTy() const;
  /// Get the name of this type
  TypeName getName() const;
  /// Get initializer
  TypeInitializer getInit() const;
  /// Get static initializer
  TypeInitializer getStaticInit() const;
  /// Gets a list of types so we know what to allocate for the struct
  std::vector<llvm::Type*> getAllocaTypes() const;
  /// Check if this name isn't already used for something else
  void validateName(std::string name) const;
  /// Add a new member to the type
  void addMember(MemberMetadata newMember, bool isStatic);
  /// Add a new method to the type
  void addMethod(MethodData func, bool isStatic);
  /// Add a new constructor to the type
  void addConstructor(ConstructorData c);
  /**
    \brief Signal that this type is completely built, and can be instantiated
    
    This does a couple things:
    - Adds codegen functions to initializers
    - Finalizes both initializers
    - Adds the bodies of non-static, non-foreign methods
    - Adds the bodies of non-foreign constructors
  */
  void finalize();
  /// \copydoc finalize
  bool isReady() const;
};

#endif
