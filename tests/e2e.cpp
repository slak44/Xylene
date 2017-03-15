#include <gtest/gtest.h>

#include "test.hpp"
#include "llvm/runner.hpp"

class E2ETest: public ::testing::Test, public ExternalProcessCompiler {
};

TEST_F(E2ETest, PrintAlphabet) {
  if (spawnProcs) EXPECT_EQ(
    compileAndRun("data/end-to-end/alphabet.xylene"),
    ProgramResult({0, "abcdefghijklmnopqrstuvwxyz", ""})
  );
}
