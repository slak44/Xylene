#include <gtest/gtest.h>
#include <vector>

#include "utils/util.hpp"
#include "lexer.hpp"
#include "token.hpp"

class LexerTest: public ::testing::Test {
protected:
  Lexer lx = Lexer();
  const std::vector<Token>& getTokens(std::string code) {
    return lx.tokenize(code, "<lexer-test>").getTokens();
  }
};

TEST_F(LexerTest, CommentsSingleLine) {
  ASSERT_EQ(getTokens("// test\n"), std::vector<Token> {Token(FILE_END, "", defaultTrace)});
  ASSERT_EQ(getTokens("// asd 123 . ////**//"), std::vector<Token> {Token(FILE_END, "", defaultTrace)});
  ASSERT_EQ(lx.getLineCount(), 1);
}

TEST_F(LexerTest, CommentsMultiLine) {
  ASSERT_EQ(getTokens("/*asdad\ndasd\nasd*/"), std::vector<Token> {Token(FILE_END, "", defaultTrace)});
  ASSERT_EQ(lx.getLineCount(), 3);
}

TEST_F(LexerTest, IntegerLiterals) {
  EXPECT_EQ(getTokens("123")[0], Token(L_INTEGER, "123", defaultTrace));
  EXPECT_EQ(getTokens("0xA")[0], Token(L_INTEGER, "10", defaultTrace));
  EXPECT_EQ(getTokens("0o10")[0], Token(L_INTEGER, "8", defaultTrace));
  EXPECT_EQ(getTokens("0b10")[0], Token(L_INTEGER, "2", defaultTrace));
}

TEST_F(LexerTest, Radix) {
  EXPECT_THROW(getTokens("0j123")[0], Error);
  EXPECT_THROW(getTokens("0123")[0], Error);
  EXPECT_THROW(getTokens("0x0123")[0], Error);
  EXPECT_EQ(getTokens("0")[0], Token(L_INTEGER, "0", defaultTrace));
}

TEST_F(LexerTest, FloatLiterals) {
  EXPECT_EQ(getTokens("12.3")[0], Token(L_FLOAT, "12.3", defaultTrace));
  EXPECT_THROW(getTokens("12.")[0], Error);
  EXPECT_THROW(getTokens("12.123.")[0], Error);
  EXPECT_THROW(getTokens("0x12.123")[0], Error);
}

TEST_F(LexerTest, BooleanLiterals) {
  EXPECT_EQ(getTokens("true")[0], Token(L_BOOLEAN, "true", defaultTrace));
  EXPECT_EQ(getTokens("false")[0], Token(L_BOOLEAN, "false", defaultTrace));
}

TEST_F(LexerTest, StringLiterals) {
  EXPECT_EQ(getTokens("\"qwerty123\"")[0], Token(L_STRING, "qwerty123", defaultTrace));
  EXPECT_THROW(getTokens("\"qwerty123")[0], Error);
}

TEST_F(LexerTest, EscapeSequences) {
  EXPECT_THROW(getTokens("\"cool string here\"  \\n "), Error);
  EXPECT_EQ(getTokens("\" \\n \"")[0], Token(L_STRING, " \n ", defaultTrace));
  // FIXME
  // EXPECT_EQ(getTokens("\" \\x0A \"")[0], Token(L_STRING, " \n ", defaultTrace));
  // EXPECT_EQ(getTokens("\" \\075 \"")[0], Token(L_STRING, " = ", defaultTrace));
  EXPECT_EQ(getTokens("\" \\\\ \"")[0], Token(L_STRING, " \\ ", defaultTrace));
  EXPECT_EQ(getTokens("\" \\\" \"")[0], Token(L_STRING, " \" ", defaultTrace));
}

TEST_F(LexerTest, Constructs) {
  EXPECT_EQ(getTokens(";")[0], Token(C_SEMI, ";", defaultTrace));
  EXPECT_EQ(getTokens("]")[0], Token(C_SQPAREN_RIGHT, "]", defaultTrace));
}

TEST_F(LexerTest, FatArrow) {
  ASSERT_EQ(getTokens("=>")[0], Token(K_FAT_ARROW, "=>", defaultTrace));
}

TEST_F(LexerTest, Operators) {
  EXPECT_EQ(getTokens("!=")[0], Token(OPERATOR, 1, defaultTrace));
  EXPECT_EQ(getTokens("1 + 1")[1], Token(OPERATOR, 31, defaultTrace));
  EXPECT_EQ(getTokens("+ 1")[0], Token(OPERATOR, 25, defaultTrace));
  EXPECT_EQ(getTokens("++1")[0], Token(OPERATOR, 23, defaultTrace));
  EXPECT_EQ(getTokens("1++ + 2")[1], Token(OPERATOR, 20, defaultTrace));
  EXPECT_EQ(lx[2], Token(OPERATOR, 31, defaultTrace));
  EXPECT_EQ(getTokens("1++ + ++2")[3], Token(OPERATOR, 23, defaultTrace));
  EXPECT_EQ(lx[2], Token(OPERATOR, 31, defaultTrace));
}

TEST_F(LexerTest, Keywords) {
  EXPECT_EQ(getTokens("define")[0], Token(K_DEFINE, "define", defaultTrace));
  EXPECT_EQ(getTokens("function")[0], Token(K_FUNCTION, "function", defaultTrace));
  EXPECT_EQ(getTokens("protected")[0], Token(K_PROTECT, "protected", defaultTrace));
}

TEST_F(LexerTest, Expression) {
  getTokens("(-12 + -3) / 1.5 >> 1");
  EXPECT_EQ(lx[0], Token(C_PAREN_LEFT, "(", defaultTrace));
  EXPECT_EQ(lx[1], Token(OPERATOR, 24, defaultTrace));
  EXPECT_EQ(lx[2], Token(L_INTEGER, "12", defaultTrace));
  EXPECT_EQ(lx[5], Token(L_INTEGER, "3", defaultTrace));
  EXPECT_EQ(lx[6], Token(C_PAREN_RIGHT, ")", defaultTrace));
  EXPECT_EQ(lx[8], Token(L_FLOAT, "1.5", defaultTrace));
  EXPECT_EQ(lx[10], Token(L_INTEGER, "1", defaultTrace));
}
