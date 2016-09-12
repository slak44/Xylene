#include "llvm/compiler.hpp"

using CmpPred = llvm::CmpInst::Predicate;
using CV = CompileVisitor;

static llvm::Value* loadIfPointer(std::unique_ptr<llvm::IRBuilder<>>& builder, llvm::Value* maybePointer) {
  if (llvm::dyn_cast_or_null<llvm::PointerType>(maybePointer->getType())) {
    return builder->CreateLoad(maybePointer, "loadIdentifier");
  }
  return maybePointer;
}

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
    operands[idx] = loadIfPointer(cv->builder, val);
    return operands[idx]->getType();
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

/**
  \brief Define repetitive arithmetic ops easily.
  
  In order, the ops defined: int/int, int/float, float/int, float/float
  \param builderMethod name of method in llvm::IRBuilder for int ops, without leading "Create"
  \param fltBuilderMethod name of method in llvm::IRBuilder for float ops, without leading "Create"
  \param opTextName lowercase name of operation, used for ir printing
*/
#define ARITHM_PAIRS(builderMethod, fltBuilderMethod, opTextName) \
{{cv->integerType, cv->integerType}, [=] CODEGEN_SIG {\
  return cv->builder->Create##builderMethod(operands[0], operands[1], "int"#opTextName);\
}},\
{{cv->integerType, cv->floatType}, [=] CODEGEN_SIG {\
  return cv->builder->Create##fltBuilderMethod(operands[0], operands[1], "intflt"#opTextName);\
}},\
{{cv->floatType, cv->integerType}, [=] CODEGEN_SIG {\
  return cv->builder->Create##fltBuilderMethod(operands[0], operands[1], "fltint"#opTextName);\
}},\
{{cv->floatType, cv->floatType}, [=] CODEGEN_SIG {\
  return cv->builder->Create##fltBuilderMethod(operands[0], operands[1], "flt"#opTextName);\
}}

