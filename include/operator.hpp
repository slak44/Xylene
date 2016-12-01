#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <string>
#include <vector>
#include <unordered_set>

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
  \brief Represents where this operator is placed with respect to his operands.
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
  \sa Associativity, Arity, Fixity, Operator::list
*/
class Operator final {
public:
  /// The symbol used for the operator, eg '+', '=', '&&'
  using Symbol = std::string;
  /// Descriptive name for the operator
  using Name = std::string;
  /// Where in the Operator::list below it is
  using Index = int64;
  /// Require that the operand at that index can be mutated if true
  using RequireReferenceList = std::vector<bool>;
protected: // to dodge false-positive compiler warning, class is final anyway
  Symbol name;
  int precedence;
  Name descName;
  Associativity associativity;
  Arity arity;
  Fixity fixity;
  RequireReferenceList refList;
  
  /**
    \brief Creates an operator. Only the Name, Symbol and precedence are mandatory.
  */
  Operator(
    Symbol name,
    int precedence,
    Name descName,
    Associativity associativity = ASSOCIATE_FROM_LEFT,
    Arity arity = BINARY,
    Fixity fixity = INFIX,
    RequireReferenceList refList = {false, false, false}
  );
public:
  Symbol getName() const;
  int getPrecedence() const;
  Name getDescName() const;
  Associativity getAssociativity() const;
  Arity getArity() const;
  Fixity getFixity() const;
  RequireReferenceList getRefList() const;
  
  bool operator==(const Operator& rhs) const;
  bool operator!=(const Operator& rhs) const;
  
  /// Complete Operator list. All instances of this class can be found here
  static const std::vector<Operator> list;
  /// Set of characters used in Operator::Symbol
  static const std::unordered_set<char> operatorCharacters;
  
  /// Find an Operator::Index from an Operator::Name
  static Operator::Index find(Name descName);
};

#endif
