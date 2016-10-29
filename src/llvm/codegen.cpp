#include "llvm/compiler.hpp"

using CmpPred = llvm::CmpInst::Predicate;

static ValueWrapper::Link loadIfPointer(std::unique_ptr<llvm::IRBuilder<>>& builder, ValueWrapper::Link maybePointer) {
  if (llvm::dyn_cast_or_null<llvm::PointerType>(maybePointer->getValue()->getType())) {
    return std::make_shared<ValueWrapper>(
      builder->CreateLoad(maybePointer->getValue(), "loadIfPointer"),
      maybePointer->getCurrentTypeName()
    );
  }
  return maybePointer;
}

ValueWrapper::Link OperatorCodegen::findAndRunFun(Node<ExpressionNode>::Link node, std::vector<ValueWrapper::Link>& operands) {
  // Check if it's a special case
  SpecialCodegenFunction func = getSpecialFun(node);
  if (func != nullptr) return func(operands, node, node->getToken().trace);
  // Otherwise look in the normal map
  // Get a list of type names
  std::vector<TypeName> operandTypes;
  operandTypes.resize(operands.size());
  // Deref pointers
  std::vector<ValueWrapper::Link> processedOperands;
  // Map an operand to a std::string representing its type
  std::transform(ALL(operands), operandTypes.begin(), [=, &operands, &processedOperands](ValueWrapper::Link val) -> TypeName {
    processedOperands.push_back(loadIfPointer(cv->builder, val));
    return val->getCurrentTypeName();
  });
  return getNormalFun(node, operandTypes)(processedOperands, operands, node->getToken().trace);
}

OperatorCodegen::SpecialCodegenFunction OperatorCodegen::getSpecialFun(Node<ExpressionNode>::Link node) noexcept {
  const Operator::Name& toFind = operatorNameFrom(node->getToken().idx);
  auto it = specialCodegenMap.find(toFind);
  if (it != specialCodegenMap.end()) return it->second;
  else return nullptr;
}

OperatorCodegen::CodegenFunction OperatorCodegen::getNormalFun(Node<ExpressionNode>::Link node, const std::vector<TypeName>& types) {
  const Operator::Name& toFind = operatorNameFrom(node->getToken().idx);
  auto opMapIt = codegenMap.find(toFind);
  if (opMapIt == codegenMap.end()) {
    throw InternalError("No such operator", {
      METADATA_PAIRS,
      {"token", node->getToken().toString()}
    });
  }
  // Try to find the function in the TypeMap using the operand types
  auto funIt = opMapIt->second.find(types);
  if (funIt == opMapIt->second.end()) {
    throw Error("TypeError", cv->typeMismatchErrorString, node->getToken().trace);
  }
  return funIt->second;
}

/// Convenience macro for getting 2 values for a binary operator
#define BIN_VALUES operands[0]->getValue(), operands[1]->getValue()

/**
  \brief Define repetitive arithmetic ops easily.
  
  In order, the ops defined: int/int, int/float, float/int, float/float
  \param builderMethod name of method in llvm::IRBuilder for int ops, without leading "Create"
  \param fltBuilderMethod name of method in llvm::IRBuilder for float ops, without leading "Create"
  \param opTextName lowercase name of operation, used for ir printing
*/
#define ARITHM_PAIRS(builderMethod, fltBuilderMethod, opTextName) \
{{"Integer", "Integer"}, [=] CODEGEN_SIG {\
  return std::make_shared<ValueWrapper>(\
    cv->builder->Create##builderMethod(BIN_VALUES, "int" opTextName),\
    "Integer"\
  );\
}},\
{{"Integer", "Float"}, [=] CODEGEN_SIG {\
  return std::make_shared<ValueWrapper>(\
    cv->builder->Create##fltBuilderMethod(BIN_VALUES, "intflt" opTextName),\
    "Float"\
  );\
}},\
{{"Float", "Integer"}, [=] CODEGEN_SIG {\
  return std::make_shared<ValueWrapper>(\
    cv->builder->Create##fltBuilderMethod(BIN_VALUES, "fltint" opTextName),\
    "Float"\
  );\
}},\
{{"Float", "Float"}, [=] CODEGEN_SIG {\
  return std::make_shared<ValueWrapper>(\
    cv->builder->Create##fltBuilderMethod(BIN_VALUES, "flt" opTextName),\
    "Float"\
  );\
}}

