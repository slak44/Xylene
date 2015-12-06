#include "operator_maps.hpp"

namespace lang {
  void concatenateNames(std::string& result, uint line) {
    result.pop_back(); // Remove trailing space
  }

  template<typename... Args>
  void concatenateNames(std::string& result, uint line, Object*& obj, Args&... args) {
    if (obj == nullptr) throw TypeError("Variable is empty or has not been defined", line);
    while (obj->getTypeData() == "Variable") {
      obj = dynamic_cast<Variable*>(obj)->read();
      if (obj == nullptr) throw TypeError("Variable is empty or has not been defined", line);
    }
    result += obj->getTypeData() + " ";
    concatenateNames(result, line, args...);
  }

  template<typename... Args>
  Object* runOperator(ExpressionChildNode* operatorNode, uint line, Args... operands) {
    Operator* op = static_cast<Operator*>(operatorNode->t.typeData);
    std::string funSig = "";
    if (op->toString().back() == '=' && op->getPrecedence() == 1) funSig = "Variable Object";
    else concatenateNames(funSig, line, operands...);
    // 1. Get a boost::any instance from the OperatorMap
    // 2. Use boost::any_cast to get a pointer to the operator function
    // 3. Dereference pointer and call function
    try {
      auto result = (*boost::any_cast<std::function<Object*(Args...)>*>(opsMap[*op][funSig]))(operands...);
      return result;
    } catch (boost::bad_any_cast& bac) {
      throw TypeError("Cannot find operation for operator '" + op->toString() + "' and operands '" + funSig + "'", line);
    }
  }
  
  Object* fromExprChildNode(ASTNode* node) {
    return static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(node)->t.typeData);
  }

  Object* runOperator(ExpressionChildNode* operatorNode) {
    uint lineNumber = operatorNode->getLineNumber(); // TODO: not all nodes implement line numbers yet
    auto arity = static_cast<Operator*>(operatorNode->t.typeData)->getArity();
    if (arity == UNARY) {
      Object* operand = fromExprChildNode(operatorNode->getChildren()[0]);
      return runOperator(operatorNode, lineNumber, operand);
    } else if (arity == BINARY) {
      Object* operandLeft = fromExprChildNode(operatorNode->getChildren()[1]);
      Object* operandRight = fromExprChildNode(operatorNode->getChildren()[0]);
      return runOperator(operatorNode, lineNumber, operandLeft, operandRight);
    } else if (arity == TERNARY) {
      Object* operand1 = fromExprChildNode(operatorNode->getChildren()[2]);
      Object* operand2 = fromExprChildNode(operatorNode->getChildren()[1]);
      Object* operand3 = fromExprChildNode(operatorNode->getChildren()[0]);
      return runOperator(operatorNode, lineNumber, operand1, operand2, operand3);
    } else throw std::runtime_error("Attempt to call operator with more than 3 operands(" + std::to_string(arity) + ")");
  }
  
  // TODO: some operators that affect variables do not change the value of the variable, eg `++i`
  OperatorMap opsMap = {
    {Operator("=", 1, ASSOCIATE_FROM_RIGHT, BINARY), {
      {"Variable Object", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        auto left = dynamic_cast<Variable*>(l);
        left->assign(r);
        return r;
      }))}
    }},
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
    // Other unary ops
    {Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(+operand->getNumber()) )},
      {MAKE_UNARY_OP(Float, new Float(+operand->getNumber()) )}
    }},
    {Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(-operand->getNumber()) )},
      {MAKE_UNARY_OP(Float, new Float(-operand->getNumber()) )}
    }},
    {Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(operand->getNumber() + 1) )},
      {MAKE_UNARY_OP(Float, new Float(operand->getNumber() + 1) )}
    }},
    {Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(operand->getNumber() - 1) )},
      {MAKE_UNARY_OP(Float, new Float(operand->getNumber() - 1) )}
    }},
    // TODO: postfix ops
    // TODO: other assignment ops
    // TODO: member access op
    // TODO: maybe comma op
  };
} /* namespace lang */