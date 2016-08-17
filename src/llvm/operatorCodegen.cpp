#include "llvm/operatorCodegen.hpp"

std::unordered_map<OperatorName, CodegenFunction> specialCodegenMap {
  {"Assignment", [] CODEGEN_SIG {
    return builder.CreateStore(operands[1], operands[0]);
  }}
};

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
