add_executable(lg main.cpp)

target_compile_options(lg PRIVATE -D NDEBUG -O3 -g0)
target_link_libraries(lg ${REQ_LLVM_LIBRARIES})

add_dependencies(lg googletest)

make_exe_symlink(lg)

add_executable(debug main.cpp)

target_compile_options(debug PRIVATE -D DEBUG -O0 -g3)
target_link_libraries(debug ${REQ_LLVM_LIBRARIES})

add_dependencies(debug googletest)

make_exe_symlink(debug)
