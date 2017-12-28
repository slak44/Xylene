#include "llvm/typeId.hpp"
#include "llvm/typeData.hpp"

TypeId::TypeId(TypeData* tyData): tyData(tyData) {
  name = tyData->getName();
}
TypeId::TypeId(TypeName name, llvm::Type* ty): basicTy(ty) {
  this->name = name;
}

std::shared_ptr<TypeId> TypeId::create(TypeData* tyData) {
  return std::make_shared<TypeId>(TypeId(tyData));
}

std::shared_ptr<TypeId> TypeId::createBasic(TypeName name, llvm::Type* ty) {
  return std::make_shared<TypeId>(TypeId(name, ty));
}

llvm::Type* TypeId::getAllocaType() const noexcept {
  if (basicTy) return basicTy;
  else return tyData->getStructTy();
}

TypeCompat TypeId::isCompat(AbstractId::Link rhs) const noexcept {
  if (rhs->storedTypeCount() == 1) {
    return *rhs == *this ? COMPATIBLE : INCOMPATIBLE;
  }
  auto typeList = PtrUtil<TypeListId>::staticPtrCast(rhs);
  // If the type list doesn't have lhs type in it, it's decidely incompatible
  bool possibleCompat = includes(typeList->getTypes(), [=](auto ty) {
    return *this == *ty;
  });
  return possibleCompat ? DYNAMIC : INCOMPATIBLE;
}

TypeListId::TypeListId(
  TypeName name,
  std::unordered_set<AbstractId::Link> types,
  llvm::StructType* taggedUnionType
): taggedUnionType(taggedUnionType), types(types) {
  this->name = name;
  if (types.size() <= 1) {
    throw InternalError(
      "Trying to make a list of 1 or less elements (use TypeId for 1 element)",
      {METADATA_PAIRS, {"size", std::to_string(types.size())}}
    );
  }
}

TypeListId::Link TypeListId::create(
  TypeName n,
  std::unordered_set<AbstractId::Link> v,
  llvm::StructType* t
) {
  return std::make_shared<TypeListId>(TypeListId(n, v, t));
}

TypeList TypeListId::storedNames() const noexcept {
  TypeList names;
  std::for_each(ALL(types), [&](AbstractId::Link id) {
    names.insert(id->getName());
  });
  return names;
}

llvm::Type* TypeListId::getAllocaType() const noexcept {
  return taggedUnionType;
}

TypeCompat TypeListId::isCompat(AbstractId::Link rhs) const noexcept {
  if (rhs->storedTypeCount() == 1) {
    return includes(types, rhs) ? COMPATIBLE : INCOMPATIBLE;
  }
  auto typeList = PtrUtil<TypeListId>::staticPtrCast(rhs);
  bool possibleCompat = false;
  // O(n * m) in theory, too small to care in practice
  for (auto lhsTy : types) {
    for (auto rhsTy : typeList->types) {
      if (rhsTy == lhsTy) {
        possibleCompat = true;
        goto breakLoop;
      }
    }
  }
  breakLoop:
  return possibleCompat ? DYNAMIC : INCOMPATIBLE;
}
