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

class ParserCompareTest: public ParserTest {
protected:
  inline void test(std::string code, std::string xmlCodePath) {
    px.parse(Lexer().tokenize(code).getTokens());
    xpx.parse(xmlFile(xmlCodePath));
    ASSERT_EQ(px.getTree(), xpx.getTree());
  }
};

TEST_F(ParserTest, Primaries) {
  px.parse(Lexer().tokenize("+ -- a ++ --;").getTokens());
  ASSERT_EQ(px.getTree(), xpx.parse(xmlFile("data/parser/unary.xml")).getTree());
}

TEST_F(ParserCompareTest, ExpressionStartingWithIdent) {
  test("a + 1;", "data/parser/expr_with_ident.xml");
}

TEST_F(ParserCompareTest, Functions) {
  test(R"code(
    function add [Integer, Float a, b: Integer, Float] => Integer, Float do
    end
  )code", "data/parser/functions/complete.xml");
  test(R"code(
    function add [Integer, Float a, b: Integer, Float] do
    end
  )code", "data/parser/functions/void_ret.xml");
  test(R"code(
    function add [Integer, Float a, b: Integer, Float] do
    end
  )code", "data/parser/functions/void_ret.xml");
  test(R"code(
    function add => Integer do
    end
  )code", "data/parser/functions/no_args.xml");
  test(R"code(
    function add do
    end
  )code", "data/parser/functions/plain.xml");
  test(R"code(
    function [Integer e] => Integer do
    end
  )code", "data/parser/functions/anon.xml");
}

TEST_F(ParserCompareTest, IfStatement) {
  test(R"code(
    if true == false do
      1 + 2;
    else do
      100 - 101;
    end
  )code", "data/parser/if_statement.xml");
}

TEST_F(ParserCompareTest, Declarations) {
  test(R"code(
    define a = 1;
    Integer i = 2;
    Float, Integer nr = 3.3;
  )code", "data/parser/declarations.xml");
}

TEST_F(ParserCompareTest, ForLoop) {
  test(R"code(
    for define x = 1; x < 3; ++x do
      1+1;
    end
  )code", "data/parser/for_loop.xml");
}

TEST_F(ParserCompareTest, ReturnStatement) {
  test(R"code(
    return 1 + 1;
  )code", "data/parser/simple_return.xml");
}
