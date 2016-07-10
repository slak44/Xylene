#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <string>

#include "utils/error.hpp"
#include "token.hpp"
#include "operator.hpp"
#include "utils/util.hpp"

class Lexer {
private:
  std::string code;
  uint64 pos = 0;
  std::vector<Token> tokens {};
  uint64 currentLine = 1;
  
  inline void skip(uint64 skipped) {
    pos += skipped;
    if (pos >= code.length()) pos = code.length();
  }
  inline void noIncrement() {
    pos--;
  }
  inline bool isEOL() {
    return code[pos] == '\n';
  }
  inline bool isEOF() {
    return code[pos] == '\0';
  }
  inline void addToken(Token tok) {
    tokens.push_back(tok);
  }
  inline void handleMultiLineComments() {
    if (code.substr(pos, 2) == "/*") {
      skip(2); // Skip "/*"
      while (code.substr(pos, 2) != "*/") {
        if (isEOF()) {
          throw Error("SyntaxError", "Multi-line comment not closed", currentLine);
        } else if (isEOL()) {
          currentLine++;
        }
        skip(1); // Skip characters one by one until we hit the end of the comment
      }
      skip(2); // Skip "*/"
    }
  }
  inline TokenType findConstruct() const {
    try {
      return constructMap.at(code[pos]);
    } catch (std::out_of_range& oor) {
      // Is not a construct
      return UNPROCESSED;
    }
  }
  inline int getNumberRadix() {
    if (code[pos] == '0' && !isdigit(code[pos + 1])) {
      auto radixIdent = code[pos + 1];
      skip(2); // Skip the "0x", etc
      switch (radixIdent) {
        case 'x': return 16;
        case 'b': return 2;
        case 'o': return 8;
        default: throw Error("SyntaxError", "Invalid radix", currentLine);
      }
    } else {
      return 10;
    }
  }
  inline Token getNumberToken(int radix) {
    if (code[pos] == '0') throw Error("SyntaxError", "Numbers cannot begin with '0'", currentLine);
    std::string number = "";
    bool isFloat = false;
    while (!isEOF()) {
      if (code[pos] == '.') {
        if (isFloat) throw Error("SyntaxError", "Malformed float, multiple decimal points", currentLine);
        isFloat = true;
        number += code[pos];
        skip(1);
      } else if (isdigit(code[pos])) {
        number += code[pos];
        skip(1);
      } else if (
          (code[pos] >= 'A' && code[pos] < 'A' + radix - 10) ||
          (code[pos] >= 'a' && code[pos] < 'a' + radix - 10)
        ) {
        number += code[pos];
        skip(1);
      } else {
        break;
      }
    }
    noIncrement();
    if (number.back() == '.') throw Error("SyntaxError", "Malformed float, missing digits after decimal point", currentLine);
    if (radix != 10 && isFloat) throw Error("SyntaxError", "Floating point numbers must be used with base 10 numbers", currentLine);
    if (radix != 10) number = std::to_string(std::stoll(number, 0, radix));
    return Token(isFloat ? L_FLOAT : L_INTEGER, number, currentLine);
  }
  inline bool isOctal(char c) {
    int val = c - '0';
    return val > 0 && val < '8';
  }
  inline bool isHexadecimal(char c) {
    int val = c - '0';
    int letter = c - 'A';
    int smallLetter = c - 'a';
    return
      (val >= 0 && val < '8') ||
      (letter >= 0 && letter < 'G') ||
      (smallLetter >= 0 && smallLetter < 'g');
  }
  inline std::string getQuotedString() {
    std::string str = "";
    while (code[pos] != '"') {
      if (isEOF()) throw Error("SyntaxError", "String literal has unmatched quote", currentLine);
      if (code[pos] == '\\') {
        skip(1);
        char escapedChar = code[pos];
        skip(1);
        try {
          escapedChar = singleCharEscapeSeqences.at(escapedChar);
          str += escapedChar;
          continue;
        } catch (std::out_of_range& oor) {
          auto badEscape = Error("SyntaxError", "Invalid escape code", currentLine);
          // This error means it wasn't found in the escape sequence map
          if (escapedChar == 'x') {
            std::string hexNumber = "";
            if (isHexadecimal(code[pos])) hexNumber += code[pos];
            else throw badEscape;
            if (isHexadecimal(code[pos + 1])) hexNumber += code[pos + 1];
            else throw badEscape;
            skip(2);
            str += static_cast<char>(std::stoll(hexNumber, 0, 16));
            continue;
          } else if (isdigit(escapedChar)) {
            std::string octalNumber = "";
            if (isOctal(escapedChar)) octalNumber += escapedChar;
            else throw badEscape;
            if (isOctal(code[pos])) octalNumber += code[pos];
            else throw badEscape;
            if (isOctal(code[pos + 1])) octalNumber += code[pos + 1];
            else throw badEscape;
            skip(3);
            str += static_cast<char>(std::stoll(octalNumber, 0, 8));
            continue;
          }
          throw badEscape;
        }
      }
      str += code[pos];
      skip(1);
    }
    return str;
  }
  inline bool isIdentifierChar() {
    bool isOperatorChar = contains(getOperatorCharacters(), code[pos]);
    bool isConstructChar = contains(getConstructCharacters(), code[pos]);
    return !isOperatorChar && !isConstructChar;
  }
  inline Fixity determineFixity(Fixity afterBinaryOrPrefix, Fixity afterIdentOrParen, Fixity otherCases) {
    if (tokens.size() == 0) return otherCases;
    Fixity type;
    if (tokens.back().isOperator() && (tokens.back().hasArity(BINARY) || tokens.back().hasFixity(PREFIX))) {
      // If the thing before this op is a binary op or a prefix unary, it means this must be a prefix unary
      type = afterBinaryOrPrefix;
    } else if (tokens.back().isTerminal() || tokens.back().type == C_PAREN_RIGHT || (tokens.back().isOperator() && tokens.back().hasFixity(POSTFIX))) {
      // If the thing before was an identifier/paren (or the postfix op attached to an ident), then this must be a postfix unary or a binary infix
      type = afterIdentOrParen;
    } else {
      type = otherCases;
    }
    return type;
  }
public:
  Lexer() {}
  
