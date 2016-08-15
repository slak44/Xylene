#ifndef BASE_PARSER_HPP
#define BASE_PARSER_HPP

#include "ast.hpp"

class BaseParser {
protected:
  AST tree;
public:
  virtual ~BaseParser() = 0;
  
  AST getTree() const {
    return tree;
  }
};

inline BaseParser::~BaseParser() {}

#endif
