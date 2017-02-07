add_executable(tests_bin
  ${COMMON_SOURCES}
  main.cpp
  utilTest.cpp
  lexerTest.cpp
  parserTest.cpp
  llvmTest.cpp
  e2e.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/tests)

if(MSVC)
  target_compile_options(tests_bin PRIVATE
    /DCRASH_ON_INTERNAL_ERROR
    /DXYLENE_BLOCK_RETURN
    /DFULL_PROGRAM_PATH=\"$<TARGET_FILE:xylene_bin>\"
    /DDATA_PARENT_DIR=\"$<TARGET_FILE_DIR:tests_bin>\"
  )
else()
  target_compile_options(tests_bin PRIVATE
    -DCRASH_ON_INTERNAL_ERROR
    -DXYLENE_BLOCK_RETURN
    -DFULL_PROGRAM_PATH=\"$<TARGET_FILE:xylene_bin>\"
    -DDATA_PARENT_DIR=\"$<TARGET_FILE_DIR:tests_bin>\"
  )
endif()

target_link_libraries(tests_bin ${COMMON_LINK_LIBS} pthread gtest libtiny-process-library.a)

# Copy the folder with test data to the executable
add_custom_command(TARGET tests_bin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/tests/data $<TARGET_FILE_DIR:tests_bin>/data)

add_dependencies(tests_bin ${COMMON_DEPS} googletest tiny-process-library xylene)

make_exe_symlink(tests)
