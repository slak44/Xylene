#include "operator_maps.hpp"

namespace lang {
  Variable* resolveNameFrom(ASTNode* localNode, std::string identifier) {
    if (localNode == nullptr) return nullptr; // Reached tree root and didn't find anything.
    try {
      return localNode->getScope()->at(identifier);
    } catch(std::out_of_range& oor) {
      return resolveNameFrom(localNode->getParent(), identifier);
    }
  }
  
  void concatenateNames(std::string& result, ExpressionChildNode* operatorNode) {
    result.pop_back(); // Remove trailing space
  }

  template<typename... Args>
  void concatenateNames(std::string& result, ExpressionChildNode* operatorNode, Object*& obj, Args&... args) {
    if (obj == nullptr) throw Error("Object has not been defined", "NullPointerError", operatorNode->getLineNumber());
    while (obj->getTypeData() == "Variable") {
      obj = dynamic_cast<Variable*>(obj)->read();
      if (obj == nullptr) throw Error("Variable is empty or has not been defined", "NullPointerError", operatorNode->getLineNumber());
    }
    result += obj->getTypeData() + " ";
    concatenateNames(result, operatorNode, args...);
  }

  template<typename... Args>
  Object* runOperator(ExpressionChildNode* operatorNode, Args... operands) {
    Operator* op = static_cast<Operator*>(operatorNode->t.typeData);
    std::string funSig = "";
    if (op->toString().back() == '=' && op->getPrecedence() == 1) funSig = "Variable Object"; // Intercepts all assignments
    else if (op->toString() == "++" || op->toString() == "--") funSig = "Variable";
    else concatenateNames(funSig, operatorNode, operands...);
    // 1. Get a boost::any instance from the OperatorMap
    // 2. Use boost::any_cast to get a pointer to the operator function
    // 3. Dereference pointer and call function
    try {
      auto result = (*boost::any_cast<std::function<Object*(Args...)>*>(opsMap[*op][funSig]))(operands...);
      return result;
    } catch (boost::bad_any_cast& bac) {
      throw Error("Cannot find operation for '" + op->toString() + "' and operands '" + funSig + "'", "NullPointerError", operatorNode->getLineNumber());
    }
  }
  
  template<typename... Args>
  Object* runOperator(Operator op, Args... operands) {
    return runOperator(new ExpressionChildNode(Token(&op, OPERATOR, PHONY_TOKEN)), operands...);
  }
  
