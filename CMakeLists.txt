cmake_minimum_required(VERSION 3.6.1)
project(test-lang)

subdirs(src tests)

include(ExternalProject)

set(GTEST_PREFIX ${PROJECT_SOURCE_DIR}/libs/googletest)
ExternalProject_Add(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.7.0
  PREFIX ${GTEST_PREFIX}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)
include_directories(SYSTEM ${GTEST_PREFIX}/src/googletest/include)
link_directories(${GTEST_PREFIX}/src/googletest-build)

set(RAPIDXML_PREFIX ${PROJECT_SOURCE_DIR}/libs/rapidxml)
ExternalProject_Add(
  rapidxml
  URL http://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
  URL_MD5 7b4b42c9331c90aded23bb55dc725d6a
  PREFIX ${RAPIDXML_PREFIX}
  CONFIGURE_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_COMMAND ""
)
include_directories(SYSTEM ${RAPIDXML_PREFIX}/src/rapidxml)

set(TERMCOLOR_PREFIX ${PROJECT_SOURCE_DIR}/libs/termcolor)
ExternalProject_Add(
  termcolor
  GIT_REPOSITORY https://github.com/ikalnitsky/termcolor.git
  GIT_TAG 252e16600f6ed8a83d6b0e92d03c292bb416e900
  PREFIX ${TERMCOLOR_PREFIX}
  CONFIGURE_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_COMMAND ""
  PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/stringstream_segfault_hack.patch
)
include_directories(SYSTEM ${TERMCOLOR_PREFIX}/src/termcolor/include)

set(TCLAP_PREFIX ${PROJECT_SOURCE_DIR}/libs/tclap)
ExternalProject_Add(
  tclap
  GIT_REPOSITORY http://git.code.sf.net/p/tclap/code
  GIT_TAG tclap-1-2-1-release-final
  PREFIX ${TCLAP_PREFIX}
  CONFIGURE_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_COMMAND ""
)
include_directories(SYSTEM ${TCLAP_PREFIX}/src/tclap/include)

set(TINY_PROC_LIB_PREFIX ${PROJECT_SOURCE_DIR}/libs/tiny-process-library)
ExternalProject_Add(
  tiny-process-library
  GIT_REPOSITORY https://github.com/eidheim/tiny-process-library.git
  GIT_TAG v1.0.5
  PREFIX ${TINY_PROC_LIB_PREFIX}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)
include_directories(SYSTEM ${TINY_PROC_LIB_PREFIX}/src)
link_directories(${TINY_PROC_LIB_PREFIX}/src/tiny-process-library-build)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_SOURCES
  ${SRC_DIR}/utils/util.cpp
  ${SRC_DIR}/utils/trace.cpp
  ${SRC_DIR}/utils/error.cpp
  ${SRC_DIR}/utils/typeInfo.cpp
  ${SRC_DIR}/operator.cpp
  ${SRC_DIR}/tokenType.cpp
  ${SRC_DIR}/token.cpp
  ${SRC_DIR}/lexer.cpp
  ${SRC_DIR}/ast.cpp
  ${SRC_DIR}/ast.overrides.cpp
  ${SRC_DIR}/parser/tokenParser.cpp
  ${SRC_DIR}/parser/xmlParser.cpp
  ${SRC_DIR}/llvm/compiler.cpp
  ${SRC_DIR}/llvm/runner.cpp
  ${SRC_DIR}/llvm/codegen.cpp
  ${PROJECT_SOURCE_DIR}/stdlib/src/io.cpp
)
set(COMMON_DEPS rapidxml termcolor)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(LLVM)
if(NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM package can't be found. Set CMAKE_PREFIX_PATH variable to LLVM's installation prefix.")
endif()
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
llvm_map_components_to_libnames(
  REQ_LLVM_LIBRARIES
  support core TransformUtils ExecutionEngine Interpreter MCJIT Target Analysis RuntimeDyld Object MCParser BitReader MC
  X86Info X86AsmParser X86AsmPrinter X86Utils X86CodeGen
)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/stdlib/include)

add_custom_target(clean-all)

foreach(folder ${PROJECT_SOURCE_DIR}/bin ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/libs ${PROJECT_SOURCE_DIR}/lg)
  add_custom_command(TARGET clean-all PRE_BUILD COMMAND ${CMAKE_COMMAND} -E remove_directory ${folder})
endforeach(folder)
add_custom_command(TARGET clean-all PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/build)

function(make_exe_symlink target_name)
  set(SYMLINK_LOCATION ${PROJECT_SOURCE_DIR}/lg)
  add_custom_target(${target_name})
  add_dependencies(${target_name} ${target_name}_bin)
  # Remove old link
  add_custom_command(TARGET ${target_name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove ${SYMLINK_LOCATION})
  # Symlink the exe to the project root
  add_custom_command(TARGET ${target_name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/bin/${target_name} ${SYMLINK_LOCATION})
endfunction(make_exe_symlink)
