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
#include <functional>
#include <type_traits>
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
  llvm::Module* module = nullptr; ///< The module that is being created
  FunctionWrapper::Link entryPoint; ///< Entry point for module
  std::stack<FunctionWrapper::Link> functionStack; ///< Current function stack. Not a call stack
  AST ast; ///< Source AST
  
  
  using CodegenFun = std::function<ValueWrapper::Link(std::vector<ValueWrapper::Link>, Node<ExpressionNode>::Link)>;
  /// Map of codegen funcs for operators
  std::unordered_map<Operator::Name, CodegenFun> codegenMap {};
  
  bool isRoot;
  
  ModuleCompiler(std::string moduleName, AST, bool isRoot);
public:
  /**
    Create a ModuleCompiler.
    
    This is a static factory because std::enable_shared_from_this<ModuleCompiler>
    requires a shared_ptr to already exist before being used. This method guarantees
    that at least one such pointer exists.
    \param t reference to a globally shared set of types
    \param isRoot if this is the root module
  */
  static Link create(
    const ProgramData::TypeSet& t, std::string moduleName, AST ast, bool isRoot);
  
  /// Compile the AST. Call this before trying to retrieve the module.
  void compile();
  
  /// Store the global RTTI in this module
  void serializeTypeSet();
  
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
  
  /// Add a main function to this module
  void addMainFunction();
  
  /// Inserts declarations for some required runtime functions
  void insertRuntimeFuncDecls();
  
  /// If the held value is a Boolean or can be converted to one
  bool canBeBoolean(ValueWrapper::Link) const;
  // TODO: this should not be necessary
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
  
  // Codegen stuff
  
  /// Fill the codegen map with the CodegenFuncs
  void fillCodegenMap();
  
  /// Load it if it's a pointer, do nothing otherwise
  ValueWrapper::Link load(ValueWrapper::Link maybePointer);
  /// Check if the given operators' types are allowed for the given set
  bool operatorTypeCheck(std::vector<ValueWrapper::Link> checkThis, std::vector<std::vector<AbstractId::Link>> checkAgainst);
  
  /**
    \brief Creates a CodegenFun for arithmetic ops
    \param intFun member function of llvm::IRBuilder that does the op for integers
    \param floatFun member function of llvm::IRBuilder that does the op for floats
    \param args arguments besides lhs/rhs to pass for the IntegerFunc, since they differ, unlike the FloatFuncs
    \tparam IntegerFunc type of intFun
    \tparam FloatFunc type of floatFun
    \tparam DefaultArgs type of parameter pack
  */
  template<typename IntegerFunc, typename FloatFunc, typename... DefaultArgs>
  CodegenFun arithmOpBuilder(IntegerFunc intFun, FloatFunc floatFun, DefaultArgs... args) {
    auto boundIntFunc = objBind(intFun, builder.get());
    auto boundFloatFunc = objBind(floatFun, builder.get());
    return [=](std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node) {
      auto allowed = operatorTypeCheck(ops, {
        {integerTid, integerTid},
        {floatTid, floatTid},
        {integerTid, floatTid},
        {floatTid, integerTid},
        {floatTid, floatTid}
      });
      if (!allowed) throw "No operation available for given types"_type + node->getTrace();
      auto load0 = load(ops[0])->val;
      auto load1 = load(ops[1])->val;
      // If both are ints, do integer operations
      bool doInteger = ops[0]->ty == ops[1]->ty && ops[0]->ty == integerTid;
      llvm::Value* calc = doInteger ?
        boundIntFunc(load0, load1, args...) :
        boundFloatFunc(load0, load1, "", nullptr);
      return std::make_shared<ValueWrapper>(
        calc,
        doInteger ? integerTid : floatTid
      );
    };
  }
  
  /**
    \brief Creates a CodegenFun that increments/decrements postfixly/prefixly
    \param f prefix/postfix
  */
  template<typename IncDecFunc>
  CodegenFun preOrPostfixOpBuilder(Fixity f, IncDecFunc fun) {
    if (f != POSTFIX && f != PREFIX)
      throw InternalError("Only accepts prefix and postfix", {METADATA_PAIRS});
    auto boundFun = objBind(fun, builder.get());
    return [=](std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node) {
      auto allowed = operatorTypeCheck(ops, {
        {integerTid},
        {floatTid}
      });
      if (!allowed) throw "No operation available for given types"_type + node->getTrace();
      auto initial = load(ops[0])->val;
      auto changed = boundFun(
        initial,
        ops[0]->ty == integerTid ?
          llvm::ConstantInt::getSigned(integerType, 1) :
          llvm::ConstantFP::get(floatType, 1.0),
        "",
        false,
        false
      );
      auto stored = builder->CreateStore(changed, ops[0]->val);
      return std::make_shared<ValueWrapper>(
        f == POSTFIX ? initial : stored,
        ops[0]->ty
      );
    };
  }
  
  /**
    \brief Creates a CodegenFun for the given predicates
    \param intPred used for int-int and bool-bool comparisons
    \param fltPred used for the other comparisons (all with at least one float)
  */
  CodegenFun cmpOpBuilder(llvm::CmpInst::Predicate intPred, llvm::CmpInst::Predicate fltPred);
  
  /// Bitwise and logical operators are identical, except the bitwise ones also work on integers
  enum Bitwiseity {
    BITWISE,
    LOGICAL
  };
  
  /// Creates a CodegenFun that applies the not operation
  CodegenFun notFunction(Bitwiseity b);
  
  using BitwiseBinFunSig = llvm::Value* (llvm::IRBuilder<>::*)(llvm::Value*, llvm::Value*, const llvm::Twine&);
  /// Creates a CodegenFun that applies the given binary bit operation
  CodegenFun bitwiseOpBuilder(Bitwiseity b, BitwiseBinFunSig opFun);
  
  /// Is a CodegenFun that applies the unary plus operation (which is a noop)
  ValueWrapper::Link unaryPlus(std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node);
  /// Is a CodegenFun that applies the unary minus operation
  ValueWrapper::Link unaryMinus(std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node);
  /// Is a CodegenFun that applies the left shift operation
  ValueWrapper::Link shiftLeft(std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node);
  /// Is a CodegenFun that applies the arithmentic right shift operation
  ValueWrapper::Link shiftRight(std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node);
  /// Is a CodegenFun that creates a call
  ValueWrapper::Link call(std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node);
  /// Is a CodegenFun that creates an assignment
  ValueWrapper::Link assignment(std::vector<ValueWrapper::Link> ops, Node<ExpressionNode>::Link node);
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

#endif
