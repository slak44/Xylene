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
  
  uint64_t pos = 0;
  uint64_t currentLine = 1;
  uint64_t currentLinePos = 0;
  
  /// Get character at current position
  inline char current() const {
    return code[pos];
  }
  /**
    \brief Get multiple characters starting at the current position
    \param charCount how many should be retrieved
  */
  inline std::string current(unsigned charCount) const {
    return code.substr(pos, charCount);
  }
  /// Get a character this many indices ahead
  inline char peekAhead(unsigned ahead) const {
    return code[pos + ahead];
  }
  /// Skip a number of chars, usually just advances to the next one
  inline void skip(uint64_t skipped) {
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
  inline Position getPos() const noexcept {
    return Position(currentLine, currentLinePos);
  }
  /// Make a range from the argument to the current position
  inline Range rangeFrom(Position start) const noexcept {
    return Range(start, getPos());
  }
  /// Make a trace from the argument to the current position
  inline Trace traceFrom(Position start) const noexcept {
    return Trace(sourceOfCode, rangeFrom(start));
  }
  /// Make a trace n characters long
  inline Trace traceFor(unsigned n) const noexcept {
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
  
  bool isValidForRadix(char c, unsigned radix) const noexcept;
  /**
    \brief Look for a radix at the current position, eg the '0x' in 0xAAA
    \returns the radix found (2, 16, etc)
  */
  unsigned readRadix();
  /**
    \brief Look for a number at the current position
    \param radix the number's radix
    \returns a token with TT::INTEGER or TT::FLOAT, with its data field in decimal
  */
  Token readNumber(unsigned radix);
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
  
  inline Token operator[](std::size_t at) const noexcept {
    return tokens[at];
  }
  
  /// Get input code
  inline const std::string& getCode() const noexcept {
    return code;
  }
  
  /// Get lexed tokens
  inline std::vector<Token> getTokens() const noexcept {
    return tokens;
  }

  /// Get total line count
  inline uint64_t getLineCount() const noexcept {
    return tokens.empty() ? 0 : currentLine;
  }

  /// Where the code came from
  inline std::string getCodeSource() const noexcept {
    return sourceOfCode;
  }
};

#endif
