#include "operators.hpp"

namespace lang {
  
  Operator::Operator(std::string opName, int precedence, Associativity asc, Arity ar):
    op(opName),
    precedence(precedence),
    asc(asc),
    ar(ar) {
    
  }
  
  Operator::Operator(std::string opName, int precedence, Arity ar): Operator(opName, precedence, ASSOCIATE_FROM_LEFT, ar) {}
  
  std::string Operator::getName() const {
    return op;
  }
  std::string Operator::getName() {
    return op;
  }
  
  Arity Operator::getArity() const {
    return ar;
  }
  Arity Operator::getArity() {
    return ar;
  }
  
  Associativity Operator::getAssociativity() const {
    return asc;
  }
  Associativity Operator::getAssociativity() {
    return asc;
  }
  
  int Operator::getPrecedence() const {
    return precedence;
  }
  int Operator::getPrecedence() {
    return precedence;
  }
  
  bool Operator::operator==(const Operator& right) const {
    return
    this->precedence == right.precedence &&
    this->ar == right.ar &&
    this->asc == right.asc &&
    this->op == right.op;
  }
  bool Operator::operator!=(const Operator& right) const {return !operator==(right);}
  
  std::vector<Operator> opList {
    Operator(">>", 9),
    Operator("<<", 9),
    
    Operator("<=", 8),
    Operator("<", 8),
    Operator(">=", 8),
    Operator(">", 8),
    
    Operator("==", 7),
    Operator("!=", 7),
    
    // TODO: Ternary "?:"
    Operator("=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("+=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("-=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("*=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("/=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("%=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("<<=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator(">>=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("&=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("^=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    Operator("|=", 1, ASSOCIATE_FROM_RIGHT, BINARY),
    
    // Postfix
    Operator("--", 13, ASSOCIATE_FROM_LEFT, UNARY),
    Operator("++", 13, ASSOCIATE_FROM_LEFT, UNARY),
    //  Operator("()", 13), // TODO: find function calls somehow
    //  Operator("[]", 13), // TODO: find subscript better, treat as macro and preprocess maybe?
    Operator(".", 13),
    
    // Prefix
    Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY),
    Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY),
    Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY),
    Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY),
    Operator("~", 12, ASSOCIATE_FROM_RIGHT, UNARY), // Bitwise NOT
    Operator("!", 12, ASSOCIATE_FROM_RIGHT, UNARY), // Logical NOT
    
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
    
    Operator(",", 0)
  };
  
  std::vector<char> getOperatorCharacters() {
    static std::vector<char> chars {};
    if (chars.size() == 0) {
      std::string mashed = "";
      std::for_each(opList.begin(), opList.end(), [&mashed](Operator op) {
        mashed += op.getName();
      });
      std::sort(mashed.begin(), mashed.end());
      mashed.erase(std::unique(mashed.begin(), mashed.end()), mashed.end());
      for (auto c : mashed) chars.push_back(c);
    }
    return chars;
  }
  
  /*
  * Checks if the given char is part of an operator or construct.
  * eg if the char is '=' or ';' this returns true, if the char is 'a' this returns false.
  */
  bool isReservedChar(char& c) {
    auto opChars = getOperatorCharacters();
    for (auto i : opChars) {
      if (c == i) return true;
    }
    std::vector<char> constrChars {
      '{', '}',
      ';', ':',
      '(', ')',
      '?'
    };
    if (std::find(constrChars.begin(), constrChars.end(), c) != constrChars.end()) {
      return true;
    }
    return false;
  }
  
} /* namespace lang */
