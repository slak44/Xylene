#ifndef BASE_PARSER_HPP
#define BASE_PARSER_HPP

#include "ast.hpp"

/**
  \brief Base of all parsers. Consistent API for getting the AST from a parser.
*/
class BaseParser {
protected:
  std::unique_ptr<AST> tree;
  
  BaseParser();
public:
  AST getTree() const {
    return *tree;
  }
};

inline BaseParser::BaseParser() {}

#endif