/**
  \brief Util function to create ValueWrappers with booleans more easily
*/
static inline ValueWrapper::Link boolVal(llvm::Value* v) {
  return std::make_shared<ValueWrapper>(v, "Boolean");
}

/**
  \brief Define repetitive comparison ops easily.
  
  In order, the ops defined: bool/bool, int/int, int/float, float/int, float/float
  \param intCmpPred the llvm::CmpInst::Predicate for int comparison
  \param fltCmpPred the llvm::CmpInst::Predicate for float comparison
  \param opTextName lowercase name of operation, used for ir printing
*/
#define CMP_PAIRS(intCmpPred, fltCmpPred, opTextName) \
{{"Boolean", "Boolean"}, [=] CODEGEN_SIG {\
  return boolVal(cv->builder->CreateICmp(intCmpPred, BIN_VALUES, "boolcmp" opTextName));\
}},\
{{"Integer", "Integer"}, [=] CODEGEN_SIG {\
  return boolVal(cv->builder->CreateICmp(intCmpPred, BIN_VALUES, "intcmp" opTextName));\
}},\
{{"Integer", "Float"}, [=] CODEGEN_SIG {\
  operands[0]->setValue(\
    cv->builder->CreateSIToFP(operands[0]->getValue(), cv->floatType, "SItoFPconv"),\
    "Float"\
  );\
  return boolVal(cv->builder->CreateFCmp(fltCmpPred, BIN_VALUES, "intfltcmp" opTextName));\
}},\
{{"Float", "Integer"}, [=] CODEGEN_SIG {\
  operands[1]->setValue(\
    cv->builder->CreateSIToFP(operands[1]->getValue(), cv->floatType, "SItoFPconv"),\
    "Float"\
  );\
  return boolVal(cv->builder->CreateFCmp(fltCmpPred, BIN_VALUES, "fltintcmp" opTextName));\
}},\
{{"Float", "Float"}, [=] CODEGEN_SIG {\
  return boolVal(cv->builder->CreateFCmp(fltCmpPred, BIN_VALUES, "fltcmp" opTextName));\
}}

/**
  \brief Defines a no-op for a unary operator of the specified type.
  \param forType the type of the operand
*/
#define UNARY_NO_OP(forType) {{forType}, [=] CODEGEN_SIG {return operands[0];}}

/**
  \brief Convenience method to get either 1 or 1.0
  \param ofWhat either "Integer" or "Float"
*/
static llvm::Constant* getOne(std::string ofWhat, llvm::Type* intTy, llvm::Type* fltTy) {
  if (ofWhat == "Integer") return llvm::ConstantInt::getSigned(intTy, 1);
  else if (ofWhat == "Float") return llvm::ConstantFP::get(fltTy, 1.0f);
  else throw InternalError("Wrong parameter, can only be Integer or Float", {
    METADATA_PAIRS,
    {"param", ofWhat}
  });
}

/**
  \brief Defines a postfix op.
  \param builderMethod name of method in llvm::IRBuilder for ops, without leading "Create"
  \param type what type is the value
  \param opTextName lowercase name of operation, used for ir printing
*/
#define POSTFIX_OP_FUN(builderMethod, type, opTextName) \
{{type}, [=] CODEGEN_SIG {\
  auto initial = cv->builder->CreateLoad(rawOperands[0]->getValue(), "postfix" opTextName "load");\
  auto changed = cv->builder->Create##builderMethod(initial, getOne(type, cv->integerType, cv->floatType), "int" opTextName);\
  cv->builder->CreateStore(changed, rawOperands[0]->getValue());\
  return std::make_shared<ValueWrapper>(initial, type);\
}}

