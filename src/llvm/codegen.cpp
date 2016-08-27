#include "include/llvm/compiler.hpp"

using CmpPred = llvm::CmpInst::Predicate;
using CV = CompileVisitor;

CV::OperatorCodegen::OperatorCodegen(CompileVisitor::Link cv):
  cv(cv),
  codegenMap({
    {"Add", {
      {{cv->integerType, cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateAdd(operands[0], operands[1], "intadd");
      }},
      {{cv->integerType, cv->floatType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFAdd(SItoFP(operands[0]), operands[1], "intfltadd");
      }},
      {{cv->floatType, cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFAdd(operands[0], SItoFP(operands[1]), "fltintadd");
      }},
      {{cv->floatType, cv->floatType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFAdd(operands[0], operands[1], "fltadd");
      }},
    }},
    {"Less", {
      {{cv->integerType, cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateICmp(CmpPred::ICMP_SLT, operands[0], operands[1], "intcmpless");
      }},
      {{cv->integerType, cv->floatType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFCmp(CmpPred::FCMP_OLT, SItoFP(operands[0]), operands[1], "intfltcmpless");
      }},
      {{cv->floatType, cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFCmp(CmpPred::FCMP_OLT, operands[0], SItoFP(operands[1]), "fltintcmpless");
      }},
      {{cv->floatType, cv->floatType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFCmp(CmpPred::FCMP_OLT, operands[0], operands[1], "fltcmpless");
      }},
    }}
  }),
  specialCodegenMap({
    {"Assignment", [=] CODEGEN_SIG {
      return cv->builder->CreateStore(operands[1], operands[0]);
    }},
    {"Postfix ++", [=] CODEGEN_SIG {
      auto initial = cv->builder->CreateLoad(operands[0], "postfixincload");
      if (initial->getType() != cv->integerType) throw Error("TypeError", cv->typeMismatchErrorString, trace); 
      auto plusOne = cv->builder->CreateAdd(initial, llvm::ConstantInt::getSigned(cv->integerType, 1), "intinc");
      cv->builder->CreateStore(plusOne, operands[0]);
      return initial;
    }}
  }) {}

CV::OperatorCodegen::CodegenFunction CV::OperatorCodegen::findAndGetFun(Token tok, std::vector<llvm::Value*>& operands) {
  // Function to return
  CodegenFunction func;
  // Check if it's a special case
  func = getSpecialFun(tok);
  if (func != nullptr) return func;
  // Otherwise look in the normal map
  // Get a list of types
  std::vector<llvm::Type*> operandTypes;
  operandTypes.resize(operands.size());
  // Map an operand to a llvm::Type* representing it
  std::size_t idx = -1;
  std::transform(ALL(operands), operandTypes.begin(), [=, &idx, &operands](llvm::Value* val) -> llvm::Type* {
    idx++;
    llvm::Type* opType = val->getType();
    if (llvm::dyn_cast_or_null<llvm::PointerType>(opType)) {
      auto load = cv->builder->CreateLoad(operands[idx], "loadIdentifier");
      operands[idx] = load;
      opType = operands[idx]->getType();
    }
    return opType;
  });
  return getNormalFun(tok, operandTypes);
}

CV::OperatorCodegen::CodegenFunction CV::OperatorCodegen::getSpecialFun(Token tok) noexcept {
  const Operator::Name& toFind = operatorNameFrom(tok.idx);
  auto it = specialCodegenMap.find(toFind);
  if (it != specialCodegenMap.end()) return it->second;
  else return nullptr;
}

CV::OperatorCodegen::CodegenFunction CV::OperatorCodegen::getNormalFun(Token tok, const std::vector<llvm::Type*>& types) {
  const Operator::Name& toFind = operatorNameFrom(tok.idx);
  auto opMapIt = codegenMap.find(toFind);
  if (opMapIt == codegenMap.end()) {
    throw InternalError("No such operator", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
  // Try to find the function in the TypeMap using the operand types
  auto funIt = opMapIt->second.find(types);
  if (funIt == opMapIt->second.end()) {
    throw Error("TypeError", cv->typeMismatchErrorString, tok.trace);
  }
  return funIt->second;
}
