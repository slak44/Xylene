#ifndef TEST_HEADER_INCLUDE_HPP
#define TEST_HEADER_INCLUDE_HPP

#include <tiny-process-library/process.hpp>
#include <fmt/format.h>

#include "llvm/runner.hpp"
#include "utils/util.hpp"

extern bool printIr;
extern bool spawnProcs;

class ExternalProcessCompiler {
private:
  fs::path programPath;
  fs::path dataDirPath;
public:
  ExternalProcessCompiler() {
    programPath = FULL_PROGRAM_PATH;
    dataDirPath = DATA_PARENT_DIR;
    if (!fs::exists(programPath)) throw InternalError("Executable is missing", {
      METADATA_PAIRS,
      {"path", programPath},
      {"tip", "maybe the xylene target hasn't been built"}
    });
    if (!fs::exists(dataDirPath)) throw InternalError("Missing data directory", {
      METADATA_PAIRS,
      {"path", dataDirPath}
    });
  }
  
  ProgramResult compileAndRun(fs::path relativePath, std::string extraArgs = "", bool disableIr = false) {
    fs::path filePath = dataDirPath / relativePath;
    if (!fs::exists(filePath)) throw InternalError("Cannot run missing file", {
      METADATA_PAIRS,
      {"path", filePath}
    });
    
    std::string stdout, stderr;
    std::string command = fmt::format(
      "{0} -f {1} {2}", programPath, filePath, extraArgs);
    Process self(
      command,
      "",
      [&stdout](const char* bytes, std::size_t) {
        stdout = bytes;
      },
      [&stderr](const char* bytes, std::size_t) {
        stderr = bytes;
      }
    );
    if (printIr && !disableIr) printIrFor(relativePath, extraArgs);
    auto exitCode = self.get_exit_status();
    return {exitCode, stdout, stderr};
  }
  
  ProgramResult printIrFor(fs::path relativePath, std::string extraArgs = "") {
    return compileAndRun(relativePath, extraArgs + " --ir --no-run", true);
  }
};

#endif
