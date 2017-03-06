#include <gtest/gtest.h>
#include <vector>

#include "utils/util.hpp"
#include "lexer.hpp"
#include "token.hpp"

class LexerTest: public ::testing::Test {
protected:
  std::unique_ptr<Lexer> lx;
  std::vector<Token> getTokens(std::string code) {
    lx = Lexer::tokenize(code, "<lexer-test>");
    return lx->getTokens();
  }
  Token at(std::size_t pos) {
    return (*lx)[pos];
  }
  static const std::vector<Token> fileEnd;
};

const std::vector<Token> LexerTest::fileEnd {Token(TT::FILE_END, "", defaultTrace)};

TEST_F(LexerTest, Hashbang) {
  ASSERT_EQ(getTokens("#! not actually valid but should ignore this whole line"), fileEnd);
}

TEST_F(LexerTest, Identifier) {
  ASSERT_EQ(getTokens("hello")[0], Token(TT::IDENTIFIER, "hello", defaultTrace));
}

TEST_F(LexerTest, CommentsSingleLine) {
  ASSERT_EQ(getTokens("// \xAA"), fileEnd);
  ASSERT_EQ(getTokens("// test\n"), fileEnd);
  EXPECT_EQ(getTokens("// asd 123 . ////**//"), fileEnd);
  ASSERT_EQ(lx->getLineCount(), 1);
}

TEST_F(LexerTest, CommentsMultiLine) {
  EXPECT_EQ(getTokens("/*asdad\ndasd\nasd*/"), fileEnd);
  ASSERT_EQ(lx->getLineCount(), 3);
}

TEST_F(LexerTest, IntegerLiterals) {
  ASSERT_EQ(getTokens("123")[0], Token(TT::INTEGER, "123", defaultTrace));
  ASSERT_EQ(getTokens("0xA")[0], Token(TT::INTEGER, "10", defaultTrace));
  ASSERT_EQ(getTokens("0o10")[0], Token(TT::INTEGER, "8", defaultTrace));
  ASSERT_EQ(getTokens("0b10")[0], Token(TT::INTEGER, "2", defaultTrace));
  ASSERT_THROW(getTokens("123abc"), Error);
  ASSERT_NO_THROW(getTokens("123+1"));
}

TEST_F(LexerTest, Radix) {
  ASSERT_THROW(getTokens("0j123")[0], Error);
  ASSERT_THROW(getTokens("0123")[0], Error);
  ASSERT_THROW(getTokens("0x0123")[0], Error);
  ASSERT_EQ(getTokens("0")[0], Token(TT::INTEGER, "0", defaultTrace));
}

TEST_F(LexerTest, FloatLiterals) {
  ASSERT_EQ(getTokens("12.3")[0], Token(TT::FLOAT, "12.3", defaultTrace));
  ASSERT_THROW(getTokens("12.")[0], Error);
  ASSERT_THROW(getTokens("12.123.")[0], Error);
  ASSERT_THROW(getTokens("0x12.123")[0], Error);
}

TEST_F(LexerTest, BooleanLiterals) {
  ASSERT_EQ(getTokens("true")[0], Token(TT::BOOLEAN, "true", defaultTrace));
  ASSERT_EQ(getTokens("false")[0], Token(TT::BOOLEAN, "false", defaultTrace));
}

TEST_F(LexerTest, StringLiterals) {
  ASSERT_EQ(getTokens(R"("qwerty123")")[0], Token(TT::STRING, "qwerty123", defaultTrace));
  ASSERT_EQ(getTokens(R"("escaped quote \" bla ")")[0], Token(TT::STRING, "escaped quote \" bla ", defaultTrace));
  ASSERT_THROW(getTokens(R"("querty123)")[0], Error);
}

TEST_F(LexerTest, EscapeSequences) {
  ASSERT_THROW(getTokens(R"("cool string here"  \n )"), Error);
  ASSERT_EQ(getTokens(R"(" \n ")")[0], Token(TT::STRING, " \n ", defaultTrace));
  ASSERT_EQ(getTokens(R"(" \x0A ")")[0], Token(TT::STRING, " \n ", defaultTrace));
  ASSERT_EQ(getTokens(R"(" \075 ")")[0], Token(TT::STRING, " = ", defaultTrace));
  ASSERT_EQ(getTokens(R"(" \\ ")")[0], Token(TT::STRING, " \\ ", defaultTrace));
  ASSERT_EQ(getTokens(R"(" \" ")")[0], Token(TT::STRING, " \" ", defaultTrace));
}

TEST_F(LexerTest, Constructs) {
  ASSERT_EQ(getTokens(";")[0], Token(TT::SEMI, ";", defaultTrace));
  ASSERT_THROW(getTokens("]"), Error);
  ASSERT_THROW(getTokens("([)]"), Error);
  ASSERT_NO_THROW(getTokens("([])"));
}

TEST_F(LexerTest, FatArrow) {
  ASSERT_EQ(getTokens("=>")[0], Token(TT::FAT_ARROW, "=>", defaultTrace));
}

TEST_F(LexerTest, Operators) {
  ASSERT_EQ(getTokens("!=")[0], Token(TT::OPERATOR, 1, defaultTrace));
  ASSERT_EQ(getTokens("1 + 1")[1], Token(TT::OPERATOR, 31, defaultTrace));
  ASSERT_EQ(getTokens("+ 1")[0], Token(TT::OPERATOR, 25, defaultTrace));
  ASSERT_EQ(getTokens("++1")[0], Token(TT::OPERATOR, 23, defaultTrace));
  EXPECT_EQ(getTokens("1++ + 2")[1], Token(TT::OPERATOR, 20, defaultTrace));
  ASSERT_EQ(at(2), Token(TT::OPERATOR, 31, defaultTrace));
  EXPECT_EQ(getTokens("1++ + ++2")[3], Token(TT::OPERATOR, 23, defaultTrace));
  ASSERT_EQ(at(2), Token(TT::OPERATOR, 31, defaultTrace));
}

TEST_F(LexerTest, Keywords) {
  ASSERT_EQ(getTokens("define")[0], Token(TT::DEFINE, "define", defaultTrace));
  ASSERT_EQ(getTokens("function")[0], Token(TT::FUNCTION, "function", defaultTrace));
  ASSERT_EQ(getTokens("protected")[0], Token(TT::PROTECT, "protected", defaultTrace));
}

TEST_F(LexerTest, Expression) {
  getTokens("(-12 + -3) / 1.5 >> 1");
  ASSERT_EQ(at(0), Token(TT::PAREN_LEFT, "(", defaultTrace));
  ASSERT_EQ(at(1), Token(TT::OPERATOR, 24, defaultTrace));
  ASSERT_EQ(at(2), Token(TT::INTEGER, "12", defaultTrace));
  ASSERT_EQ(at(5), Token(TT::INTEGER, "3", defaultTrace));
  ASSERT_EQ(at(6), Token(TT::PAREN_RIGHT, ")", defaultTrace));
  ASSERT_EQ(at(8), Token(TT::FLOAT, "1.5", defaultTrace));
  ASSERT_EQ(at(10), Token(TT::INTEGER, "1", defaultTrace));
}
