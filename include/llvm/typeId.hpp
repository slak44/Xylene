#ifndef TYPEID_HPP
#define TYPEID_HPP

#include <memory>
#include <unordered_set>

#include "utils/util.hpp"
#include "utils/error.hpp"

namespace llvm {
  class Type;
  class Value;
  class StructType;
}
class TypeData; 

/// Type compatibility states
enum TypeCompat {
  COMPATIBLE = 1, ///< Can assign one to another
  INCOMPATIBLE = 0, ///< Can't assign one to another
  DYNAMIC = 1 << 5, ///< Cannot statically determine compatibility
};

/**
  \brief An abstract identifier with a name and a unique id.
*/
class AbstractId {
public:
  using Link = std::shared_ptr<AbstractId>;
private:
  /**
    The values returned by this function MUST be unique (until max int value is hit).
    Otherwise, there are no requirements on the value of the number.
  */
  static inline UniqueIdentifier generateId() {
    static UniqueIdentifier lastId = 100;
    lastId += 1;
    return lastId;
  }
protected:
  /// Name of the this type
  TypeName name;
  /// Constant numeric unique id
  const UniqueIdentifier id = generateId();
  
  AbstractId() = default;
  AbstractId(const AbstractId&) = default;
  virtual ~AbstractId() {}
public:
  virtual UniqueIdentifier getId() const;
  virtual TypeName getName() const;
  
  /// \returns how many types are stored by this identifier
  virtual std::size_t storedTypeCount() const = 0;
  /// \returns names of the types stored by this identifier
  virtual TypeList storedNames() const = 0;
  /// \returns what should llvm allocate for this id
  virtual llvm::Type* getAllocaType() const = 0;
  /// \returns if the parameter can be assigned to this id
  virtual TypeCompat isCompat(AbstractId::Link) const = 0;
  
  inline std::string typeNames() const {
    return collate(storedNames());
  }
  
  inline bool operator==(const AbstractId& rhs) const {
    return this->getId() == rhs.getId();
  }
  inline bool operator!=(const AbstractId& rhs) const {
    return !operator==(rhs);
  }
  
  inline bool operator==(const UniqueIdentifier& rhs) const {
    return this->getId() == rhs;
  }
  inline bool operator!=(const UniqueIdentifier& rhs) const {
    return !operator==(rhs);
  }
};

namespace std {
  /// AbstractId std::hash template specialization
  template<>
  struct hash<AbstractId> {
    using argument_type = AbstractId;
    using result_type = size_t;
    size_t operator()(const AbstractId& tid) const {
      // These ids are unique, so they should work well as hashes
      return tid.getId();
    }
  };
  template<>
  struct hash<vector<AbstractId::Link>> {
    using argument_type = vector<AbstractId::Link>;
    using result_type = size_t;
    size_t operator()(const vector<AbstractId::Link>& vec) const {
      size_t seed = vec.size();
      for (auto e : vec) {
        seed ^= e->getId() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };
}

/**
  \brief Identifies a single type.
*/
class TypeId: public AbstractId {
public:
  using Link = std::shared_ptr<TypeId>;
private:
  /**
    For basic types (ints floats), basicTy is used.
    For user types, tyData is used.
  */
  llvm::Type* basicTy = nullptr;
  TypeData* tyData = nullptr;
protected:
  TypeId(TypeData* tyData);
  TypeId(TypeName, llvm::Type*);
public:
  /// Static factory for a user defined type
  static Link create(TypeData* tyData);
  /// Static factory for basic types
  static Link createBasic(TypeName, llvm::Type*);
  
  /// \returns associated TypeData* or nullptr if this type is basic
  TypeData* getTyData() const;
  
  virtual inline std::size_t storedTypeCount() const override {
    return 1;
  }
  TypeList storedNames() const override;
  llvm::Type* getAllocaType() const override;
  TypeCompat isCompat(AbstractId::Link) const override;
};

/**
  \brief Identifies a list of types.
*/
class TypeListId: public AbstractId {
public:
  using Link = std::shared_ptr<TypeListId>;
private:
  llvm::StructType* taggedUnionType;
  std::unordered_set<AbstractId::Link> types;
protected:
  TypeListId(TypeName, std::unordered_set<AbstractId::Link>, llvm::StructType*);
public:
  /// Create a new type list with a name and some types in it
  static Link create(TypeName, std::unordered_set<AbstractId::Link>, llvm::StructType*);
  
  /// \returns list of types in this list
  std::unordered_set<AbstractId::Link> getTypes() const;
  
  virtual inline std::size_t storedTypeCount() const override {
    return types.size();
  }
  TypeList storedNames() const override;
  llvm::Type* getAllocaType() const override;
  TypeCompat isCompat(AbstractId::Link) const override;
};

// TODO class AliasId: public AbstractId

#endif
