#include <tclap/CmdLine.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/TargetSelect.h>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "llvm/globalTypes.hpp"
#include "llvm/compiler.hpp"
#include "interpreter/interpreter.hpp"

enum ExitCodes: int {
  NORMAL_EXIT = 0, // Everything is OK
  TCLAP_ERROR = 11, // TCLAP did something bad, should not happen
  CLI_ERROR = 1, // The user is retar-- uh I mean the user used a wrong option
  USER_PROGRAM_ERROR = 2, // The thing we had to lex/parse/run has an issue
  INTERNAL_ERROR = 3 // Unrecoverable error in this program, almost always a bug
};

int main(int argc, const char* argv[]) {
  try {
    TCLAP::CmdLine cmd("test-lang", ' ', "pre-release");
    
    TCLAP::SwitchArg printTokens("", "tokens", "Print token list", cmd);
    TCLAP::SwitchArg printAST("", "ast", "Print AST (if applicable)", cmd);
    TCLAP::SwitchArg printIR("", "ir", "Print LLVM IR (if applicable)", cmd);
    
    TCLAP::SwitchArg doNotParse("", "no-parse", "Don't parse the token list", cmd);
    TCLAP::SwitchArg doNotRun("", "no-run", "Don't execute the AST", cmd);
    
    std::vector<std::string> runnerValues {"llvm-lli", "llvm-llc", "tree-walk"};
    TCLAP::ValuesConstraint<std::string> runnerConstraint(runnerValues);
    TCLAP::ValueArg<std::string> runner("r", "runner", "How to run this code", false, "llvm-lli", &runnerConstraint, cmd);
    
    TCLAP::ValueArg<std::string> code("e", "eval", "Code to evaluate", false, std::string(), "string", cmd);
    TCLAP::ValueArg<std::string> filePath("f", "file", "Load code from this file", false, std::string(), "path", cmd);
    cmd.parse(argc, argv);
    
    if (code.getValue().empty() && filePath.getValue().empty()) {
      TCLAP::ArgException arg("Must specify either option -e or -f");
      cmd.getOutput()->failure(cmd, arg);
    }
    
    auto lx = Lexer();
    lx.tokenize(code.getValue());
    if (printTokens.getValue()) for (auto tok : lx.getTokens()) println(tok);
    
    if (doNotParse.getValue()) return NORMAL_EXIT;
    auto px = TokenParser();
    px.parse(lx.getTokens());
    if (printAST.getValue()) px.getTree().print();
    
    if (doNotRun.getValue()) return NORMAL_EXIT;
    
    if (runner.getValue() == "tree-walk") {
      auto in = TreeWalkInterpreter();
      in.interpret(px.getTree());
      return NORMAL_EXIT;
    }
    
    CompileVisitor::Link v = CompileVisitor::create(globalContext, "Command Line Module", px.getTree());
    v->visit();
    if (printIR.getValue()) v->getModule()->dump();
    
    if (runner.getValue() == "llvm-lli") {
      llvm::InitializeNativeTarget();
      llvm::InitializeNativeTargetAsmPrinter();
      llvm::InitializeNativeTargetAsmParser();

      std::string onError = "";
      auto eb = new llvm::EngineBuilder(PtrUtil<llvm::Module>::unique(v->getModule()));
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
  } catch (const TCLAP::ExitException& arg) {
    return CLI_ERROR;
  } catch (const TCLAP::ArgException& arg) {
    println("TCLAP error", arg.error(), "for", arg.argId());
    return TCLAP_ERROR;
  } catch (const Error& err) {
    println(err.what());
    return USER_PROGRAM_ERROR;
  } catch (const InternalError& err) {
    println(err.what());
    return INTERNAL_ERROR;
  }
  return 0;
}
