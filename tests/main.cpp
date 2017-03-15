#include <gtest/gtest.h>
#include <tclap/CmdLine.h>

#include "test.hpp"
#include "utils/util.hpp"

bool printIr = false;
bool spawnProcs = true;

int main(int argc, char* argv[]) {
  TCLAP::CmdLine cmd("", ' ', "");
  TCLAP::SwitchArg disableProcessSpawn("", "no-procs", "Don't run tests who spawn new processes", cmd);
  TCLAP::SwitchArg printIrArg("", "ir", "Print LLVM IR where available", cmd);
  cmd.parse(argc, argv);
  
  printIr = printIrArg.getValue();
  spawnProcs = !disableProcessSpawn.getValue();
  
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
