#include "lexer.hpp"

Lexer::Lexer(std::string code, std::string sourceOfCode):
  code(code), sourceOfCode(sourceOfCode) {}

std::unique_ptr<Lexer> Lexer::tokenize(std::string code, std::string sourceOfCode) {
  Lexer* lx = new Lexer(code, sourceOfCode);
  lx->processTokens();
  return std::unique_ptr<Lexer>(lx);
}

bool Lexer::isIdentifierChar() const noexcept {
  bool isOperatorChar = Operator::operatorCharacters.find(current()) !=
    Operator::operatorCharacters.end();
  bool isConstructChar = TT::findConstruct(current()) != TT::UNPROCESSED;
  return !isOperatorChar && !isConstructChar && !isEOL() && !isEOF();
}

Fixity Lexer::determineFixity(
  Fixity afterBinaryOrPrefix,
  Fixity afterIdentOrParen,
  Fixity otherCases
) const noexcept {
  if (tokens.empty()) return otherCases;
  auto tok = tokens.back();
  if (tok.isOp() && (tok.op().hasArity(BINARY) || tok.op().hasFixity(PREFIX))) {
    // If the thing before this op is a binary op or a prefix unary, it means this
    // must be a prefix unary
    return afterBinaryOrPrefix;
  } else if (
    tok.isTerminal() ||
    tok.type == TT::PAREN_RIGHT ||
    (tok.isOp() && tok.op().hasFixity(POSTFIX))
  ) {
    // If the thing before was an identifier/paren (or the postfix op attached to an
    // ident), then this must be a postfix unary or a binary infix
    return afterIdentOrParen;
  }
  return otherCases;
}

inline void Lexer::handleMultiLineComments() {
  // TODO: nested comments
  Position start = getPos();
  if (current(2) == "/*") {
    skip(2); // Skip "/*"
    while (current(2) != "*/") {
      if (isEOF()) throw Error("SyntaxError",
        "Multi-line comment not closed", traceFrom(start));
      else if (isEOL()) nextLine();
      skip(1); // Skip characters one by one until we hit the end of the comment
    }
    skip(2); // Skip "*/"
  }
}

uint Lexer::readRadix() {
  if (current() == '0' && isalpha(peekAhead(1))) {
    Position zeroPos = getPos();
    auto radixIdent = peekAhead(1);
    skip(2); // Skip the "0x", etc
    switch (radixIdent) {
      case 'x': return 16;
      case 'b': return 2;
      case 'o': return 8;
      default: throw Error("SyntaxError", "Invalid radix", traceFrom(zeroPos));
    }
  } else {
    return 10;
  }
}

#pragma GCC diagnostic push
// It doesn't matter for these comparisons
#pragma GCC diagnostic ignored "-Wsign-compare"

bool Lexer::isValidForRadix(char c, uint radix) const noexcept {
  if (radix == 0) return false;
  if (radix <= 10 && c >= '0' && c < '0' + radix) return true;
  if (c >= '0' && c <= '9') return true;
  c = static_cast<char>(std::toupper(c));
  if (c >= 'A' && c < 'A' + radix - 10) return true;
  return false;
}

#pragma GCC diagnostic pop

Token Lexer::readNumber(uint radix) {
  Position start = getPos();
  if (current() == '0' && std::isdigit(peekAhead(1))) throw Error("SyntaxError",
    "Numbers cannot begin with '0'", Trace(sourceOfCode, Range(start, 1)));
  std::string number = "";
  bool isFloat = false;
  while (!isEOF()) {
    if (current() == '.') {
      if (isFloat) throw Error("SyntaxError",
        "Malformed float, multiple decimal points", traceFrom(start));
      isFloat = true;
      number += current();
      skip(1);
    } else if (isValidForRadix(current(), radix)) {
      number += current();
      skip(1);
    } else {
      break;
    }
  }
  noIncrement();
  if (number.back() == '.') throw Error("SyntaxError",
    "Malformed float, missing digits after decimal point", traceFrom(start));
  if (radix != 10 && isFloat) throw Error("SyntaxError",
    "Floating point numbers must be decimal", traceFrom(start));
  if (radix != 10)
    number = std::to_string(std::stoll(number, nullptr, static_cast<int>(radix)));
  return Token(isFloat ? TT::FLOAT : TT::INTEGER, number, traceFrom(start));
}

