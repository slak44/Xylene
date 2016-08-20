#include "llvm/operatorCodegen.hpp"

std::unordered_map<Operator::Name, CodegenFunction> specialCodegenMap {
  {"Assignment", [] CODEGEN_SIG {
    return builder.CreateStore(operands[1], operands[0]);
  }},
  {"Postfix ++", [] CODEGEN_SIG {
    auto initial = builder.CreateLoad(operands[0], "postfixincload");
    if (initial->getType() != integerType) throw Error("TypeError", typeMismatchErrorString, line); 
    auto plusOne = builder.CreateAdd(initial, llvm::ConstantInt::getSigned(integerType, 1), "intinc");
    builder.CreateStore(plusOne, operands[0]);
    return initial;
  }}
};

using CmpPred = llvm::CmpInst::Predicate;

std::unordered_map<Operator::Name, TypeMap> codegenMap {
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
  }},
  {"Less", {
    {{L_INTEGER, L_INTEGER}, [] CODEGEN_SIG {
      return builder.CreateICmp(CmpPred::ICMP_SLT, operands[0], operands[1], "intcmpless");
    }},
    {{L_INTEGER, L_FLOAT}, [] CODEGEN_SIG {
      return builder.CreateFCmp(CmpPred::FCMP_OLT, SItoFP(builder, operands[0]), operands[1], "intfltcmpless");
    }},
    {{L_FLOAT, L_INTEGER}, [] CODEGEN_SIG {
      return builder.CreateFCmp(CmpPred::FCMP_OLT, operands[0], SItoFP(builder, operands[1]), "fltintcmpless");
    }},
    {{L_FLOAT, L_FLOAT}, [] CODEGEN_SIG {
      return builder.CreateFCmp(CmpPred::FCMP_OLT, operands[0], operands[1], "fltcmpless");
    }},
  }}
};
