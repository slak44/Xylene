#include "runtime/runtime.hpp"

void finish(char* message, int exitCode) {
  println(message);
  std::exit(exitCode);
}
