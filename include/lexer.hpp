#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <string>
#include <cctype>

#include "utils/util.hpp"
#include "utils/trace.hpp"
#include "utils/error.hpp"
#include "operator.hpp"
#include "token.hpp"

/**
  \brief Base of Lexer. Maintains all of its state, and provides convenience methods for manipulating it
*/
class LexerBase {
private:
  std::string fileName;
  std::string code;
  uint64 pos = 0;
  std::vector<Token> tokens {};
  uint64 currentLine = 1;
  uint64 currentLinePos = 0;
protected:
  /// Get character at current position
  inline char current() const {
    return code[pos];
  }
  /**
    \brief Get multiple characters starting at the current position
    \param charCount how many should be retrieved
  */
  inline std::string current(uint charCount) const {
    return code.substr(pos, charCount);
  }
  /// Get a character this many indices ahead
  inline char peekAhead(uint ahead) const {
    return code[pos + ahead];
  }
  /// Get the last inserted Token
  inline const Token& peekBehind() const {
    return tokens.back();
  }
  /// Skip a number of chars, usually just advances to the next one
  inline void skip(uint64 skipped) {
    pos += skipped;
    currentLinePos += skipped;
    if (pos >= code.length()) pos = code.length();
  }
  /// Decrement current position so the loop doesn't increment automatically
  inline void noIncrement() {
    pos--;
  }
  /// Is the input finished
  inline bool hasFinishedString() const {
    return pos == code.length();
  }
  /// Is end of line
  inline bool isEOL() const {
    return current() == '\n';
  }
  /// Is end of file
  inline bool isEOF() const {
    return current() == '\0';
  }
  /// Add a new Token to the output
  inline void addToken(Token tok) {
    tokens.push_back(tok);
  }
  /// Get count of tokens
  inline std::size_t getTokenCount() const {
    return tokens.size();
  }
  /// Advance to the next line
  inline void nextLine() {
    currentLinePos = 0;
    currentLine++;
  }
  /// Get the current line number
  inline uint64 getCurrentLine() const {
    return currentLine;
  }
  /// Get a Position object
  inline const Position getCurrentPosition() const {
    return Position(currentLine, currentLinePos);
  }
  /// Make a range from the argument to the current position
  inline const Range getRangeToHere(const Position start) const {
    return Range(start, getCurrentPosition());
  }
  /// Get the current file name
  inline const std::string& getFileName() const {
    return fileName;
  }
  /**
    \brief Subclasses should use this to loop over the input and create Tokens from it
  */
  virtual void processInput() = 0;
public:
  /// Get input code
  const std::string& getCode() const;
  /// Get output tokens
  const std::vector<Token>& getTokens() const;
  /// Access an output token at a particular position
  const Token& operator[](std::size_t p) const;
  /// Get total line count
  uint64 getLineCount() const;
  
  /// Call to create output token list
  LexerBase& tokenize(std::string code, std::string fileName);
};

/**
  \brief Tokenizes input.
*/
class Lexer: public LexerBase {
private:
  /// Utility function
  inline TokenType findConstruct() const;
  /// \copydoc findConstruct
  inline bool isOctalDigit(char c) const;
  /// \copydoc findConstruct
  inline bool isIdentifierChar() const;
  /// \copydoc findConstruct
  inline Fixity determineFixity(Fixity afterBinaryOrPrefix, Fixity afterIdentOrParen, Fixity otherCases) const;
  
  /// \copydoc findConstruct
  inline void handleMultiLineComments();
  /// \copydoc findConstruct
  inline int getNumberRadix();
  /// \copydoc findConstruct
  inline Token getNumberToken(uint radix);
  /// \copydoc findConstruct
  inline std::string getQuotedString();
protected:
  void processInput();
public:
  Lexer() {}
};

