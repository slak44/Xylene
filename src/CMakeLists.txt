add_executable(release_bin ${COMMON_SOURCES} main.cpp)
add_executable(debug_bin ${COMMON_SOURCES} main.cpp)
add_library(runtime_lib STATIC
  ${SRC_DIR}/utils/util.cpp
  ${SRC_DIR}/runtime/runtime.cpp
  ${SRC_DIR}/runtime/io.cpp
)

if(UNIX)
  target_compile_options(release_bin PRIVATE -O3 -g0 -Wdisabled-optimization)
  target_compile_options(debug_bin PRIVATE -D CRASH_ON_INTERNAL_ERROR -O0 -g3)
  if(XYLENE_MEASURE_TIME)
    target_compile_options(release_bin PRIVATE -D XYLENE_MEASURE_TIME)
  endif()
elseif(MSVC)
  target_compile_options(release_bin PRIVATE)
  target_compile_options(debug_bin PRIVATE /DCRASH_ON_INTERNAL_ERROR)
  if(XYLENE_MEASURE_TIME)
    target_compile_options(release_bin PRIVATE /DXYLENE_MEASURE_TIME)
  endif()
endif()

target_link_libraries(release_bin ${COMMON_LINK_LIBS})
target_link_libraries(debug_bin ${COMMON_LINK_LIBS})

add_dependencies(release_bin ${COMMON_DEPS} runtime_lib)
add_dependencies(debug_bin ${COMMON_DEPS} runtime_lib)

make_exe_symlink(release)
make_exe_symlink(debug)
