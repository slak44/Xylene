#include "llvm/typeId.hpp"
#include "llvm/typeData.hpp"

UniqueIdentifier AbstractId::getId() const {
  return id;
}

TypeName AbstractId::getName() const {
  return name;
}

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

TypeData* TypeId::getTyData() const {
  return tyData;
}

llvm::Type* TypeId::getAllocaType() const {
  if (basicTy) return basicTy;
  else return tyData->getStructTy();
}

TypeList TypeId::storedNames() const {
  return {name};
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

std::unordered_set<AbstractId::Link> TypeListId::getTypes() const {
  return types;
}

TypeList TypeListId::storedNames() const {
  TypeList names;
  std::for_each(ALL(types), [&](AbstractId::Link id) {
    names.insert(id->getName());
  });
  return names;
}

llvm::Type* TypeListId::getAllocaType() const {
  return taggedUnionType;
}
