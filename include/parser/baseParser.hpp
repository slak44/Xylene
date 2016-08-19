#ifndef BASE_PARSER_HPP
#define BASE_PARSER_HPP

#include "ast.hpp"

/**
  \brief Base of all parsers. Consistent API for getting the AST from a parser.
*/
class BaseParser {
protected:
  std::unique_ptr<AST> tree;
public:
  BaseParser();
  BaseParser(const BaseParser&);
  BaseParser(BaseParser&&);
  virtual ~BaseParser() = 0;
  
  AST getTree() const {
    return *tree;
  }
};

inline BaseParser::BaseParser() {}
inline BaseParser::BaseParser(const BaseParser&) {}
inline BaseParser::BaseParser(BaseParser&&) {}
inline BaseParser::~BaseParser() {}

#endif
