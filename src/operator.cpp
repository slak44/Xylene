#include "operator.hpp"

Operator::Operator(
  Operator::Symbol symbol,
  int precedence,
  Operator::Name name,
  Associativity associativity,
  Arity arity,
  Fixity fixity,
  RequireReferenceList refList
):
  symbol(symbol),
  precedence(precedence),
  name(name),
  associativity(associativity),
  arity(arity),
  fixity(fixity),
  refList(refList) {
  if (arity == UNARY && fixity == INFIX) {
    throw InternalError("There are no unary infix operators", {METADATA_PAIRS});
  }
}
  
bool Operator::operator==(const Operator& rhs) const noexcept {
  return symbol == rhs.symbol &&
    precedence == rhs.precedence &&
    name == rhs.name &&
    associativity == rhs.associativity &&
    arity == rhs.arity &&
    fixity == rhs.fixity &&
    refList == rhs.refList;
}
bool Operator::operator!=(const Operator& rhs) const noexcept {
  return !operator==(rhs);
}

namespace {
  /// Assignment requires that the first operand is mutable
  const Operator::RequireReferenceList assignmentList {true, false};
  /// Some unary operators (eg ++) mutate their operand
  const Operator::RequireReferenceList unaryOps {true};
}

const std::vector<Operator> Operator::list {
  Operator("==", 70, "Equality"),
  Operator("!=", 70, "Inequality"),
  
  Operator("=", 10, "Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("+=", 10, "+Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("-=", 10, "-Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("*=", 10, "*Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("/=", 10, "/Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("%=", 10, "%Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("<<=", 10, "<<Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator(">>=", 10, ">>Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("&=", 10, "&Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("^=", 10, "^Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("|=", 10, "|Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  
  Operator(">>", 90, "Bitshift >>"),
  Operator("<<", 90, "Bitshift <<"),
  
  Operator("<=", 80, "Less or equal"),
  Operator("<", 80, "Less"),
  Operator(">=", 80, "Greater or equal"),
  Operator(">", 80, "Greater"),
  
  Operator("--", 130, "Postfix --", ASSOCIATE_FROM_LEFT, UNARY, POSTFIX, unaryOps),
  Operator("++", 130, "Postfix ++", ASSOCIATE_FROM_LEFT, UNARY, POSTFIX, unaryOps),
  
  Operator(".", 130, "Member access", ASSOCIATE_FROM_LEFT, BINARY, INFIX, {true, false}),
  
  Operator("--", 120, "Prefix --", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX, unaryOps),
  Operator("++", 120, "Prefix ++", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX, unaryOps),
  Operator("-", 120, "Unary -", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("+", 120, "Unary +", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("~", 120, "Bitwise NOT", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("!", 120, "Logical NOT", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  
  Operator("*", 110, "Multiply"),
  Operator("/", 110, "Divide"),
  Operator("%", 110, "Modulo"),
  
  Operator("+", 100, "Add"),
  Operator("-", 100, "Substract"),
  
  Operator("&&", 30, "Logical AND"),
  Operator("||", 20, "Logical OR"),
  
  Operator("&", 60, "Bitwise AND"),
  Operator("^", 50, "Bitwise XOR"),
  Operator("|", 40, "Bitwise OR"),
  
  Operator(",", 0, "Comma"),
  
  // These don't get matched by the lexer as operators, their symbols get matched as
  // constructs. The expressions using those are created in the parser
  Operator("[]", 130, "Subscript", ASSOCIATE_FROM_LEFT, BINARY, CIRCUMFIX, {true, false}),
  // Second operand to call is the thing being called, first is arguments to call
  Operator("()", 130, "Call", ASSOCIATE_FROM_LEFT, BINARY, POSTFIX, {false, true}),
  Operator("?:", 10, "Conditional", ASSOCIATE_FROM_LEFT, TERNARY, CIRCUMFIX),
  Operator(" ", 0, "Call arguments", ASSOCIATE_FROM_LEFT, POLYADIC)
};

const std::unordered_set<char> Operator::operatorCharacters = std::accumulate(
  ALL(Operator::list),
  std::unordered_set<char> {},
  [](std::unordered_set<char>& previous, const Operator op) {
    for (char c : op.getSymbol()) previous.insert(c);
    return previous;
  }
);

Operator::Index Operator::find(Operator::Name name) {
  int idx = -1;
  auto it = std::find_if(ALL(Operator::list), [&](auto op) {
    idx++;
    return op.getName() == name;
  });
  if (it == Operator::list.end()) throw InternalError("No such operator", {
    METADATA_PAIRS,
    {"offending Operator::Name", name}
  });
  return static_cast<Operator::Index>(idx);
}