inline void Lexer::handleMultiLineComments() {
  // TODO nested comments
  Position start = getCurrentPosition();
  if (current(2) == "/*") {
    skip(2); // Skip "/*"
    while (current(2) != "*/") {
      if (isEOF()) throw Error("SyntaxError", "Multi-line comment not closed", Trace(getFileName(), getRangeToHere(start)));
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
  Position start = getCurrentPosition();
  while (current() != '"') {
    if (isEOF()) throw Error("SyntaxError", "String literal has unmatched quote", Trace(getFileName(), getRangeToHere(start)));
    if (current() == '\\') {
      Position escCharPos = getCurrentPosition();
      char escapedChar = peekAhead(1);
      skip(2); // Skip escape code, eg '\n', '\t'
      try {
        escapedChar = singleCharEscapeSeqences.at(escapedChar);
        str += escapedChar;
        continue;
      } catch (std::out_of_range& oor) {
        auto badEscape = Error("SyntaxError", "Invalid escape code", Trace(getFileName(), getRangeToHere(escCharPos)));
        // \x00 hex escape code
        if (escapedChar == 'x') {
          std::string hexNumber = "";
          if (std::isxdigit(current())) hexNumber += current();
          else throw badEscape;
          if (std::isxdigit(peekAhead(1))) hexNumber += peekAhead(1);
          else throw badEscape;
          skip(2);
          str += static_cast<char>(std::stoll(hexNumber, nullptr, 16));
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
          str += static_cast<char>(std::stoll(octalNumber, nullptr, 8));
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
  bool isOperatorChar =
    Operator::operatorCharacters.find(current()) != Operator::operatorCharacters.end();
  bool isConstructChar = contains({
    ' ', '\n', '\0', '(', ')', '[', ']', '?', ';', ':'
  }, current());
  return !isOperatorChar && !isConstructChar;
}

inline Fixity Lexer::determineFixity(Fixity afterBinaryOrPrefix, Fixity afterIdentOrParen, Fixity otherCases) const {
  if (getTokenCount() == 0) return otherCases;
  Fixity type;
  if (peekBehind().isOp() && (peekBehind().op().hasArity(BINARY) || peekBehind().op().hasFixity(PREFIX))) {
    // If the thing before this op is a binary op or a prefix unary, it means this must be a prefix unary
    type = afterBinaryOrPrefix;
  } else if (peekBehind().isTerminal() || peekBehind().type == C_PAREN_RIGHT || (peekBehind().isOp() && peekBehind().op().hasFixity(POSTFIX))) {
    // If the thing before was an identifier/paren (or the postfix op attached to an ident), then this must be a postfix unary or a binary infix
    type = afterIdentOrParen;
  } else {
    type = otherCases;
  }
  return type;
}

inline int Lexer::getNumberRadix() {
  if (current() == '0' && isalpha(peekAhead(1))) {
    Position zeroPos = getCurrentPosition();
    auto radixIdent = peekAhead(1);
    skip(2); // Skip the "0x", etc
    switch (radixIdent) {
      case 'x': return 16;
      case 'b': return 2;
      case 'o': return 8;
      default: throw Error("SyntaxError", "Invalid radix", Trace(getFileName(), getRangeToHere(zeroPos)));
    }
  } else {
    return 10;
  }
}

inline Token Lexer::getNumberToken(uint radix) {
  Position start = getCurrentPosition();
  if (current() == '0' && isdigit(peekAhead(1)))
    throw Error("SyntaxError", "Numbers cannot begin with '0'", Trace(getFileName(), Range(start, 1)));
  std::string number = "";
  bool isFloat = false;
  while (!isEOF()) {
    if (current() == '.') {
      if (isFloat) throw Error("SyntaxError", "Malformed float, multiple decimal points", Trace(getFileName(), getRangeToHere(start)));
      isFloat = true;
      number += current();
      skip(1);
    } else if (isdigit(current())) {
      number += current();
      skip(1);
    } else if (
        (current() >= 'A' && static_cast<uint>(current()) < 'A' + radix - 10) ||
        (current() >= 'a' && static_cast<uint>(current()) < 'a' + radix - 10)
      ) {
      number += current();
      skip(1);
    } else {
      break;
    }
  }
  noIncrement();
  if (number.back() == '.') throw Error("SyntaxError", "Malformed float, missing digits after decimal point", Trace(getFileName(), getRangeToHere(start)));
  if (radix != 10 && isFloat) throw Error("SyntaxError", "Floating point numbers must be used with base 10 numbers", Trace(getFileName(), getRangeToHere(start)));
  if (radix != 10) number = std::to_string(std::stoll(number, nullptr, radix));
  return Token(isFloat ? L_FLOAT : L_INTEGER, number, Trace(getFileName(), getRangeToHere(start)));
}

#endif