char Lexer::readEscapeSeq() {
  Position escChar = getPos();
  char escapedChar = peekAhead(1);
  skip(2); // Skip escape code, eg '\n', '\t'
  auto charIt = singleCharEscapeSeqences.find(escapedChar);
  if (charIt != std::end(singleCharEscapeSeqences)) {
    return charIt->second;
  }
  auto badEscape = Error("SyntaxError", "Invalid escape code", traceFrom(escChar));
  if (escapedChar == 'x') {
    // \x00 hex escape code
    std::string hexNumber = "";
    if (std::isxdigit(current())) hexNumber += current();
    else throw badEscape;
    if (std::isxdigit(peekAhead(1))) hexNumber += peekAhead(1);
    else throw badEscape;
    skip(2);
    return static_cast<char>(std::stoi(hexNumber, nullptr, 16));
  } else if (std::isdigit(escapedChar)) {
    // \000 octal escape code
    std::string octalNumber = "";
    if (isValidForRadix(escapedChar, 8)) octalNumber += escapedChar;
    else throw badEscape;
    if (isValidForRadix(current(), 8)) octalNumber += current();
    else throw badEscape;
    if (isValidForRadix(peekAhead(1), 8)) octalNumber += peekAhead(1);
    else throw badEscape;
    skip(3);
    return static_cast<char>(std::stoi(octalNumber, nullptr, 8));
  }
  throw badEscape;
}

void Lexer::processTokens() {
  for (; pos != code.length(); skip(1)) {
    // Comments
    if (current(2) == "//") {
      while (!isEOL() && !isEOF()) skip(1);
      continue;
    }
    handleMultiLineComments();
    // Count lines
    if (isEOL()) nextLine();
    // Ignore whitespace
    if (std::isspace(current())) continue;
    // Check for number literals
    if (std::isdigit(current())) {
      uint radix = readRadix();
      tokens.push_back(readNumber(radix));
      continue;
    }
    // Check for string literals
    if (current() == '"') {
      Position start = getPos();
      skip(1); // Skip the quote
      std::string str = "";
      while (current() != '"') {
        if (isEOF()) throw Error("SyntaxError",
          "String literal has unmatched quote", traceFrom(start));
        if (current() == '\\') {
          str += readEscapeSeq();
          continue;
        }
        str += current();
        skip(1);
      }
      tokens.push_back(Token(TT::STRING, str, traceFrom(start)));
      continue;
    }
    // Check for constructs
    TokenType construct = TT::findConstruct(current());
    if (construct != TT::UNPROCESSED) {
      tokens.push_back(Token(construct, current(1), traceFor(1)));
      continue;
    }
    // Check for fat arrows
    if (current(2) == "=>") {
      tokens.push_back(Token(TT::FAT_ARROW, "=>", traceFor(2)));
      skip(2);
      continue;
    }
    // Check for operators
    auto operatorIt = Operator::list.end();
    for (auto op = Operator::list.begin(); op != Operator::list.end(); ++op) {
      bool doesNameMatch =
        current(static_cast<uint>(op->getSymbol().length())) == op->getSymbol();
      if (!doesNameMatch) continue;
      Fixity type;
      if (op->hasSymbol("++") || op->hasSymbol("--")) {
        // Figure out if it's postfix or prefix
        type = determineFixity(PREFIX, POSTFIX, PREFIX);
      } else if (op->hasSymbol("+") || op->hasSymbol("-")) {
        // Figure out if it's postfix or infix
        type = determineFixity(PREFIX, INFIX, PREFIX);
      } else {
        // This operator is unambiguous
        operatorIt = op;
        break;
      }
      // Refine the search using the fixity information
      operatorIt = std::find_if(ALL(Operator::list), [=](const Operator& currentOp) {
        return op->hasSymbol(currentOp.getSymbol()) && currentOp.getFixity() == type;
      });
      break;
    }
    if (operatorIt != Operator::list.end()) {
      Position operatorStart = getPos();
      skip(operatorIt->getSymbol().length());
      tokens.push_back(Token(
        TT::OPERATOR,
        static_cast<Operator::Index>(operatorIt - Operator::list.begin()),
        traceFrom(operatorStart)
      ));
      noIncrement();
      continue;
    }
    // Get a string until the char can't be part of an identifier
    std::string str = "";
    Position identStart = getPos();
    while (isIdentifierChar()) {
      if (current() == '\\')
        throw Error("SyntaxError", "Extraneous escape character", traceFor(1));
      str += current();
      skip(1);
    }
    noIncrement();
    // No point in looking for it anywhere if it's empty
    if (str.length() == 0) continue;
    // Check for boolean literals
    if (str == "true" || str == "false") {
      tokens.push_back(Token(TT::BOOLEAN, str, traceFrom(identStart)));
      continue;
    }
    // Check for keywords
    TokenType keyword = TT::findKeyword(str);
    if (keyword != TT::UNPROCESSED) {
      tokens.push_back(Token(keyword, str, traceFrom(identStart)));
      continue;
    }
    
    // Must be an identifier
    tokens.push_back(Token(TT::IDENTIFIER, str, traceFrom(identStart)));
  }
  tokens.push_back(Token(TT::FILE_END, traceFor(1)));
}

Token Lexer::operator[](std::size_t at) const {
  return tokens[at];
}

const std::string& Lexer::getCode() const noexcept {
  return code;
}

const std::vector<Token>& Lexer::getTokens() const noexcept {
  return tokens;
}

uint64 Lexer::getLineCount() const noexcept {
  if (tokens.empty()) return 0;
  return currentLine;
}

std::string Lexer::getCodeSource() const noexcept {
  return sourceOfCode;
}