/**
  \brief Defines a prefix op.
  \param builderMethod name of method in llvm::IRBuilder for ops, without leading "Create"
  \param type what type is the value
  \param opTextName lowercase name of operation, used for ir printing
*/
#define PREFIX_OP_FUN(builderMethod, type, opTextName) \
{{type}, [=] CODEGEN_SIG {\
  auto initial = cv->builder->CreateLoad(rawOperands[0]->getValue(), "prefix" opTextName "load");\
  auto changed = cv->builder->Create##builderMethod(initial, getOne(type, cv->integerType, cv->floatType), "int" opTextName);\
  return std::make_shared<ValueWrapper>(cv->builder->CreateStore(changed, rawOperands[0]->getValue()), type);\
}}

/**
  \brief Defines a binary bitwise operation.
  \param forType for what llvm::Type to do this op
  \param builderMethod name of method in llvm::IRBuilder for int ops, without leading "Create"
  \param opTextName used for ir printing
*/
#define BITWISE_BIN_PAIR(forType, builderMethod, opTextName)\
{{forType, forType}, [=] CODEGEN_SIG {\
  return std::make_shared<ValueWrapper>(\
    cv->builder->Create##builderMethod(BIN_VALUES, opTextName),\
    forType\
  );\
}}

/**
  \brief Defines a bitwise not operation.
  \param forType for what type to do this op
  \param typeName lowercase name of type for ir printing
  \param opTextName lowercase name of operation, used for ir printing
*/
#define BITWISE_NOT_PAIR(forType, typeName, opTextName) \
{{forType}, [=] CODEGEN_SIG {\
  return std::make_shared<ValueWrapper>(\
    cv->builder->CreateNot(operands[0]->getValue(), typeName opTextName "not"),\
    forType\
  );\
}}

