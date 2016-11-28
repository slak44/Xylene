#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <tiny-process-library/process.hpp>

#include "llvm/runner.hpp"

extern "C" bool printIr;

namespace fs = std::experimental::filesystem;

class E2ETest: public ::testing::Test {
protected:
  inline ProgramResult compileAndRun(fs::path file) {
    std::string stdout, stderr;
    Process self(
      std::string(FULL_PROGRAM_PATH) + " -f " +
      (DATA_PARENT_DIR/file).c_str() + (printIr ? " --ir" : ""),
      "",
      [&stdout](const char* bytes, std::size_t) {
        stdout = bytes;
      },
      [&stderr](const char* bytes, std::size_t) {
        stderr = bytes;
      }
    );
    auto exitCode = self.get_exit_status();
    return {exitCode, stdout, stderr};
  }
};

TEST_F(E2ETest, PrintAlphabet) {
  EXPECT_EQ(
    compileAndRun("data/end-to-end/alphabet.xylene"),
    ProgramResult({0, "abcdefghijklmnopqrstuvwxyz", ""})
  );
}
