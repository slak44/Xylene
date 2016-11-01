add_executable(release_bin ${COMMON_SOURCES} main.cpp)

target_compile_options(release_bin PRIVATE -O3 -g0 -Wdisabled-optimization)
target_link_libraries(release_bin ${REQ_LLVM_LIBRARIES})

add_dependencies(release_bin ${COMMON_DEPS})

make_exe_symlink(release)

add_executable(debug_bin ${COMMON_SOURCES} main.cpp)

target_compile_options(debug_bin PRIVATE -D CRASH_ON_INTERNAL_ERROR -O0 -g3)
target_link_libraries(debug_bin ${REQ_LLVM_LIBRARIES})

add_dependencies(debug_bin ${COMMON_DEPS})

make_exe_symlink(debug)