OperatorCodegen::OperatorCodegen(CompileVisitor::Link cv):
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
      UNARY_NO_OP("Integer"),
      UNARY_NO_OP("Float")
    }},
    {"Unary -", {
      {{"Integer"}, [=] CODEGEN_SIG {
        return std::make_shared<ValueWrapper>(
          cv->builder->CreateNeg(operands[0]->getValue(), "negateint"),
          "Integer"
        );
      }},
      {{"Float"}, [=] CODEGEN_SIG {
        return std::make_shared<ValueWrapper>(
          cv->builder->CreateFNeg(operands[0]->getValue(), "negateflt"),
          "Float"
        );
      }}
    }},
    {"Bitshift >>", {
      {{"Integer", "Integer"}, [=] CODEGEN_SIG {
        return std::make_shared<ValueWrapper>(
          cv->builder->CreateShl(BIN_VALUES, "leftshift"),
          "Integer"
        );
      }}
    }},
    {"Bitshift <<", {
      {{"Integer", "Integer"}, [=] CODEGEN_SIG {
        return std::make_shared<ValueWrapper>(
          cv->builder->CreateAShr(BIN_VALUES, "rightshift"),
          "Integer"
        );
      }}
    }},
    {"Bitwise NOT", {
      BITWISE_NOT_PAIR("Integer", "int", "bit"),
      BITWISE_NOT_PAIR("Boolean", "bool", "bit")
    }},
    {"Logical NOT", {
      BITWISE_NOT_PAIR("Boolean", "bool", "logical")
    }},
    {"Bitwise AND", {
      BITWISE_BIN_PAIR("Integer", And, "intbitand"),
      BITWISE_BIN_PAIR("Boolean", And, "boolbitand"),
    }},
    {"Logical AND", {
      BITWISE_BIN_PAIR("Boolean", And, "boollogicaland"),
    }},
    {"Bitwise OR", {
      BITWISE_BIN_PAIR("Integer", Or, "intbitor"),
      BITWISE_BIN_PAIR("Boolean", Or, "boolbitor"),
    }},
    {"Logical OR", {
      BITWISE_BIN_PAIR("Boolean", Or, "boollogicalor"),
    }},
    {"Bitwise XOR", {
      BITWISE_BIN_PAIR("Integer", Xor, "intbitxor"),
      BITWISE_BIN_PAIR("Boolean", Xor, "boolbitxor"),
    }},
    {"Postfix ++", {
      POSTFIX_OP_FUN(Add, "Integer", "inc"),
      POSTFIX_OP_FUN(FAdd, "Float", "inc")
    }},
    {"Postfix --", {
      POSTFIX_OP_FUN(Sub, "Integer", "dec"),
      POSTFIX_OP_FUN(FSub, "Float", "dec")
    }},
    {"Prefix ++", {
      PREFIX_OP_FUN(Add, "Integer", "inc"),
      PREFIX_OP_FUN(FAdd, "Float", "inc")
    }},
    {"Prefix --", {
      PREFIX_OP_FUN(Sub, "Integer", "dec"),
      PREFIX_OP_FUN(FSub, "Float", "dec")
    }}
  }),
  specialCodegenMap({
    {"Assignment", [=] SPECIAL_CODEGEN_SIG {
      auto varIdent = Node<ExpressionNode>::staticPtrCast(node)->at(0);
      DeclarationWrapper::Link decl = PtrUtil<DeclarationWrapper>::dynPtrCast(operands[0]);
      if (decl == nullptr) throw Error("ReferenceError", "Cannot assign to this value", varIdent->getToken().trace);
      // If the declaration doesn't allow this type, complain
      if (!cv->isTypeAllowedIn(decl->getTypeList(), operands[1]->getCurrentTypeName())) {
        throw Error("TypeError",
          "Type list of '" + varIdent->getToken().data +
          "' does not contain type '" + operands[1]->getCurrentTypeName() + "'",
          varIdent->getToken().trace
        );
      }
      // Store into the variable
      cv->builder->CreateStore(operands[1]->getValue(), operands[0]->getValue());
      // The value of the decl will remain the pointer to the var in memory,
      // The type is changed to the newly assigned one
      decl->setValue(decl->getValue(), operands[1]->getCurrentTypeName());
      // Return the assigned value
      return operands[1];
    }},
    {"Call", [=] SPECIAL_CODEGEN_SIG {
      if (operands[0]->getCurrentTypeName() != "Function") {
        throw Error("TypeError", "Attempt to call non-function", trace);
      }
      auto fw = PtrUtil<FunctionWrapper>::staticPtrCast(operands[0]);
      // Get a list of arguments to pass to CreateCall
      std::vector<llvm::Value*> args {};
      // We skip the first operand because it is the function itself, not an arg
      auto opIt = operands.begin() + 1;
      auto map = fw->getSignature().getArguments();
      if (operands.size() - 1 != fw->getSignature().getArguments().size()) {
        throw InternalError(
          "Operand count mismatches func sig argument count",
          {
            METADATA_PAIRS,
            {"ops", std::to_string(operands.size() - 1)},
            {"args", std::to_string(fw->getSignature().getArguments().size())},
          }
        );
      }
      for (auto it = map.begin(); it != map.end(); ++it, ++opIt) {
        if (!cv->isTypeAllowedIn(it->second.getEvalTypeList(), (*opIt)->getCurrentTypeName())) {
          throw Error("TypeError",
            "Function argument '" + it->first + "' type list does not contain type '" +
            (*opIt)->getCurrentTypeName() + "'",
            trace
          );
        }
        args.push_back((*opIt)->getValue());
      }
      TypeList returnTl = fw->getSignature().getReturnType().getEvalTypeList();
      TypeName returnedType;
      if (returnTl.size() == 1) returnedType = *returnTl.begin();
      else throw InternalError("Not implemented", {METADATA_PAIRS});
      // TODO: use invoke instead of call in the future, it has exception handling and stuff
      return std::make_shared<ValueWrapper>(
        cv->builder->CreateCall(
          fw->getValue(),
          args,
          fw->getValue()->getReturnType()->isVoidTy() ? "" : "call"
        ),
        returnedType
      );
    }}
  }) {}
  
#undef BIN_VALUES
#undef ARITHM_PAIRS
#undef CMP_PAIRS
#undef UNARY_NO_OP
#undef POSTFIX_OP_FUN
#undef PREFIX_OP_FUN
#undef BITWISE_BIN_PAIR
#undef BITWISE_NOT_PAIR
