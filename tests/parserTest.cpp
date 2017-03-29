#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>

#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "parser/xmlParser.hpp"
#include "token.hpp"

class ParserTest: public ::testing::Test {
protected:
  inline rapidxml::file<> xmlFile(std::string path) {
    path = fmt::format("{0}/{1}", DATA_PARENT_DIR, path);
    return rapidxml::file<>(path.c_str());
  }
  inline void parse(std::string code) {
    TokenParser::parse(Lexer::tokenize(code, "<parser-test>")->getTokens());
  }
};

class ParserCompareTest: public ParserTest {
protected:
  inline void compare(std::string code, std::string xmlCodePath) {
    auto pxTr = TokenParser::parse(Lexer::tokenize(code, xmlCodePath)->getTokens());
    auto xpxTr = XMLParser::parse(xmlFile(xmlCodePath));
    EXPECT_EQ(pxTr, xpxTr);
  }
};

TEST_F(ParserCompareTest, Expressions) {
  compare("+ -- a ++ --;", "data/parser/unary.xml");
  compare("a + 1;", "data/parser/expr/start_with_ident.xml");
  compare("-(a + b)++;", "data/parser/expr/parens_unaries.xml");
  EXPECT_THROW(parse("1 + return;"), Error);
  EXPECT_THROW(parse("1 + do;"), Error);
  EXPECT_THROW(parse("1 + else;"), Error);
  EXPECT_THROW(parse("1+1 return;"), Error);
  EXPECT_THROW(parse("if do end;"), Error);
}

TEST_F(ParserCompareTest, FunctionCalls) {
  compare("a();", "data/parser/expr/calls/no_args.xml");
  compare("a(42);", "data/parser/expr/calls/one_terminal_arg.xml");
  compare("a(1+1);", "data/parser/expr/calls/one_expr_arg.xml");
  compare("a(42, 6714);", "data/parser/expr/calls/2_args.xml");
  compare("a(1+1, 3 * (5 + 5));", "data/parser/expr/calls/expression_args.xml");
  compare("-a++();", "data/parser/expr/calls/after_postfix.xml");
}

TEST_F(ParserCompareTest, Functions) {
  compare(R"code(
    function add [Integer, Float a, Integer, Float b] => Integer, Float do
    end
  )code", "data/parser/functions/complete.xml");
  compare(R"code(
    function add [Integer, Float a, Integer, Float b] do
    end
  )code", "data/parser/functions/void_ret.xml");
  compare(R"code(
    function add => Integer do
    end
  )code", "data/parser/functions/no_args.xml");
  compare(R"code(
    function add do
    end
  )code", "data/parser/functions/plain.xml");
  compare(R"code(
    function [Integer e] => Integer do
    end
  )code", "data/parser/functions/anon.xml");
  compare(R"code(
    foreign function putchar [Integer char] => Integer;
  )code", "data/parser/functions/foreign.xml");
}

TEST_F(ParserCompareTest, IfStatement) {
  compare(R"code(
    if true == false do
      1 + 2;
    else do
      100 - 101;
    end
  )code", "data/parser/if_statement.xml");
}

TEST_F(ParserCompareTest, Declarations) {
  compare(R"code(
    define a = 1;
    Integer i = 2;
    Float, Integer nr = 3.3;
  )code", "data/parser/declarations.xml");
}

TEST_F(ParserCompareTest, ForLoop) {
  compare(R"code(
    for define x = 1; x < 3; do
      1+1;
    end
  )code", "data/parser/for_loops/no_updates.xml");
  compare(R"code(
    for define x = 1; x < 3; ++x do
      1+1;
    end
  )code", "data/parser/for_loops/for_loop.xml");
  compare(R"code(
    for define x = 1; x < 6; ++x, ++x do
      1+1;
    end
  )code", "data/parser/for_loops/2_updates.xml");
  compare(R"code(
    for define x = 1; x < 6; ++x, ++x, --x do
      1+1;
    end
  )code", "data/parser/for_loops/3_updates.xml");
}

TEST_F(ParserCompareTest, ReturnStatement) {
  compare(R"code(
    return 1 + 1;
  )code", "data/parser/simple_return.xml");
}

TEST_F(ParserCompareTest, Type) {
  compare(R"code(
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
