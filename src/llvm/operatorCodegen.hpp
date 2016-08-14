#ifndef OPERATOR_CODEGEN_HPP
#define OPERATOR_CODEGEN_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

#include "globalTypes.hpp"
#include "utils/util.hpp"
#include "operator.hpp"
#include "tokenType.hpp"

using CodegenFunction = std::function<llvm::Value*(llvm::IRBuilder<>, std::vector<llvm::Value*>)>;
#define CODEGEN_SIG (llvm::IRBuilder<> builder, std::vector<llvm::Value*> operands) -> llvm::Value*
// Maps operand types to a func that generates code from them
using TypeMap = std::unordered_map<std::vector<TokenType>, CodegenFunction, VectorHash<TokenType>>;

static inline llvm::Value* SItoFP(llvm::IRBuilder<> builder, llvm::Value* integer) {
  return builder.CreateSIToFP(integer, floatType, "SItoFPconv");
}

std::unordered_map<OperatorName, TypeMap> codegenMap {
  {"Add", {
    {{L_INTEGER, L_INTEGER}, [] CODEGEN_SIG {
      return builder.CreateAdd(operands[0], operands[1], "intadd");
    }},
    {{L_INTEGER, L_FLOAT}, [] CODEGEN_SIG {
      return builder.CreateFAdd(SItoFP(builder, operands[0]), operands[1], "intfltadd");
    }},
    {{L_FLOAT, L_INTEGER}, [] CODEGEN_SIG {
      return builder.CreateFAdd(operands[0], SItoFP(builder, operands[1]), "fltintadd");
    }},
    {{L_FLOAT, L_FLOAT}, [] CODEGEN_SIG {
      return builder.CreateFAdd(operands[0], operands[1], "fltadd");
    }},
  }}
};

#endif
