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
  # TODO apply patch
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

set(COMMON_DEPS rapidxml termcolor)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(LLVM)
if(NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM package can't be found. Set CMAKE_PREFIX_PATH variable to LLVM's installation prefix.")
endif()
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES support core)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/libs)

function(make_exe_symlink target_name)
  # Symlink the exe to the project root
  add_custom_command(TARGET ${target_name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/bin/${target_name} ${PROJECT_SOURCE_DIR}/lg)
endfunction(make_exe_symlink)
