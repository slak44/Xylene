#ifndef PARSER_TEST_HPP
#define PARSER_TEST_HPP

#include <gtest/gtest.h>

#include "parser.hpp"
#include "token.hpp"

class ParserTest: public ::testing::Test {
protected:
  Parser px = Parser();
};

TEST_F(ParserTest, Expression) {
  return;
  // (-12 + -3) / 1.5 >> 1
  px.parse({
    Token(C_PAREN_LEFT, "(", 1), Token(OPERATOR, 24, 1), Token(L_INTEGER, "12", 1),
    Token(OPERATOR, 31, 1), Token(OPERATOR, 24, 1), Token(L_INTEGER, "3", 1), Token(C_PAREN_RIGHT, ")", 1),
    Token(OPERATOR, 29, 1), Token(L_FLOAT, "1.5", 1), Token(OPERATOR, 13, 1), Token(L_INTEGER, "1", 1), Token(FILE_END, "", 1)
  });
  ASTNode root = px.getTree().root;
}

TEST_F(ParserTest, SimpleASTEquality) {
  px.parse({Token(L_INTEGER, "1", 1), Token(OPERATOR, 31, 1), Token(L_INTEGER, "1", 1), Token(FILE_END, "", 1)});
  AST tree = AST();
  auto operand1 = Node<ExpressionNode>::make(Token(L_INTEGER, "1", 1));
  auto operand2 = Node<ExpressionNode>::make(Token(L_INTEGER, "1", 1));
  auto op = Node<ExpressionNode>::make(Token(OPERATOR, 31, 1));
  op->addChild(operand1);
  op->addChild(operand2);
  auto rootBlock = Node<BlockNode>::make();
  rootBlock->addChild(op);
  tree.root = *rootBlock;
  ASSERT_EQ(px.getTree(), tree);
}

#endif
