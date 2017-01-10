#include <gtest/gtest.h>
#include <vector>

#include "utils/util.hpp"
#include "lexer.hpp"
#include "token.hpp"

class LexerTest: public ::testing::Test {
protected:
  std::unique_ptr<Lexer> lx;
  const std::vector<Token>& getTokens(std::string code) {
    lx = Lexer::tokenize(code, "<lexer-test>");
    return lx->getTokens();
  }
  Token at(std::size_t pos) {
    return (*lx)[pos];
  }
};

TEST_F(LexerTest, CommentsSingleLine) {
  ASSERT_EQ(getTokens("// test\n"), std::vector<Token> {Token(TT::FILE_END, "", defaultTrace)});
  ASSERT_EQ(getTokens("// asd 123 . ////**//"), std::vector<Token> {Token(TT::FILE_END, "", defaultTrace)});
  ASSERT_EQ(lx->getLineCount(), 1);
}

TEST_F(LexerTest, CommentsMultiLine) {
  ASSERT_EQ(getTokens("/*asdad\ndasd\nasd*/"), std::vector<Token> {Token(TT::FILE_END, "", defaultTrace)});
  ASSERT_EQ(lx->getLineCount(), 3);
}

TEST_F(LexerTest, IntegerLiterals) {
  EXPECT_EQ(getTokens("123")[0], Token(TT::INTEGER, "123", defaultTrace));
  EXPECT_EQ(getTokens("0xA")[0], Token(TT::INTEGER, "10", defaultTrace));
  EXPECT_EQ(getTokens("0o10")[0], Token(TT::INTEGER, "8", defaultTrace));
  EXPECT_EQ(getTokens("0b10")[0], Token(TT::INTEGER, "2", defaultTrace));
}

TEST_F(LexerTest, Radix) {
  EXPECT_THROW(getTokens("0j123")[0], Error);
  EXPECT_THROW(getTokens("0123")[0], Error);
  EXPECT_THROW(getTokens("0x0123")[0], Error);
  EXPECT_EQ(getTokens("0")[0], Token(TT::INTEGER, "0", defaultTrace));
}

TEST_F(LexerTest, FloatLiterals) {
  EXPECT_EQ(getTokens("12.3")[0], Token(TT::FLOAT, "12.3", defaultTrace));
  EXPECT_THROW(getTokens("12.")[0], Error);
  EXPECT_THROW(getTokens("12.123.")[0], Error);
  EXPECT_THROW(getTokens("0x12.123")[0], Error);
}

TEST_F(LexerTest, BooleanLiterals) {
  EXPECT_EQ(getTokens("true")[0], Token(TT::BOOLEAN, "true", defaultTrace));
  EXPECT_EQ(getTokens("false")[0], Token(TT::BOOLEAN, "false", defaultTrace));
}

TEST_F(LexerTest, StringLiterals) {
  EXPECT_EQ(getTokens("\"qwerty123\"")[0], Token(TT::STRING, "qwerty123", defaultTrace));
  EXPECT_THROW(getTokens("\"qwerty123")[0], Error);
}

TEST_F(LexerTest, EscapeSequences) {
  EXPECT_THROW(getTokens("\"cool string here\"  \\n "), Error);
  EXPECT_EQ(getTokens("\" \\n \"")[0], Token(TT::STRING, " \n ", defaultTrace));
  // TODO
  // EXPECT_EQ(getTokens("\" \\x0A \"")[0], Token(TT::STRING, " \n ", defaultTrace));
  // EXPECT_EQ(getTokens("\" \\075 \"")[0], Token(TT::STRING, " = ", defaultTrace));
  EXPECT_EQ(getTokens("\" \\\\ \"")[0], Token(TT::STRING, " \\ ", defaultTrace));
  EXPECT_EQ(getTokens("\" \\\" \"")[0], Token(TT::STRING, " \" ", defaultTrace));
}

TEST_F(LexerTest, Constructs) {
  EXPECT_EQ(getTokens(";")[0], Token(TT::SEMI, ";", defaultTrace));
  EXPECT_EQ(getTokens("]")[0], Token(TT::SQPAREN_RIGHT, "]", defaultTrace));
}

TEST_F(LexerTest, FatArrow) {
  ASSERT_EQ(getTokens("=>")[0], Token(TT::FAT_ARROW, "=>", defaultTrace));
}

TEST_F(LexerTest, Operators) {
  EXPECT_EQ(getTokens("!=")[0], Token(TT::OPERATOR, 1, defaultTrace));
  EXPECT_EQ(getTokens("1 + 1")[1], Token(TT::OPERATOR, 31, defaultTrace));
  EXPECT_EQ(getTokens("+ 1")[0], Token(TT::OPERATOR, 25, defaultTrace));
  EXPECT_EQ(getTokens("++1")[0], Token(TT::OPERATOR, 23, defaultTrace));
  EXPECT_EQ(getTokens("1++ + 2")[1], Token(TT::OPERATOR, 20, defaultTrace));
  EXPECT_EQ(at(2), Token(TT::OPERATOR, 31, defaultTrace));
  EXPECT_EQ(getTokens("1++ + ++2")[3], Token(TT::OPERATOR, 23, defaultTrace));
  EXPECT_EQ(at(2), Token(TT::OPERATOR, 31, defaultTrace));
}

TEST_F(LexerTest, Keywords) {
  EXPECT_EQ(getTokens("define")[0], Token(TT::DEFINE, "define", defaultTrace));
  EXPECT_EQ(getTokens("function")[0], Token(TT::FUNCTION, "function", defaultTrace));
  EXPECT_EQ(getTokens("protected")[0], Token(TT::PROTECT, "protected", defaultTrace));
}

TEST_F(LexerTest, Expression) {
  getTokens("(-12 + -3) / 1.5 >> 1");
  EXPECT_EQ(at(0), Token(TT::PAREN_LEFT, "(", defaultTrace));
  EXPECT_EQ(at(1), Token(TT::OPERATOR, 24, defaultTrace));
  EXPECT_EQ(at(2), Token(TT::INTEGER, "12", defaultTrace));
  EXPECT_EQ(at(5), Token(TT::INTEGER, "3", defaultTrace));
  EXPECT_EQ(at(6), Token(TT::PAREN_RIGHT, ")", defaultTrace));
  EXPECT_EQ(at(8), Token(TT::FLOAT, "1.5", defaultTrace));
  EXPECT_EQ(at(10), Token(TT::INTEGER, "1", defaultTrace));
}
