add_executable(release ${COMMON_SOURCES} main.cpp)

target_compile_options(release PRIVATE -O3 -g0)
target_link_libraries(release ${REQ_LLVM_LIBRARIES})

add_dependencies(release ${COMMON_DEPS} tclap)

make_exe_symlink(release)

add_executable(debug ${COMMON_SOURCES} main.cpp)

target_compile_options(debug PRIVATE -O0 -g3)
target_link_libraries(debug ${REQ_LLVM_LIBRARIES})

add_dependencies(debug ${COMMON_DEPS} tclap)

make_exe_symlink(debug)
