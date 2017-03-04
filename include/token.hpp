#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include <array>
#include <algorithm>

#include "utils/util.hpp"
#include "utils/trace.hpp"
#include "utils/error.hpp"
#include "operator.hpp"

#ifdef _MSC_VER
  #undef FILE_END
  #undef VOID
#endif

class TokenType {
private:
  int index;
protected:
  const char* prettyName;
public:
  constexpr explicit TokenType(int idx, const char* prettyName):
    index(idx), prettyName(prettyName) {}

  constexpr operator int() const {return index;}
  constexpr bool operator==(const TokenType& rhs) const {return index == rhs.index;}
  constexpr bool operator!=(const TokenType& rhs) const {return !operator==(rhs);}
  
  std::string toString() const;
};

/**
  TT stands for TokenType, because a namespace can't have the same name as a class
  These identifiers inside are all caps because they are pretending to be enum values
*/
namespace TT {
  class Literal: public TokenType {
  public:
    constexpr explicit Literal(int idx, const char* prettyName):
      TokenType(idx, prettyName) {}
  };

  class Keyword: public TokenType {
  public:
    constexpr explicit Keyword(int idx, const char* data): TokenType(idx, data) {}
    
    std::string getKeyword() const;
  };

  class Construct: public TokenType {
  private:
    char data;
  public:
    constexpr explicit Construct(char c, int idx, const char* prettyName):
      TokenType(idx, prettyName), data(c) {}

    char getData() const;
  };
  
  constexpr Literal INTEGER(100, "Integer");
  constexpr Literal FLOAT(101, "Float");
  constexpr Literal BOOLEAN(102, "Boolean");
  constexpr Literal STRING(103, "String");

  constexpr Keyword DEFINE(200, "define");
  constexpr Keyword AS(201, "as");
  constexpr Keyword IMPORT(202, "import");
  constexpr Keyword EXPORT(203, "export");
  constexpr Keyword ALL_OF(204, "all");
  constexpr Keyword FROM(205, "from");
  constexpr Keyword FUNCTION(206, "function");
  constexpr Keyword RETURN(207, "return");
  constexpr Keyword DO(208, "do");
  constexpr Keyword END(209, "end");
  constexpr Keyword IF(210, "if");
  constexpr Keyword ELSE(211, "else");
  constexpr Keyword WHILE(212, "while");
  constexpr Keyword FOR(213, "for");
  constexpr Keyword BREAK(214, "break");
  constexpr Keyword CONTINUE(215, "continue");
  constexpr Keyword FAT_ARROW(216, "=>");
  constexpr Keyword VOID(217, "void");
  constexpr Keyword FOREIGN(218, "foreign");
  constexpr Keyword TYPE(219, "type");
  constexpr Keyword INHERITS(220, "inherits");
  constexpr Keyword CONSTR(221, "constructor");
  constexpr Keyword METHOD(222, "method");
  constexpr Keyword PUBLIC(223, "public");
  constexpr Keyword PRIVATE(224, "private");
  constexpr Keyword PROTECT(225, "protected");
  constexpr Keyword STATIC(226, "static");
  constexpr Keyword THROW(227, "throw");
  constexpr Keyword TRY(228, "try");
  constexpr Keyword CATCH(229, "catch");

  constexpr Construct SEMI(';', 300, "Semicolon");
  constexpr Construct TWO_POINT(':', 301, "Colon");
  constexpr Construct QUESTION('?', 302, "Question mark");
  constexpr Construct PAREN_LEFT('(', 303, "Left parenthesis");
  constexpr Construct PAREN_RIGHT(')', 304, "Right parenthesis");
  constexpr Construct SQPAREN_LEFT('[', 305, "Left square parenthesis");
  constexpr Construct SQPAREN_RIGHT(']', 306, "Right square parenthesis");
  constexpr Construct CALL_BEGIN('(', 307, "Call begin");
  constexpr Construct CALL_END(')', 308, "Call end");

  constexpr TokenType FILE_END(999, "End of file");
  constexpr TokenType OPERATOR(1, "Operator");
  constexpr TokenType IDENTIFIER(5, "Identifier");
  constexpr TokenType UNPROCESSED(0, "Unprocessed?");
  
  constexpr std::array<Keyword, 30> keywords = {
    DEFINE,
    AS, IMPORT, EXPORT, ALL_OF, FROM,
    FUNCTION, RETURN,
    DO, END,
    IF, ELSE,
    WHILE, FOR, BREAK, CONTINUE,
    FAT_ARROW, VOID, FOREIGN,
    TYPE, INHERITS, CONSTR, METHOD,
    PUBLIC, PRIVATE, PROTECT, STATIC,
    THROW, TRY, CATCH
  };
  
  constexpr std::array<Construct, 7> constructs = {
    SEMI, TWO_POINT, QUESTION,
    PAREN_LEFT, PAREN_RIGHT,
    SQPAREN_LEFT, SQPAREN_RIGHT
  };
  
  constexpr std::array<TokenType, 45> all = {
    INTEGER, FLOAT, BOOLEAN, STRING,
    
    DEFINE,
    AS, IMPORT, EXPORT, ALL_OF, FROM,
    FUNCTION, RETURN,
    DO, END,
    IF, ELSE,
    WHILE, FOR, BREAK, CONTINUE,
    FAT_ARROW, VOID, FOREIGN,
    TYPE, INHERITS, CONSTR, METHOD,
    PUBLIC, PRIVATE, PROTECT, STATIC,
    THROW, TRY, CATCH,
    
    SEMI, TWO_POINT, QUESTION,
    PAREN_LEFT, PAREN_RIGHT,
    SQPAREN_LEFT, SQPAREN_RIGHT,
    
    FILE_END,
    OPERATOR,
    IDENTIFIER,
    UNPROCESSED
  };
  
  TokenType findByPrettyName(std::string);
  TokenType findConstruct(char);
  TokenType findKeyword(std::string);
}

/**
  \brief Holds a token encountered by the lexer.
*/
class Token {
public:
  TokenType type; ///< \see TokenType
  std::string data = ""; ///< Stores the data that represents this token. May be processed
  Operator::Index idx = 9999; ///< Only if the type is OPERATOR
  Trace trace; ///< At what line of the input was this Token encountered
  
  /// Create a non-operator Token.
  Token(TokenType type, std::string data, Trace trace);
  /// Create an operator Token.
  Token(TokenType type, Operator::Index idx, Trace trace);
  /// Create a Token without initializing any data.
  Token(TokenType type, Trace trace);
  
  /// Matches identifiers and literals
  bool isTerminal() const;
  /// Matches operators
  bool isOp() const;
  
  /// Get the stored operator
  Operator op() const;
    
  bool operator==(const Token& tok) const;
  bool operator!=(const Token& tok) const;
  
  std::string toString() const;
};

inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
  return os << tok.toString();
}

inline std::ostream& operator<<(std::ostream& os, const TokenType& tok) {
  return os << tok.toString();
}

#endif
