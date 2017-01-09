add_library(runtime_lib STATIC
  ${SRC_DIR}/utils/util.cpp
  ${SRC_DIR}/runtime/runtime.cpp
  ${SRC_DIR}/runtime/io.cpp
)

add_executable(xylene_bin ${COMMON_SOURCES} main.cpp)
target_link_libraries(xylene_bin ${COMMON_LINK_LIBS})
add_dependencies(xylene_bin ${COMMON_DEPS} runtime_lib)

make_exe_symlink(xylene)
