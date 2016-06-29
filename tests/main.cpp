#include <gtest/gtest.h>

#include "lexerTest.hpp"
#include "parserTest.hpp"

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
