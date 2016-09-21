#include <gtest/gtest.h>
#include <tclap/CmdLine.h>

bool printIr = false;

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  TCLAP::CmdLine cmd("", ' ', "");
  TCLAP::SwitchArg printIrArg("", "ir", "Print LLVM IR where available", cmd);
  cmd.parse(argc, argv);
  printIr = printIrArg.getValue();
  return RUN_ALL_TESTS();
}
