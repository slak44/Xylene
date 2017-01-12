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
  
Operator::Symbol Operator::getSymbol() const {return symbol;}
int Operator::getPrec() const {return precedence;}
Operator::Name Operator::getName() const {return name;}
Associativity Operator::getAssociativity() const {return associativity;}
Arity Operator::getArity() const {return arity;}
Fixity Operator::getFixity() const {return fixity;}
Operator::RequireReferenceList Operator::getRefList() const {return refList;}

bool Operator::hasSymbol(Operator::Symbol s) const {return s == symbol;}
bool Operator::hasPrec(int p) const {return p == precedence;}
bool Operator::hasName(Operator::Name n) const {return n == name;}
bool Operator::hasAsoc(Associativity a) const {return a == associativity;}
bool Operator::hasArity(Arity a) const {return a == arity;}
bool Operator::hasFixity(Fixity f) const {return f == fixity;}

bool Operator::operator==(const Operator& rhs) const {
  return symbol == rhs.symbol &&
    precedence == rhs.precedence &&
    name == rhs.name &&
    associativity == rhs.associativity &&
    arity == rhs.arity &&
    fixity == rhs.fixity &&
    refList == rhs.refList;
}
bool Operator::operator!=(const Operator& rhs) const {
  return !operator==(rhs);
}

/// Assignment requires that the first operand is mutable
static const Operator::RequireReferenceList assignmentList {true, false};
/// Some unary operators (eg ++) mutate their operand
static const Operator::RequireReferenceList unaryOps {true};

const std::vector<Operator> Operator::list {
  Operator("==", 7, "Equality"),
  Operator("!=", 7, "Inequality"),
  
  Operator("=", 1, "Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("+=", 1, "+Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("-=", 1, "-Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("*=", 1, "*Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("/=", 1, "/Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("%=", 1, "%Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("<<=", 1, "<<Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator(">>=", 1, ">>Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("&=", 1, "&Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("^=", 1, "^Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("|=", 1, "|Assignment", ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  
  Operator(">>", 9, "Bitshift >>"),
  Operator("<<", 9, "Bitshift <<"),
  
  Operator("<=", 8, "Less or equal"),
  Operator("<", 8, "Less"),
  Operator(">=", 8, "Greater or equal"),
  Operator(">", 8, "Greater"),
  
  Operator("--", 13, "Postfix --", ASSOCIATE_FROM_LEFT, UNARY, POSTFIX, unaryOps),
  Operator("++", 13, "Postfix ++", ASSOCIATE_FROM_LEFT, UNARY, POSTFIX, unaryOps),
  
  Operator(".", 13, "Member access", ASSOCIATE_FROM_LEFT, BINARY, INFIX, {true, false}),
  
  Operator("--", 12, "Prefix --", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX, unaryOps),
  Operator("++", 12, "Prefix ++", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX, unaryOps),
  Operator("-", 12, "Unary -", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("+", 12, "Unary +", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("~", 12, "Bitwise NOT", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("!", 12, "Logical NOT", ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  
  Operator("*", 11, "Multiply"),
  Operator("/", 11, "Divide"),
  Operator("%", 11, "Modulo"),
  
  Operator("+", 10, "Add"),
  Operator("-", 10, "Substract"),
  
  Operator("&&", 3, "Logical AND"),
  Operator("||", 2, "Logical OR"),
  
  Operator("&", 6, "Bitwise AND"),
  Operator("^", 5, "Bitwise XOR"),
  Operator("|", 4, "Bitwise OR"),
  
  Operator(",", 0, "Comma"),
  
  // These don't get matched by the lexer as operators, their symbols get matched as
  // constructs. The expressions using those are created in the parser
  Operator("[]", 13, "Subscript", ASSOCIATE_FROM_LEFT, BINARY, CIRCUMFIX, {true, false}),
  // Call op will contain argument tree with commas + name of function
  Operator("()", 13, "Call", ASSOCIATE_FROM_LEFT, BINARY, POSTFIX, {false, true}),
  Operator("?:", 1, "Conditional", ASSOCIATE_FROM_LEFT, TERNARY, CIRCUMFIX),
  Operator(" ", 0, "No-op", ASSOCIATE_FROM_LEFT, NULLARY)
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
