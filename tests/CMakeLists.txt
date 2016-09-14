add_executable(tests_bin
  ${COMMON_SOURCES}
  main.cpp
  utilTest.cpp
  lexerTest.cpp
  parserTest.cpp
  llvmTest.cpp
)

set(TEST_DATA ${PROJECT_SOURCE_DIR}/tests/data)

include_directories(${PROJECT_SOURCE_DIR}/tests)

target_compile_options(tests_bin PRIVATE
  -D CRASH_ON_INTERNAL_ERROR
  -DFULL_PROGRAM_PATH=\"$<TARGET_FILE:debug_bin>\"
  -DDATA_PARENT_DIR=\"$<TARGET_FILE_DIR:tests_bin>\"
  -O0 -g3
)
target_link_libraries(tests_bin ${REQ_LLVM_LIBRARIES} pthread gtest libtiny-process-library.a)

# Copy the folder with test data to the executable
add_custom_command(TARGET tests_bin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEST_DATA} $<TARGET_FILE_DIR:tests_bin>/data)

add_dependencies(tests_bin ${COMMON_DEPS} googletest tiny-process-library debug)

make_exe_symlink(tests)
