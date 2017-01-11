#include "utils/typeInfo.hpp"

TypeInfo::TypeInfo(TypeName type): evalValue(TypeList({type})) {}
TypeInfo::TypeInfo(const char* type): TypeInfo(std::string(type)) {}
TypeInfo::TypeInfo(TypeList evalValue): evalValue(evalValue) {}
TypeInfo::TypeInfo(std::nullptr_t voidType): isVoidType(true) {UNUSED(voidType);}
TypeInfo::TypeInfo(): evalValue({}) {}

TypeList TypeInfo::getEvalTypeList() const {
  throwIfVoid();
  return evalValue;
}

bool TypeInfo::isDynamic() const {
  throwIfVoid();
  return evalValue.size() == 0;
}

bool TypeInfo::isVoid() const {
  return isVoidType;
}

std::string TypeInfo::getTypeNameString() const {
  if (isVoidType) return "[void]";
  return isDynamic() ? "[dynamic]" : collate(evalValue);
}

std::string TypeInfo::toString() const {
  return "TypeInfo: " + getTypeNameString();
}

bool TypeInfo::operator==(const TypeInfo& rhs) const {
  if (isVoidType != rhs.isVoidType) return false;
  return evalValue == rhs.evalValue;
}

bool TypeInfo::operator!=(const TypeInfo& rhs) const {
  return !operator==(rhs);
}

StaticTypeInfo::StaticTypeInfo(std::string type): DefiniteTypeInfo({type}) {}
StaticTypeInfo::StaticTypeInfo(const char* type): StaticTypeInfo(std::string(type)) {}

std::string StaticTypeInfo::toString() const {
  return "StaticTypeInfo: " + *std::begin(evalValue);
}

DefiniteTypeInfo::DefiniteTypeInfo(TypeList evalValue): TypeInfo(evalValue) {}
DefiniteTypeInfo::DefiniteTypeInfo(): TypeInfo(TypeList {}) {}

std::string DefiniteTypeInfo::toString() const {
  return "DefiniteTypeInfo: " + getTypeNameString();
}

FunctionSignature::FunctionSignature(TypeInfo returnType, Arguments arguments): returnType(returnType), arguments(arguments) {}

TypeInfo FunctionSignature::getReturnType() const {
  return returnType;
}

FunctionSignature::Arguments FunctionSignature::getArguments() const {
  return arguments;
}

std::string FunctionSignature::toString() const {
  std::string str = "FunctionSignature (";
  str += "return: " + returnType.getTypeNameString();
  str += ", arguments: " + collate(arguments, [](Arguments::value_type arg) {
    return "(arg " + arg.first + ": " + arg.second.getTypeNameString() + ")";
  });
  str += ")";
  return str;
}

bool FunctionSignature::operator==(const FunctionSignature& rhs) const {
  return returnType == rhs.returnType && arguments == rhs.arguments;
}
bool FunctionSignature::operator!=(const FunctionSignature& rhs) const {
  return !operator==(rhs);
}
