#ifndef OPERATOR_CODEGEN_HPP
#define OPERATOR_CODEGEN_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Type.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

#include "globalTypes.hpp"
#include "utils/util.hpp"
#include "operator.hpp"
#include "tokenType.hpp"

// Generates IR using provided arguments
using CodegenFunction = std::function<llvm::Value*(llvm::IRBuilder<>, std::vector<llvm::Value*>)>;
// Signature for a lambda representing a CodegenFunction
#define CODEGEN_SIG (llvm::IRBuilder<> builder, std::vector<llvm::Value*> operands) -> llvm::Value*
// Maps operand types to a func that generates code from them
using TypeMap = std::unordered_map<std::vector<TokenType>, CodegenFunction, VectorHash<TokenType>>;

// Convenience method
static inline llvm::Value* SItoFP(llvm::IRBuilder<> builder, llvm::Value* integer) {
  return builder.CreateSIToFP(integer, floatType, "SItoFPconv");
}

static inline llvm::Value* createLoad(llvm::IRBuilder<> builder, llvm::Value* from) {
  return builder.CreateLoad(from, "loadIdentifier");
}

extern std::unordered_map<OperatorName, CodegenFunction> specialCodegenMap;
extern std::unordered_map<OperatorName, TypeMap> codegenMap;

#endif
