#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <numeric>

#include "utils/util.hpp"
#include "utils/error.hpp"

/// Represents how an operator associates.
enum Associativity: int {
  /// The default. Operators associate from left to right.
  ASSOCIATE_FROM_LEFT,
  /// Operators associate from right to left.
  ASSOCIATE_FROM_RIGHT
};

/// Represents the amount of operands this operator has.
enum Arity: int {
  NULLARY = 0,
  UNARY = 1,
  BINARY = 2, ///< The default
  TERNARY = 3,
  POLYADIC = 9999 // 2+ operands
};

/// Represents where this operator is placed with respect to his operands.
enum Fixity: int {
  INFIX = 0, ///< The default
  PREFIX = -1,
  POSTFIX = 1,
  CIRCUMFIX = 2
};

/**
  \brief Class that represents an operator. Cannot be instantiated.
  \sa Associativity, Arity, Fixity, Operator::list
*/
class Operator final {
public:
  /// The symbol used for the operator, eg '+', '=', '&&'
  using Symbol = std::string;
  /// Descriptive name for the operator, eg 'Equality', 'Logical AND'
  using Name = std::string;
  /// Where in the Operator::list below it is
  using Index = std::size_t;
  /// If true, require that the operand at a specific index in this list can be mutated
  using RequireReferenceList = std::vector<bool>;
private:
  /// \copydoc Symbol
  Symbol symbol;
  /// Operator precedence in expressions
  int precedence;
  /// \copydoc Name
  Name name;
  /// \copydoc Associativity
  Associativity associativity;
  /// \copydoc Arity
  Arity arity;
  /// \copydoc Fixity
  Fixity fixity;
  /// \copydoc RequireReferenceList
  RequireReferenceList refList;
  
  /// Creates an operator. Only the Name, Symbol and precedence are mandatory.
  Operator(
    Symbol symbol,
    int precedence,
    Name name,
    Associativity associativity = ASSOCIATE_FROM_LEFT,
    Arity arity = BINARY,
    Fixity fixity = INFIX,
    RequireReferenceList refList = {false, false, false}
  );
public:
  inline Symbol getSymbol() const noexcept {return symbol;}
  inline int getPrec() const noexcept {return precedence;}
  inline Name getName() const noexcept {return name;}
  inline Associativity getAssociativity() const noexcept {return associativity;}
  inline Arity getArity() const noexcept {return arity;}
  inline Fixity getFixity() const noexcept {return fixity;}
  inline RequireReferenceList getRefList() const noexcept {return refList;}

  inline bool hasSymbol(Symbol s) const noexcept {return s == symbol;}
  inline bool hasPrec(int p) const noexcept {return p == precedence;}
  inline bool hasName(Name n) const noexcept {return n == name;}
  inline bool hasAsoc(Associativity a) const noexcept {return a == associativity;}
  inline bool hasArity(Arity a) const noexcept {return a == arity;}
  inline bool hasFixity(Fixity f) const noexcept {return f == fixity;}
  
  bool operator==(const Operator& rhs) const noexcept;
  bool operator!=(const Operator& rhs) const noexcept;
  
  /**
    \brief Complete Operator list. All instances of this class can be found here
    
    This container must be ordered.
  */ 
  static const std::vector<Operator> list;
  /// Set of characters used in Operator::Symbol
  static const std::unordered_set<char> operatorCharacters;
  
  /// Find an Operator::Index from an Operator::Name
  static Index find(Name name);
};

#endif
