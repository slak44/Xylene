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
    px = TokenParser();
    px.parse(Lexer::tokenize(code, xmlCodePath)->getTokens());
    xpx.parse(xmlFile(xmlCodePath));
    ASSERT_EQ(px.getTree(), xpx.getTree());
  }
};

TEST_F(ParserCompareTest, Expressions) {
  test("+ -- a ++ --;", "data/parser/unary.xml");
  test("a + 1;", "data/parser/expr/start_with_ident.xml");
  test("-(a + b)++;", "data/parser/expr/parens_unaries.xml");
}

TEST_F(ParserCompareTest, FunctionCalls) {
  test("a();", "data/parser/expr/calls/no_args.xml");
  test("a(42);", "data/parser/expr/calls/one_terminal_arg.xml");
  test("a(1+1);", "data/parser/expr/calls/one_expr_arg.xml");
  test("a(42, 6714);", "data/parser/expr/calls/2_args.xml");
  test("a(1+1, 3 * (5 + 5));", "data/parser/expr/calls/expression_args.xml");
  test("-a++();", "data/parser/expr/calls/after_postfix.xml");
}

TEST_F(ParserCompareTest, Functions) {
  test(R"code(
    function add [Integer, Float a, Integer, Float b] => Integer, Float do
    end
  )code", "data/parser/functions/complete.xml");
  test(R"code(
    function add [Integer, Float a, Integer, Float b] do
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
  test(R"code(
    foreign function putchar [Integer char] => Integer;
  )code", "data/parser/functions/foreign.xml");
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

TEST_F(ParserCompareTest, Type) {
  test(R"code(
    type CoolType inherits from CoolParentType do
      Integer i = 0;
      public constructor [Integer int] do
        i = int;
      end
      public static method nice [Integer x] => Integer do
        return x + 1;
      end
    end
  )code", "data/parser/type_complete.xml");
}
