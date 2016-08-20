#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>

#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "parser/xmlParser.hpp"
#include "token.hpp"

class ParserTest: public ::testing::Test {
protected:
  TokenParser px = TokenParser();
  XMLParser xpx = XMLParser();
  
  inline rapidxml::file<> xmlFile(std::string path) {
    path = DATA_PARENT_DIR + ("/" + path);
    return rapidxml::file<>(path.c_str());
  }
};

TEST_F(ParserTest, Primaries) {
  px.parse(Lexer().tokenize("+ -- a ++ --;").getTokens());
  ASSERT_EQ(px.getTree(), xpx.parse(xmlFile("data/parser/unary.xml")).getTree());
}

TEST_F(ParserTest, ExpressionParsing) {
  // (-12 + -3) / 1.5 >> 1
  
  // Manually lexed
  px.parse({
    Token(C_PAREN_LEFT, "(", 1), Token(OPERATOR, 24, 1), Token(L_INTEGER, "12", 1),
    Token(OPERATOR, 31, 1), Token(OPERATOR, 24, 1), Token(L_INTEGER, "3", 1), Token(C_PAREN_RIGHT, ")", 1),
    Token(OPERATOR, 29, 1), Token(L_FLOAT, "1.5", 1), Token(OPERATOR, 13, 1), Token(L_INTEGER, "1", 1), Token(C_SEMI, ";", 1), Token(FILE_END, "", 1)
  });
  AST manualLexer = px.getTree();
  
  // Lexed by Lexer class
  px.parse(Lexer().tokenize("(-12 + -3) / 1.5 >> 1;").getTokens());
  AST lexerLexer = px.getTree();
  
  // No lexing or parsing; AST created directly
  using ExprNode = Node<ExpressionNode>;
  auto rootBlock = Node<BlockNode>::make(ROOT_BLOCK);
  auto shift = ExprNode::make(Token(OPERATOR, 13, 1));
  auto division = ExprNode::make(Token(OPERATOR, 29, 1));
  auto addition = ExprNode::make(Token(OPERATOR, 31, 1));
  auto negInt1 = ExprNode::make(Token(OPERATOR, 24, 1));
  negInt1->addChild(ExprNode::make(Token(L_INTEGER, "12", 1)));
  auto negInt2 = ExprNode::make(Token(OPERATOR, 24, 1));
  negInt2->addChild(ExprNode::make(Token(L_INTEGER, "3", 1)));
  addition->addChild(negInt1);
  addition->addChild(negInt2);
  division->addChild(addition);
  division->addChild(ExprNode::make(Token(L_FLOAT, "1.5", 1)));
  shift->addChild(division);
  shift->addChild(ExprNode::make(Token(L_INTEGER, "1", 1)));
  rootBlock->addChild(shift);
  AST noLexer = AST(rootBlock);
  
  EXPECT_EQ(manualLexer, lexerLexer);
  EXPECT_EQ(manualLexer, noLexer);
  EXPECT_EQ(lexerLexer, noLexer);
}

TEST_F(ParserTest, SimpleASTEquality) {
  // 1 + 1;
  px.parse({Token(L_INTEGER, "1", 1), Token(OPERATOR, 31, 1), Token(L_INTEGER, "1", 1), Token(C_SEMI, ";", 1), Token(FILE_END, "", 1)});
  auto operand1 = Node<ExpressionNode>::make(Token(L_INTEGER, "1", 1));
  auto operand2 = Node<ExpressionNode>::make(Token(L_INTEGER, "1", 1));
  auto op = Node<ExpressionNode>::make(Token(OPERATOR, 31, 1));
  op->addChild(operand1);
  op->addChild(operand2);
  auto rootBlock = Node<BlockNode>::make(ROOT_BLOCK);
  rootBlock->addChild(op);
  AST tree = AST(rootBlock);
  ASSERT_EQ(px.getTree(), tree);
}

TEST_F(ParserTest, NoBinaryOperators) {
  // -1;
  px.parse({
    Token(OPERATOR, operatorIndexFrom("Unary -"), 1),
    Token(L_INTEGER, "1", 1),
    Token(C_SEMI, ";", 1),
    Token(FILE_END, "", 1)
  });
  auto op = Node<ExpressionNode>::make(Token(OPERATOR, operatorIndexFrom("Unary -"), 1));
  op->addChild(Node<ExpressionNode>::make(Token(L_INTEGER, "1", 1)));
  auto rootBlock = Node<BlockNode>::make(ROOT_BLOCK);
  rootBlock->addChild(op);
  AST tree = AST(rootBlock);
  ASSERT_EQ(tree, px.getTree());
}
TEST_F(ParserTest, XMLParse) {
  // (12 + 3) / 1.5;
  px.parse({
    Token(C_PAREN_LEFT, "(", 1), Token(L_INTEGER, "12", 1),
    Token(OPERATOR, 31, 1), Token(L_INTEGER, "3", 1), Token(C_PAREN_RIGHT, ")", 1),
    Token(OPERATOR, 29, 1), Token(L_FLOAT, "1.5", 1), Token(C_SEMI, ";", 1), Token(FILE_END, "", 1)
  });
  xpx.parse(xmlFile("data/parser/simple_expr.xml"));
  ASSERT_EQ(px.getTree(), xpx.getTree());
}

TEST_F(ParserTest, ExpressionStartingWithIdent) {
  px.parse(Lexer().tokenize("a + 1;").getTokens());
  xpx.parse(xmlFile("data/parser/expr_with_ident.xml"));
  ASSERT_EQ(px.getTree(), xpx.getTree());
}

TEST_F(ParserTest, IfStatement) {
  px.parse(Lexer().tokenize(R"(
    if true == false do
      1 + 2;
    else do
      100 - 101;
    end
  )").getTokens());
  xpx.parse(xmlFile("data/parser/if_statement.xml"));
  ASSERT_EQ(px.getTree(), xpx.getTree());
}

TEST_F(ParserTest, Declarations) {
  px.parse(Lexer().tokenize(R"(
    define a = 1;
    Integer i = 2;
    Float, Integer nr = 3.3;
  )").getTokens());
  xpx.parse(xmlFile("data/parser/declarations.xml"));
  ASSERT_EQ(px.getTree(), xpx.getTree());
}

TEST_F(ParserTest, ForLoop) {
  px.parse(Lexer().tokenize(R"(
    for define x = 1; x < 3; ++x do
      1+1;
    end
  )").getTokens());
  xpx.parse(xmlFile("data/parser/for_loop.xml"));
  ASSERT_EQ(px.getTree(), xpx.getTree());
}

TEST_F(ParserTest, ReturnStatement) {
  px.parse(Lexer().tokenize(R"(
    return 1 + 1;
  )").getTokens());
  xpx.parse(xmlFile("data/parser/simple_return.xml"));
  ASSERT_EQ(px.getTree(), xpx.getTree());
}
