#include "utils/typeInfo.hpp"

std::string TypeInfo::getTypeNameString() const noexcept {
  if (isVoidType) return "[void]";
  return isDynamic() ? "[dynamic]" : collate(evalValue);
}

std::string TypeInfo::toString() const noexcept {
  return "TypeInfo: " + getTypeNameString();
}

bool TypeInfo::operator==(const TypeInfo& rhs) const noexcept {
  if (isVoidType != rhs.isVoidType) return false;
  return evalValue == rhs.evalValue;
}

bool TypeInfo::operator!=(const TypeInfo& rhs) const noexcept {
  return !operator==(rhs);
}

StaticTypeInfo::StaticTypeInfo(std::string type): DefiniteTypeInfo({type}) {}
StaticTypeInfo::StaticTypeInfo(const char* type): StaticTypeInfo(std::string(type)) {}

std::string StaticTypeInfo::toString() const noexcept {
  return "StaticTypeInfo: " + *std::begin(evalValue);
}

DefiniteTypeInfo::DefiniteTypeInfo(TypeList evalValue): TypeInfo(evalValue) {}
DefiniteTypeInfo::DefiniteTypeInfo(): TypeInfo(TypeList {}) {}

std::string DefiniteTypeInfo::toString() const noexcept {
  return "DefiniteTypeInfo: " + getTypeNameString();
}

FunctionSignature::FunctionSignature(TypeInfo returnType, Arguments arguments): returnType(returnType), arguments(arguments) {}

std::string FunctionSignature::toString() const noexcept {
  auto args = collate(arguments, [](Arguments::value_type arg) {
    return fmt::format("({0}: {1})", arg.first, arg.second.getTypeNameString());
  });
  return fmt::format("FunctionSignature: return {0}, arguments {1}", returnType.getTypeNameString(), args);
}