/**
  \brief Define repetitive comparison ops easily.
  
  In order, the ops defined: bool/bool, int/int, int/float, float/int, float/float
  \param intCmpPred the llvm::CmpInst::Predicate for int comparison
  \param fltCmpPred the llvm::CmpInst::Predicate for float comparison
  \param opTextName lowercase name of operation, used for ir printing
*/
#define CMP_PAIRS(intCmpPred, fltCmpPred, opTextName) \
{{cv->booleanType, cv->booleanType}, [=] CODEGEN_SIG {\
  return cv->builder->CreateICmp(intCmpPred, operands[0], operands[1], "boolcmp"#opTextName);\
}},\
{{cv->integerType, cv->integerType}, [=] CODEGEN_SIG {\
  return cv->builder->CreateICmp(intCmpPred, operands[0], operands[1], "intcmp"#opTextName);\
}},\
{{cv->integerType, cv->floatType}, [=] CODEGEN_SIG {\
  operands[0] = cv->builder->CreateSIToFP(operands[0], cv->floatType, "SItoFPconv");\
  return cv->builder->CreateFCmp(fltCmpPred, operands[0], operands[1], "intfltcmp"#opTextName);\
}},\
{{cv->floatType, cv->integerType}, [=] CODEGEN_SIG {\
  operands[1] = cv->builder->CreateSIToFP(operands[1], cv->floatType, "SItoFPconv");\
  return cv->builder->CreateFCmp(fltCmpPred, operands[0], operands[1], "fltintcmp"#opTextName);\
}},\
{{cv->floatType, cv->floatType}, [=] CODEGEN_SIG {\
  return cv->builder->CreateFCmp(fltCmpPred, operands[0], operands[1], "fltcmp"#opTextName);\
}}

/**
  \brief Defines a no-op for a unary operator of the specified type.
  \param forType the type of the operand
*/
#define UNARY_NO_OP(forType) {{forType}, [=] CODEGEN_SIG {return operands[0];}}

/**
  \brief Defines a postfix op.
  \param builderMethod Add or Sub; increment by one or decrement by one
  \param opTextName lowercase name of operation, used for ir printing
  TODO work with floats
*/
#define POSTFIX_OP_FUN(builderMethod, opTextName) \
[=] CODEGEN_SIG {\
  auto initial = cv->builder->CreateLoad(operands[0], "postfix"#opTextName"load");\
  if (initial->getType() != cv->integerType) throw Error("TypeError", cv->typeMismatchErrorString, trace);\
  auto changed = cv->builder->Create##builderMethod(initial, llvm::ConstantInt::getSigned(cv->integerType, 1), "int"#opTextName);\
  cv->builder->CreateStore(changed, operands[0]);\
  return initial;\
}

/**
  \brief Defines a prefix op.
  \param builderMethod Add or Sub; increment by one or decrement by one
  \param opTextName lowercase name of operation, used for ir printing
  TODO work with floats
*/
#define PREFIX_OP_FUN(builderMethod, opTextName) \
[=] CODEGEN_SIG {\
  auto initial = cv->builder->CreateLoad(operands[0], "prefix"#opTextName"load");\
  if (initial->getType() != cv->integerType) throw Error("TypeError", cv->typeMismatchErrorString, trace);\
  auto changed = cv->builder->Create##builderMethod(initial, llvm::ConstantInt::getSigned(cv->integerType, 1), "int"#opTextName);\
  return cv->builder->CreateStore(changed, operands[0]);\
}

/**
  \brief Defines a binary bitwise operation.
  \param forType for what llvm::Type to do this op
  \param builderMethod name of method in llvm::IRBuilder for int ops, without leading "Create"
  \param opTextName used for ir printing
*/
#define BITWISE_BIN_PAIR(forType, builderMethod, opTextName)\
{{forType, forType}, [=] CODEGEN_SIG {\
  return cv->builder->Create##builderMethod(operands[0], operands[1], opTextName);\
}}

/**
  \brief Defines a bitwise not operation.
  \param forType for what llvm::Type to do this op
  \param typeName lowercase name of type for ir printing
  \param opTextName lowercase name of operation, used for ir printing
*/
#define BITWISE_NOT_PAIR(forType, typeName, opTextName) \
{{forType}, [=] CODEGEN_SIG {\
  return cv->builder->CreateNot(operands[0], typeName#opTextName"not");\
}}

CV::OperatorCodegen::OperatorCodegen(CompileVisitor::Link cv):
  cv(cv),
  codegenMap({
    {"Add", {
      ARITHM_PAIRS(Add, FAdd, "add")
    }},
    {"Substract", {
      ARITHM_PAIRS(Sub, FSub, "sub")
    }},
    {"Multiply", {
      ARITHM_PAIRS(Mul, FMul, "mul")
    }},
    {"Divide", {
      ARITHM_PAIRS(SDiv, FDiv, "div")
    }},
    {"Modulo", {
      ARITHM_PAIRS(SRem, FRem, "rem")
    }},
    {"Equality", {
      CMP_PAIRS(CmpPred::ICMP_EQ, CmpPred::FCMP_OEQ, "eq")
    }},
    {"Inequality", {
      CMP_PAIRS(CmpPred::ICMP_NE, CmpPred::FCMP_ONE, "noteq")
    }},
    {"Less", {
      CMP_PAIRS(CmpPred::ICMP_SLT, CmpPred::FCMP_OLT, "less")
    }},
    {"Less or equal", {
      CMP_PAIRS(CmpPred::ICMP_SLE, CmpPred::FCMP_OLE, "lessequal")
    }},
    {"Greater", {
      CMP_PAIRS(CmpPred::ICMP_SGT, CmpPred::FCMP_OGT, "greater")
    }},
    {"Greater or equal", {
      CMP_PAIRS(CmpPred::ICMP_SGE, CmpPred::FCMP_OGE, "greaterequal")
    }},
    {"Unary +", {
      UNARY_NO_OP(cv->integerType),
      UNARY_NO_OP(cv->floatType)
    }},
    {"Unary -", {
      {{cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateNeg(operands[0], "negateint");
      }},
      {{cv->floatType}, [=] CODEGEN_SIG {
        return cv->builder->CreateFNeg(operands[0], "negateflt");
      }}
    }},
    {"Bitshift >>", {
      {{cv->integerType, cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateShl(operands[0], operands[1], "leftshift");
      }}
    }},
    {"Bitshift <<", {
      {{cv->integerType, cv->integerType}, [=] CODEGEN_SIG {
        return cv->builder->CreateAShr(operands[0], operands[1], "rightshift");
      }}
    }},
    {"Bitwise NOT", {
      BITWISE_NOT_PAIR(cv->integerType, "int", "bit"),
      BITWISE_NOT_PAIR(cv->booleanType, "bool", "bit")
    }},
    {"Logical NOT", {
      BITWISE_NOT_PAIR(cv->booleanType, "bool", "logical")
    }},
    {"Bitwise AND", {
      BITWISE_BIN_PAIR(cv->integerType, And, "intbitand"),
      BITWISE_BIN_PAIR(cv->booleanType, And, "boolbitand"),
    }},
    {"Logical AND", {
      BITWISE_BIN_PAIR(cv->booleanType, And, "boollogicaland"),
    }},
    {"Bitwise OR", {
      BITWISE_BIN_PAIR(cv->integerType, Or, "intbitor"),
      BITWISE_BIN_PAIR(cv->booleanType, Or, "boolbitor"),
    }},
    {"Logical OR", {
      BITWISE_BIN_PAIR(cv->booleanType, Or, "boollogicalor"),
    }},
    {"Bitwise XOR", {
      BITWISE_BIN_PAIR(cv->integerType, Xor, "intbitxor"),
      BITWISE_BIN_PAIR(cv->booleanType, Xor, "boolbitxor"),
    }}
  }),
  specialCodegenMap({
    {"Assignment", [=] CODEGEN_SIG {
      return cv->builder->CreateStore(operands[1], operands[0]);
    }},
    {"Call", [=] CODEGEN_SIG {
      llvm::Function* funcPtr = llvm::dyn_cast_or_null<llvm::Function>(operands[0]);
      if (funcPtr == nullptr) throw Error("TypeError", "Attempt to call non-function", trace);
      // Slice the func ptr
      auto args = std::vector<llvm::Value*>(operands.begin() + 1, operands.end());
      // TODO: use invoke instead of call in the future, it has exception handling and stuff
      return cv->builder->CreateCall(funcPtr, args, funcPtr->getReturnType()->isVoidTy() ? "" : "call");
    }},
    {"Postfix ++", POSTFIX_OP_FUN(Add, "inc")},
    {"Postfix --", POSTFIX_OP_FUN(Sub, "dec")},
    {"Prefix ++", PREFIX_OP_FUN(Add, "inc")},
    {"Prefix --", PREFIX_OP_FUN(Sub, "dec")}
  }) {}
  
  #undef ARITHM_PAIRS
