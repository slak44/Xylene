#ifndef TYPE_DATA_HPP
#define TYPE_DATA_HPP

#include "ast.hpp"
#include "llvm/values.hpp"

namespace llvm {
  class FunctionType;
}

class ModuleCompiler;

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
    
    ValueWrapper::Link thisObject = nullptr;
    /**
      \brief Gets the pointer passed to the initializerInstance
      
      Only for normal initializers.
    */
    ValueWrapper::Link getInitStructArg();
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
  std::shared_ptr<ModuleCompiler> mc;
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
    return fmt::format("{0}_{1}_{2}", prefix, node->getName(), nameOfThing);
  }
public:
  /**
    \brief Create a TypeData
  */
  TypeData(llvm::StructType* type, std::shared_ptr<ModuleCompiler>, Node<TypeNode>::Link tyNode);
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
