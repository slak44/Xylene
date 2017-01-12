#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <stack>
#include <string>
#include <cctype>
#include <unordered_map>

#include "utils/util.hpp"
#include "utils/trace.hpp"
#include "utils/error.hpp"
#include "operator.hpp"
#include "token.hpp"

namespace {
  /**
    \brief Maps the second character in an escape sequence (eg the 'n' in \\n) to the
    actual escaped character.
  */
  const std::unordered_map<char, char> singleCharEscapeSeqences {
    {'a', '\a'},
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'v', '\v'},
    {'\\', '\\'},
    {'\'', '\''},
    {'"', '\"'},
    {'?', '\?'},
  };
}

/// Tokenizes input.
class Lexer {
private:
  std::string code;
  std::string sourceOfCode;
  std::vector<Token> tokens {};
  
  uint64 pos = 0;
  uint64 currentLine = 1;
  uint64 currentLinePos = 0;
  
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
  /// Skip a number of chars, usually just advances to the next one
  inline void skip(uint64 skipped) {
    pos += skipped;
    currentLinePos += skipped;
    if (pos >= code.length()) pos = code.length();
  }
  /// Decrement current position so the loop doesn't increment automatically
  inline void noIncrement() noexcept {
    pos--;
  }
  /// Is end of line
  inline bool isEOL() const noexcept {
    return current() == '\n';
  }
  /// Is end of file
  inline bool isEOF() const noexcept {
    return pos == code.length();
  }
  /// Advance to the next line
  inline void nextLine() noexcept {
    currentLinePos = 0;
    currentLine++;
  }
  /// Get a Position object to the current location
  inline const Position getPos() const {
    return Position(currentLine, currentLinePos);
  }
  /// Make a range from the argument to the current position
  inline const Range rangeFrom(const Position start) const {
    return Range(start, getPos());
  }
  /// Make a trace from the argument to the current position
  inline const Trace traceFrom(const Position start) const {
    return Trace(sourceOfCode, rangeFrom(start));
  }
  /// Make a trace n characters long
  inline const Trace traceFor(uint n) const {
    return Trace(sourceOfCode, Range(getPos(), n));
  }

  inline void handleMultiLineComments();
  
  /// Check if the current character can be part of an identifier
  bool isIdentifierChar() const noexcept;
  /**
    \brief Disambiguate the fixity of the last token added
    \returns one of the arguments, depending on the token in front
  */
  Fixity determineFixity(
    Fixity afterBinaryOrPrefix,
    Fixity afterIdentOrParen,
    Fixity otherCases
  ) const noexcept;
  
  /// Disambiguate between parens and calls, on the current token
  TokenType determineParenBeginType() const noexcept;
  
  bool isValidForRadix(char c, uint radix) const noexcept;
  /**
    \brief Look for a radix at the current position, eg the '0x' in 0xAAA
    \returns the radix found (2, 16, etc)
  */
  uint readRadix();
  /**
    \brief Look for a number at the current position
    \param radix the number's radix
    \returns a token with TT::INTEGER or TT::FLOAT, with its data field in decimal
  */
  Token readNumber(uint radix);
  /**
    \brief Look for an escape sequence (assumes that the \\ was skipped)
    \returns whatever was escaped
  */
  char readEscapeSeq();
protected:
  /// Put the actual lexical analysis in here, not in tokenize
  void processTokens();
  
  Lexer(std::string code, std::string sourceOfCode);
  Lexer(const Lexer&) = default;
public:
  /**
    \brief Call to create token list from code
    \param code thing to tokenize
    \param sourceOfCode where the code came from, used for error messages
  */
  static std::unique_ptr<Lexer> tokenize(std::string code, std::string sourceOfCode);
  
  Token operator[](std::size_t) const;
  
  /// Get input code
  const std::string& getCode() const noexcept;
  /// Get output tokens
  const std::vector<Token>& getTokens() const noexcept;
  /// Get total line count
  uint64 getLineCount() const noexcept;
  /// Get the where the code is from
  std::string getCodeSource() const noexcept;
};

#endif
