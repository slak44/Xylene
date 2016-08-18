#include <tclap/CmdLine.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "llvm/globalTypes.hpp"
#include "llvm/compiler.hpp"
#include "interpreter/interpreter.hpp"

int main(int argc, const char* argv[]) {
  try {
    TCLAP::CmdLine cmd("test-lang", ' ', "pre-release");
    
    TCLAP::SwitchArg printTokens("", "tokens", "Print token list", cmd);
    TCLAP::SwitchArg printAST("", "ast", "Print AST", cmd);
    TCLAP::SwitchArg printIR("", "ir", "Print LLVM IR (if applicable)", cmd);
    
    TCLAP::SwitchArg doNotParse("", "no-parse", "Don't parse the token list", cmd);
    TCLAP::SwitchArg doNotRun("", "no-run", "Don't execute the AST", cmd);
    
    std::vector<std::string> runnerValues {"llvm-lli", "llvm-llc", "tree-walk"};
    TCLAP::ValuesConstraint<std::string> runnerConstraint(runnerValues);
    TCLAP::ValueArg<std::string> runner("r", "runner", "How to run this code", false, "llvm-lli", &runnerConstraint, cmd);
    
    TCLAP::ValueArg<std::string> code("e", "eval", "Code to evaluate", true, std::string(), "string", cmd);
    cmd.parse(argc, argv);
    
    auto lx = Lexer();
    lx.tokenize(code.getValue());
    if (printTokens.getValue()) for (auto tok : lx.getTokens()) println(tok);
    
    if (doNotParse.getValue()) return 0;
    auto px = TokenParser();
    px.parse(lx.getTokens());
    if (printAST.getValue()) px.getTree().print();
    
    if (doNotRun.getValue()) return 0;
    
    if (runner.getValue() == "tree-walk") {
      auto in = TreeWalkInterpreter();
      in.interpret(px.getTree());
      return 0;
    }
    
    CompileVisitor::Link v = CompileVisitor::create(globalContext, "Command Line Module", px.getTree());
    v->visit();
    if (printIR.getValue()) v->getModule()->dump();
    
    if (runner.getValue() == "llvm-lli") {
      std::string onError = "";
      auto eb = new llvm::EngineBuilder(v->getModule());
      llvm::ExecutionEngine* ee = eb
        ->setEngineKind(llvm::EngineKind::Interpreter)
        .setErrorStr(&onError)
        .create();
      auto main = v->getEntryPoint();
      if (onError != "") throw InternalError("ExecutionEngine error", {
        METADATA_PAIRS,
        {"supplied error string", onError}
      });
      return ee->runFunctionAsMain(main, {}, {});
    } else if (runner.getValue() == "llvm-llc") {
      println("Not yet implemented!");
      // TODO
    }
  } catch (TCLAP::ArgException& arg) {
    std::cerr << "TCLAP error " << arg.error() << " for " << arg.argId() << std::endl;
  }
  return 0;
}
