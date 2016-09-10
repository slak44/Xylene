#include "lexer.hpp"

const std::string& LexerBase::getCode() const {
  return code;
}

const std::vector<Token>& LexerBase::getTokens() const {
  return tokens;
}

const Token& LexerBase::operator[](std::size_t p) const {
  return tokens[p];
}

uint64 LexerBase::getLineCount() const {
  if (tokens.size() == 0) return 0;
  return getCurrentLine();
}

LexerBase& LexerBase::tokenize(std::string code, std::string fileName) {
  this->code = code;
  this->fileName = fileName;
  pos = 0;
  tokens = {};
  currentLine = 1;
  processInput();
  return *this;
};

void Lexer::processInput() {
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
      addToken(Token(L_STRING, getQuotedString(), Trace(getFileName(), getRangeToHere(start))));
      continue;
    }
    // Check for constructs
    TokenType construct = findConstruct();
    if (construct != UNPROCESSED) {
      addToken(Token(construct, current(1), Trace(getFileName(), Range(getCurrentPosition(), 1))));
      continue;
    }
    // Check for fat arrows
    if (current(2) == "=>") {
      addToken(Token(K_FAT_ARROW, "=>", Trace(getFileName(), Range(getCurrentPosition(), 2))));
      skip(2);
      continue;
    }
    // Check for operators
    auto operatorIt = operatorList.end();
    for (auto op = operatorList.begin(); op != operatorList.end(); ++op) {
      bool doesNameMatch = current(op->getName().length()) == op->getName();
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
      Position operatorStart = getCurrentPosition();
      skip(operatorIt->getName().length());
      addToken(Token(OPERATOR, operatorIt - operatorList.begin(), Trace(getFileName(), getRangeToHere(operatorStart))));
      noIncrement();
      continue;
    }
    // Get a string until the char can't be part of an identifier
    std::string str = "";
    Position identStart = getCurrentPosition();
    while (isIdentifierChar()) {
      if (current() == '\\') throw Error("SyntaxError", "Extraneous escape character", Trace(getFileName(), Range(getCurrentPosition(), 1)));
      str += current();
      skip(1);
    }
    noIncrement();
    // No point in looking for it anywhere if it's empty
    if (str.length() == 0) continue;
    // Check for boolean literals
    if (str == "true" || str == "false") {
      addToken(Token(L_BOOLEAN, str, Trace(getFileName(), getRangeToHere(identStart))));
      continue;
    }
    // Check for keywords
    try {
      TokenType type = keywordsMap.at(str);
      addToken(Token(type, str, Trace(getFileName(), getRangeToHere(identStart))));
      continue;
    } catch (std::out_of_range &oor) {
      // This means it's not a keyword, so it must be an identifier
      addToken(Token(IDENTIFIER, str, Trace(getFileName(), getRangeToHere(identStart))));
      continue;
    }
  }
  addToken(Token(FILE_END, "", Trace(getFileName(), Range(getCurrentPosition(), 1))));
}
