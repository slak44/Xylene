#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <string>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "operator.hpp"
#include "token.hpp"

class LexerBase {
private:
  std::string code;
  uint64 pos = 0;
  std::vector<Token> tokens {};
  uint64 currentLine = 1;
protected:
  // Data access
  inline char current() const {
    return code[pos];
  }
  inline std::string current(uint charCount) const {
    return code.substr(pos, charCount);
  }
  inline char peekAhead(uint ahead) const {
    return code[pos + ahead];
  }
  inline const Token& peekBehind() const {
    return tokens.back();
  }
  // Position
  inline void skip(uint64 skipped) {
    pos += skipped;
    if (pos >= code.length()) pos = code.length();
  }
  inline void noIncrement() {
    pos--;
  }
  inline bool hasFinishedString() const {
    return pos == code.length();
  }
  // Special chars
  inline bool isEOL() const {
    return current() == '\n';
  }
  inline bool isEOF() const {
    return current() == '\0';
  }
  // Tokens
  inline void addToken(Token tok) {
    tokens.push_back(tok);
  }
  inline std::size_t getTokenCount() const {
    return tokens.size();
  }
  // Line number
  inline void nextLine() {
    currentLine++;
  }
  inline uint64 getCurrentLine() const {
    return currentLine;
  }
  // Loop over the string here in subclasses
  virtual void processInput() = 0;
public:
  const std::string& getCode() const;
  const std::vector<Token>& getTokens() const;
  const Token& operator[](std::size_t p) const;
  uint64 getLineCount() const;
  
  virtual LexerBase& tokenize(std::string code);
};

class Lexer: public LexerBase {
private:
  inline TokenType findConstruct() const;
  inline bool isOctalDigit(char c) const;
  inline bool isIdentifierChar() const;
  inline Fixity determineFixity(Fixity afterBinaryOrPrefix, Fixity afterIdentOrParen, Fixity otherCases) const;
  
  inline void handleMultiLineComments();
  inline int getNumberRadix();
  inline Token getNumberToken(int radix);
  inline std::string getQuotedString();
protected:
  void processInput();
public:
  Lexer() {}
};

inline void Lexer::handleMultiLineComments() {
  // TODO nested comments
  if (current(2) == "/*") {
    skip(2); // Skip "/*"
    while (current(2) != "*/") {
      if (isEOF()) throw Error("SyntaxError", "Multi-line comment not closed", getCurrentLine());
      else if (isEOL()) nextLine();
      skip(1); // Skip characters one by one until we hit the end of the comment
    }
    skip(2); // Skip "*/"
  }
}

inline TokenType Lexer::findConstruct() const {
  try {
    return constructMap.at(current());
  } catch (std::out_of_range& oor) {
    // Is not a construct
    return UNPROCESSED;
  }
}

inline bool Lexer::isOctalDigit(char c) const {
  return c >= '0' && c <= '7';
}

inline std::string Lexer::getQuotedString() {
  std::string str = "";
  while (current() != '"') {
    if (isEOF()) throw Error("SyntaxError", "String literal has unmatched quote", getCurrentLine());
    if (current() == '\\') {
      char escapedChar = peekAhead(1);
      skip(2); // Skip escape code, eg '\n', '\t'
      try {
        escapedChar = singleCharEscapeSeqences.at(escapedChar);
        str += escapedChar;
        continue;
      } catch (std::out_of_range& oor) {
        auto badEscape = Error("SyntaxError", "Invalid escape code", getCurrentLine());
        // \x00 hex escape code
        if (escapedChar == 'x') {
          std::string hexNumber = "";
          if (std::isxdigit(current())) hexNumber += current();
          else throw badEscape;
          if (std::isxdigit(peekAhead(1))) hexNumber += peekAhead(1);
          else throw badEscape;
          skip(2);
          str += static_cast<char>(std::stoll(hexNumber, 0, 16));
          continue;
        // \00 octal escape code
        } else if (isdigit(escapedChar)) {
          std::string octalNumber = "";
          if (isOctalDigit(escapedChar)) octalNumber += escapedChar;
          else throw badEscape;
          if (isOctalDigit(current())) octalNumber += current();
          else throw badEscape;
          if (isOctalDigit(peekAhead(1))) octalNumber += peekAhead(1);
          else throw badEscape;
          skip(3);
          str += static_cast<char>(std::stoll(octalNumber, 0, 8));
          continue;
        }
        throw badEscape;
      }
    }
    str += current();
    skip(1);
  }
  return str;
}

inline bool Lexer::isIdentifierChar() const {
  bool isOperatorChar = contains(getOperatorCharacters(), current());
  bool isConstructChar = contains(getConstructCharacters(), current());
  return !isOperatorChar && !isConstructChar;
}

inline Fixity Lexer::determineFixity(Fixity afterBinaryOrPrefix, Fixity afterIdentOrParen, Fixity otherCases) const {
  if (getTokenCount() == 0) return otherCases;
  Fixity type;
  if (peekBehind().isOperator() && (peekBehind().hasArity(BINARY) || peekBehind().hasFixity(PREFIX))) {
    // If the thing before this op is a binary op or a prefix unary, it means this must be a prefix unary
    type = afterBinaryOrPrefix;
  } else if (peekBehind().isTerminal() || peekBehind().type == C_PAREN_RIGHT || (peekBehind().isOperator() && peekBehind().hasFixity(POSTFIX))) {
    // If the thing before was an identifier/paren (or the postfix op attached to an ident), then this must be a postfix unary or a binary infix
    type = afterIdentOrParen;
  } else {
    type = otherCases;
  }
  return type;
}

inline int Lexer::getNumberRadix() {
  if (current() == '0' && isalpha(peekAhead(1))) {
    auto radixIdent = peekAhead(1);
    skip(2); // Skip the "0x", etc
    switch (radixIdent) {
      case 'x': return 16;
      case 'b': return 2;
      case 'o': return 8;
      default: throw Error("SyntaxError", "Invalid radix", getCurrentLine());
    }
  } else {
    return 10;
  }
}

inline Token Lexer::getNumberToken(int radix) {
  if (current() == '0' && isdigit(peekAhead(1))) throw Error("SyntaxError", "Numbers cannot begin with '0'", getCurrentLine());
  std::string number = "";
  bool isFloat = false;
  while (!isEOF()) {
    if (current() == '.') {
      if (isFloat) throw Error("SyntaxError", "Malformed float, multiple decimal points", getCurrentLine());
      isFloat = true;
      number += current();
      skip(1);
    } else if (isdigit(current())) {
      number += current();
      skip(1);
    } else if (
        (current() >= 'A' && current() < 'A' + radix - 10) ||
        (current() >= 'a' && current() < 'a' + radix - 10)
      ) {
      number += current();
      skip(1);
    } else {
      break;
    }
  }
  noIncrement();
  if (number.back() == '.') throw Error("SyntaxError", "Malformed float, missing digits after decimal point", getCurrentLine());
  if (radix != 10 && isFloat) throw Error("SyntaxError", "Floating point numbers must be used with base 10 numbers", getCurrentLine());
  if (radix != 10) number = std::to_string(std::stoll(number, 0, radix));
  return Token(isFloat ? L_FLOAT : L_INTEGER, number, getCurrentLine());
}

#endif