  Object* fromExprChildNode(ASTNode* node) {
    return static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(node)->t.typeData);
  }

  Object* runOperator(ExpressionChildNode* operatorNode) {
    auto arity = static_cast<Operator*>(operatorNode->t.typeData)->getArity();
    std::vector<Object*> operands {};
    auto operandCount = operatorNode->getChildren().size();
    for (std::size_t i = operandCount; i != 0; i--) {
      Object* operand;
      ExpressionChildNode* operandNode = dynamic_cast<ExpressionChildNode*>(operatorNode->getChildren()[i - 1]);
      if (operandNode->t.type == VARIABLE) {
        operand = resolveNameFrom(operandNode, operandNode->t.data);
      }
      else operand = fromExprChildNode(operandNode);
      if (operand == nullptr) throw std::runtime_error("Attempt to call operator " + operatorNode->t.data + " with operand " + std::to_string(operandCount - i) + " set to nullptr.");
      operands.push_back(operand);
    }
    
    if (arity == UNARY) return runOperator(operatorNode, operands[0]);
    else if (arity == BINARY) return runOperator(operatorNode, operands[0], operands[1]);
    else if (arity == TERNARY) return runOperator(operatorNode, operands[0], operands[1], operands[2]);
    else throw std::runtime_error("Attempt to call operator with more than 3 operands(" + std::to_string(arity) + ")");
  }
  
  // TODO: some operators that affect variables do not change the value of the variable, eg `++i`
  OperatorMap opsMap = {
    // Plain assignment
    {Operator("=", 1, ASSOCIATE_FROM_RIGHT, BINARY), {
      {"Variable Object", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        auto left = dynamic_cast<Variable*>(l);
        left->assign(r);
        return r;
      }))}
    }},
    // Assignment with extra operation
    MAKE_ASSIGNMENT_OP(+, 10),
    MAKE_ASSIGNMENT_OP(-, 10),
    MAKE_ASSIGNMENT_OP(*, 11),
    MAKE_ASSIGNMENT_OP(/, 11),
    MAKE_ASSIGNMENT_OP(%, 11),
    MAKE_ASSIGNMENT_OP(<<, 9),
    MAKE_ASSIGNMENT_OP(>>, 9),
    MAKE_ASSIGNMENT_OP(&, 6),
    MAKE_ASSIGNMENT_OP(^, 5),
    MAKE_ASSIGNMENT_OP(|, 4),
    // Comparison operators
    {Operator("<=", 8), {
      EXPAND_COMPARISON_OPS(<=)
    }},
    {Operator("<", 8), {
      EXPAND_COMPARISON_OPS(<)
    }},
    {Operator(">=", 8), {
      EXPAND_COMPARISON_OPS(>=)
    }},
    {Operator(">", 8), {
      EXPAND_COMPARISON_OPS(>)
    }},
    {Operator("==", 7), {
      EXPAND_COMPARISON_OPS(==),
      {MAKE_BINARY_OP(String, String, new Boolean(left->toString() == right->toString()) )},
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() == right->value()) )}
    }},
    {Operator("!=", 7), {
      EXPAND_COMPARISON_OPS(!=),
      {MAKE_BINARY_OP(String, String, new Boolean(left->toString() != right->toString()) )},
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() != right->value()) )}
    }},
    // Arithmetic operators
    {Operator("+", 10), {
      {MAKE_BINARY_OP(String, String, new String(left->toString() + right->toString()) )}, // String concatenation
      EXPAND_NUMERIC_OPS(+)
    }},
    {Operator("-", 10), {
      EXPAND_NUMERIC_OPS(-)
    }},
    {Operator("*", 11), {
      EXPAND_NUMERIC_OPS(*)
    }},
    {Operator("/", 11), {
      EXPAND_NUMERIC_OPS(/)
    }},
    {Operator("%", 11), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() % right->getNumber()) )}
    }},
    // Bitwise operators
    {Operator(">>", 9), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() >> right->getNumber()) )}
    }},
    {Operator("<<", 9), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() << right->getNumber()) )}
    }},
    {Operator("&", 6), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() & right->getNumber()) )}
    }},
    {Operator("^", 5), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() ^ right->getNumber()) )}
    }},
    {Operator("|", 4), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() | right->getNumber()) )}
    }},
    {Operator("~", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(~operand->getNumber()) )}
    }},
    // Logical operators
    {Operator("!", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Boolean, new Boolean(!operand->value()) )},
      {MAKE_UNARY_OP(Integer, new Boolean(!operand->getNumber()) )}
    }},
    {Operator("&&", 3), {
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() && right->value()) )},
      {MAKE_BINARY_OP(Integer, Boolean, new Boolean(left->getNumber() && right->value()) )},
      {MAKE_BINARY_OP(Boolean, Integer, new Boolean(left->value() && right->getNumber()) )},
      {MAKE_BINARY_OP(Integer, Integer, new Boolean(left->getNumber() && right->getNumber()) )}
    }},
    {Operator("||", 2), {
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() || right->value()) )},
      {MAKE_BINARY_OP(Integer, Boolean, new Boolean(left->getNumber() || right->value()) )},
      {MAKE_BINARY_OP(Boolean, Integer, new Boolean(left->value() || right->getNumber()) )},
      {MAKE_BINARY_OP(Integer, Integer, new Boolean(left->getNumber() || right->getNumber()) )}
    }},
    // Signum operators
    {Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(+operand->getNumber()) )},
      {MAKE_UNARY_OP(Float, new Float(+operand->getNumber()) )}
    }},
    {Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(-operand->getNumber()) )},
      {MAKE_UNARY_OP(Float, new Float(-operand->getNumber()) )}
    }},
    // Prefix operators
    {MAKE_MUTATING_UNARY_OP(Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY), +, true)},
    {MAKE_MUTATING_UNARY_OP(Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY), -, true)},
    // Postfix operators
    {MAKE_MUTATING_UNARY_OP(Operator("++", 13, ASSOCIATE_FROM_LEFT, UNARY), +, false)},
    {MAKE_MUTATING_UNARY_OP(Operator("--", 13, ASSOCIATE_FROM_LEFT, UNARY), -, false)}
    // TODO: member access op
    // TODO: maybe comma op
  };
} /* namespace lang */