#include "lexer.hpp"

Lexer::Lexer(std::string code, std::string sourceOfCode):
  code(code), sourceOfCode(sourceOfCode) {}

std::unique_ptr<Lexer> Lexer::tokenize(std::string code, std::string sourceOfCode) {
  Lexer* lx = new Lexer(code, sourceOfCode);
  lx->processTokens();
  return std::unique_ptr<Lexer>(lx);
}

void Lexer::processTokens() {
  for (; !hasFinishedString(); skip(1)) {
    // Comments
    if (current(2) == "//") {
      while (!isEOL() && !isEOF()) skip(1);
      skip(1); // Skip EOL/EOF
    }
    handleMultiLineComments();
    // Count lines
    if (isEOL()) nextLine();
    // Ignore whitespace
    if (isspace(current())) continue;
    // Check for number literals
    if (isdigit(current())) {
      auto radix = getNumberRadix();
      addToken(getNumberToken(radix));
      continue;
    }
    // Check for string literals
    if (current() == '"') {
      Position start = getCurrentPosition();
      skip(1); // Skip the quote
      addToken(Token(TT::STRING, getQuotedString(), Trace(sourceOfCode, getRangeToHere(start))));
      continue;
    }
    // Check for constructs
    TokenType construct = TT::findConstruct(current());
    if (construct != TT::UNPROCESSED) {
      addToken(Token(construct, current(1), Trace(sourceOfCode, Range(getCurrentPosition(), 1))));
      continue;
    }
    // Check for fat arrows
    if (current(2) == "=>") {
      addToken(Token(TT::FAT_ARROW, "=>", Trace(sourceOfCode, Range(getCurrentPosition(), 2))));
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
        if (op->hasSymbol(currentOp.getSymbol()) && currentOp.getFixity() == type) return true;
        return false;
      });
      break;
    }
    if (operatorIt != Operator::list.end()) {
      Position operatorStart = getCurrentPosition();
      skip(operatorIt->getSymbol().length());
      addToken(Token(
        TT::OPERATOR,
        static_cast<Operator::Index>(operatorIt - Operator::list.begin()),
        Trace(sourceOfCode, getRangeToHere(operatorStart))
      ));
      noIncrement();
      continue;
    }
    // Get a string until the char can't be part of an identifier
    std::string str = "";
    Position identStart = getCurrentPosition();
    while (isIdentifierChar()) {
      if (current() == '\\') throw Error("SyntaxError", "Extraneous escape character", Trace(sourceOfCode, Range(getCurrentPosition(), 1)));
      str += current();
      skip(1);
    }
    noIncrement();
    // No point in looking for it anywhere if it's empty
    if (str.length() == 0) continue;
    // Check for boolean literals
    if (str == "true" || str == "false") {
      addToken(Token(TT::BOOLEAN, str, Trace(sourceOfCode, getRangeToHere(identStart))));
      continue;
    }
    // Check for keywords
    TokenType keyword = TT::findKeyword(str);
    if (keyword != TT::UNPROCESSED) {
      addToken(Token(keyword, str, Trace(sourceOfCode, getRangeToHere(identStart))));
      continue;
    }
    
    // Must be an identifier
    addToken(Token(TT::IDENTIFIER, str, Trace(sourceOfCode, getRangeToHere(identStart))));
  }
  addToken(Token(TT::FILE_END, Trace(sourceOfCode, Range(getCurrentPosition(), 1))));
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
  return getCurrentLine();
}

std::string Lexer::getCodeSource() const noexcept {
  return sourceOfCode;
}
