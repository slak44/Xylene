#include <gtest/gtest.h>
#include <tclap/CmdLine.h>

#pragma GCC diagnostic push
#ifdef __clang__
  #pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

bool printIr = false;

#pragma GCC diagnostic pop

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  TCLAP::CmdLine cmd("", ' ', "");
  TCLAP::SwitchArg printIrArg("", "ir", "Print LLVM IR where available", cmd);
  cmd.parse(argc, argv);
  printIr = printIrArg.getValue();
  return RUN_ALL_TESTS();
}
