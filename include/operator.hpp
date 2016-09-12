#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "utils/util.hpp"
#include "utils/error.hpp"

/**
  \brief Represents how an operator associates.
*/
enum Associativity: int {
  /// The default. Operators associate from left to right.
  ASSOCIATE_FROM_LEFT,
  /// Operators associate from right to left.
  ASSOCIATE_FROM_RIGHT
};

/**
  \brief Represents the amount of operands this operator has.
*/
enum Arity: int {
  NULLARY = 0,
  UNARY = 1,
  BINARY = 2, ///< The default
  TERNARY = 3
};

/**
  \brief Represents where this operator should be placed in respect to his operands.
*/
enum Fixity: int {
  /// The default
  INFIX = 0,
  PREFIX = -1,
  POSTFIX = 1,
  CIRCUMFIX = 2
};

/**
  \brief Class that represents an operator.
  \sa Associativity, Arity, Fixity, operatorList
*/
class Operator {
public:
  /// The symbol used for the operator, eg '+', '=', '&&'
  using Symbol = std::string;
  /// Convenience names from the operatorNameMap below
  using Name = std::string;
  /// Where in the operatorList below it is
  using Index = int64;
  /// Require that the operand at that index can be mutated if true
  using RequireReferenceList = std::vector<bool>;
private:
  Symbol name;
  int precedence;
  Associativity associativity;
  Arity arity;
  Fixity fixity;
  RequireReferenceList refList;
public:
  /**
    \brief Creates an operator. Only the Symbol and the precedence are mandatory.
    TODO this should not be public
  */
  Operator(
    Symbol name,
    int precedence,
    Associativity associativity = ASSOCIATE_FROM_LEFT,
    Arity arity = BINARY,
    Fixity fixity = INFIX,
    RequireReferenceList refList = {false, false, false}
  );
    
  Symbol getName() const;
  int getPrecedence() const;
  Associativity getAssociativity() const;
  Arity getArity() const;
  Fixity getFixity() const;
  RequireReferenceList getRefList() const;
  
  bool operator==(const Operator& rhs) const;
  bool operator!=(const Operator& rhs) const;
};

/// Assignment requires that the first operand is mutable
static const Operator::RequireReferenceList assignmentList {true, false};
/// Some unary operators (eg ++) mutate their operand
static const Operator::RequireReferenceList unaryOps {true};

/**
  \brief The main operator list.
  This list contains the operators used by the lexer. Some of them, like (), ?: and [] can't be here due to their syntax.
*/
const std::vector<Operator> operatorList {
  Operator("==", 7),
  Operator("!=", 7),
  
  Operator("=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("+=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("-=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("*=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("/=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("%=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("<<=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator(">>=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("&=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("^=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  Operator("|=", 1, ASSOCIATE_FROM_RIGHT, BINARY, INFIX, assignmentList),
  
  Operator(">>", 9),
  Operator("<<", 9),
  
  Operator("<=", 8),
  Operator("<", 8),
  Operator(">=", 8),
  Operator(">", 8),
  
  Operator("--", 13, ASSOCIATE_FROM_LEFT, UNARY, POSTFIX, unaryOps),
  Operator("++", 13, ASSOCIATE_FROM_LEFT, UNARY, POSTFIX, unaryOps),
  
  Operator(".", 13, ASSOCIATE_FROM_LEFT, BINARY, INFIX, {true, false}),
  
  Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX, unaryOps),
  Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX, unaryOps),
  Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("~", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("!", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  
  Operator("*", 11),
  Operator("/", 11),
  Operator("%", 11),
  
  Operator("+", 10),
  Operator("-", 10),
  
  Operator("&&", 3), // Logical AND
  Operator("||", 2), // Logical OR
  
  Operator("&", 6), // Bitwise AND
  Operator("^", 5), // Bitwise XOR
  Operator("|", 4), // Bitwise OR
  
  Operator(",", 0),
  
  // These don't get matched by the lexer as operators, but their symbols get matched as constructs
  // They are created in the parser
  Operator("()", 13, ASSOCIATE_FROM_LEFT, BINARY, POSTFIX, {true}), // Name of function and argument tree with commas
  Operator("[]", 13, ASSOCIATE_FROM_LEFT, BINARY, CIRCUMFIX, {true, false}),
  Operator("?:", 1, ASSOCIATE_FROM_LEFT, TERNARY, CIRCUMFIX),
  Operator(" ", 0, ASSOCIATE_FROM_LEFT, NULLARY)
};

/**
  \brief Maps simple descriptions of operators to their index in the operatorList.
  Also used in XML files.
*/
static const std::unordered_map<Operator::Name, Operator::Index> operatorNameMap {
  {"Equality", 0},
  {"Inequality", 1},
  {"Assignment", 2},
  {"+Assignment", 3},
  {"-Assignment", 4},
  {"*Assignment", 5},
  {"/Assignment", 6},
  {"%Assignment", 7},
  {"<<Assignment", 8},
  {">>Assignment", 9},
  {"&Assignment", 10},
  {"^Assignment", 11},
  {"|Assignment", 12},
  {"Bitshift >>", 13},
  {"Bitshift <<", 14},
  {"Less or equal", 15},
  {"Less", 16},
  {"Greater or equal", 17},
  {"Greater", 18},
  {"Postfix --", 19},
  {"Postfix ++", 20},
  {"Member access", 21},
  {"Prefix --", 22},
  {"Prefix ++", 23},
  {"Unary -", 24},
  {"Unary +", 25},
  {"Bitwise NOT", 26},
  {"Logical NOT", 27},
  {"Multiply", 28},
  {"Divide", 29},
  {"Modulo", 30},
  {"Add", 31},
  {"Substract", 32},
  {"Logical AND", 33},
  {"Logical OR", 34},
  {"Bitwise AND", 35},
  {"Bitwise XOR", 36},
  {"Bitwise OR", 37},
  {"Comma", 38},
  {"Call", 39},
  {"Subscript", 40},
  {"Conditional", 41},
  {"No-op", 42}
};

/**
  \brief Gets a vector of all chars used in Operator symbols.
  \sa Operator::Symbol
*/
std::vector<char> getOperatorCharacters();
/**
  \brief Gets a vector of all chars used in constructs. This is effectively a constant.
*/
std::vector<char> getConstructCharacters();

/**
  \brief Gets an Operator using its Operator::Name
*/
const Operator& operatorFrom(const Operator::Name& name);
/**
  \brief Gets an Operator::Name using its Operator::Index
*/
Operator::Name operatorNameFrom(Operator::Index index);
/**
  \brief Gets an Operator::Index using its Operator::Name
*/
Operator::Index operatorIndexFrom(const Operator::Name& name);

#endif
