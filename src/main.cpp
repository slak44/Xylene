#include <tclap/CmdLine.h>

#include "utils/util.hpp"
#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "interpreter.hpp"

int main(int argc, const char* argv[]) {
  try {
    TCLAP::CmdLine cmd("test-lang", ' ', "0.1.0");
    TCLAP::SwitchArg printTokens("", "tokens", "Print token list", cmd);
    TCLAP::SwitchArg printAST("", "ast", "Print AST", cmd);
    TCLAP::SwitchArg doNotParse("", "no-parse", "Don't parse the token list", cmd);
    TCLAP::SwitchArg doNotInterpret("", "no-interpret", "Don't interpret the AST", cmd);
    TCLAP::ValueArg<std::string> code("e", "eval", "Code to evaluate", true, std::string(), "string", cmd);
    cmd.parse(argc, argv);
    
    auto lx = Lexer();
    lx.tokenize(code.getValue());
    if (printTokens.getValue()) for (auto tok : lx.getTokens()) println(tok);
    
    if (doNotParse.getValue()) return 0;
    auto px = TokenParser();
    px.parse(lx.getTokens());
    if (printAST.getValue()) px.getTree().print();
    
    if (doNotInterpret.getValue()) return 0;
    auto in = Interpreter();
    in.interpret(px.getTree());
    
  } catch (TCLAP::ArgException& arg) {
    std::cerr << "TCLAP error " << arg.error() << " for " << arg.argId() << std::endl;
  }
  return 0;
}