  Lexer& tokenize(std::string code) {
    this->code = code;
    pos = 0;
    tokens = {};
    currentLine = 1;
    for (; pos < code.length(); pos++) {
      // Comments
      if (code.substr(pos, 2) == "//")
        while (!isEOL() && !isEOF()) skip(1);
      handleMultiLineComments();
      // Count lines
      if (isEOL()) currentLine++;
      // Ignore whitespace
      if (isspace(code[pos])) continue;
      // Check for number literals
      if (isdigit(code[pos])) {
        auto radix = getNumberRadix();
        addToken(getNumberToken(radix));
        continue;
      }
      // Check for string literals
      if (code[pos] == '"') {
        skip(1); // Skip the quote
        addToken(Token(L_STRING, getQuotedString(), currentLine));
        continue;
      }
      // Check for constructs
      TokenType construct = findConstruct();
      if (construct != UNPROCESSED) {
        addToken(Token(construct, code.substr(pos, 1), currentLine));
        continue;
      }
      // Check for fat arrows
      if (code.substr(pos, 2) == "=>") {
        addToken(Token(K_FAT_ARROW, "=>", currentLine));
        skip(2);
        continue;
      }
      // Check for operators
      auto operatorIt = operatorList.end();
      for (auto op = operatorList.begin(); op != operatorList.end(); ++op) {
        bool doesNameMatch = code.substr(pos, op->getName().length()) == op->getName();
        if (!doesNameMatch) continue;
        Fixity type;
        if (op->getName() == "++" || op->getName() == "--") {
          // Figure out if it's postfix or prefix
          type = determineFixity(PREFIX, POSTFIX, PREFIX);
        } else if (op->getName() == "+" || op->getName() == "-") {
          // Figure out if it's postfix or infix
          type = determineFixity(PREFIX, INFIX, PREFIX);
        } else {
          // This operator is unambiguous
          operatorIt = op;
          break;
        }
        // Refine the search using the fixity information
        operatorIt = std::find_if(ALL(operatorList), [=](const Operator& currentOp) {
          if (op->getName() == currentOp.getName() && currentOp.getFixity() == type) return true;
          return false;
        });
        break;
      }
      if (operatorIt != operatorList.end()) {
        skip(operatorIt->getName().length());
        addToken(Token(OPERATOR, operatorIt - operatorList.begin(), currentLine));
        noIncrement();
        continue;
      }
      // Get a string until the char can't be part of an identifier
      std::string str = "";
      while (isIdentifierChar()) {
        str += code[pos];
        skip(1);
      }
      noIncrement();
      // No point in looking for it anywhere if it's empty
      if (str.length() == 0) continue;
      // Check for boolean literals
      if (str == "true" || str == "false") {
        addToken(Token(L_BOOLEAN, str, currentLine));
        continue;
      }
      // Check for keywords
      try {
        TokenType type = keywordsMap.at(str);
        addToken(Token(type, str, currentLine));
        continue;
      } catch (std::out_of_range &oor) {
        // This means it's not a keyword, so it must be an identifier
        addToken(Token(IDENTIFIER, str, currentLine));
        continue;
      }
    }
    tokens.push_back(Token(FILE_END, "\0", currentLine));
    return *this;
  }
  
  std::vector<Token> getTokens() const {
    return tokens;
  }
  
  Token operator[](uint64 pos) {
    return tokens[pos];
  }
  
  uint64 getLineCount() const {
    if (tokens.size() == 0) return 0;
    return currentLine;
  }
};

#endif
