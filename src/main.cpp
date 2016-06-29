#include <tclap/CmdLine.h>

#include "util.hpp"
#include "lexer.hpp"
#include "parser.hpp"

void run(std::string code, bool printTokens, bool doNotParse) {
  auto lx = Lexer();
  lx.tokenize(code);
  if (printTokens) for (auto tok : lx.getTokens()) println(tok);
  if (doNotParse) return;
  auto px = Parser();
  px.parse(lx.getTokens());
  px.getTree().root.printTree(0);
}

int main(int argc, const char* argv[]) {
  try {
    TCLAP::CmdLine cmd("test-lang", ' ', "0.1.0");
    TCLAP::SwitchArg printTokens("t", "tokens", "Print token list", cmd);
    TCLAP::SwitchArg doNotParse("", "no-parse", "Don't parse the token list", cmd);
    TCLAP::ValueArg<std::string> code("e", "eval", "Code to evaluate", true, std::string(), "string", cmd);
    cmd.parse(argc, argv);
    run(code.getValue(), printTokens.getValue(), doNotParse.getValue());
  } catch (TCLAP::ArgException& arg) {
    std::cerr << "TCLAP error " << arg.error() << " for " << arg.argId() << std::endl;
  }
  return 0;
}
