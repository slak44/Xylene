add_executable(tests main.cpp)

set(TEST_DATA ${PROJECT_SOURCE_DIR}/tests/data)

include_directories(${PROJECT_SOURCE_DIR}/tests)

target_compile_options(tests PRIVATE -D DEBUG -DDATA_PARENT_DIR=\"$<TARGET_FILE_DIR:tests>\" -O0 -g3)
target_link_libraries(tests ${REQ_LLVM_LIBRARIES} pthread gtest)

# Copy the folder with test data to the executable
add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEST_DATA} $<TARGET_FILE_DIR:tests>/data)

add_dependencies(tests googletest ${COMMON_DEPS})

make_exe_symlink(tests)
