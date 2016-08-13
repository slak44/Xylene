cmake_minimum_required(VERSION 3.6.1)
project(test-lang)

subdirs(src tests)

set(TCLAP_PATH ${PROJECT_SOURCE_DIR}/libs/tclap-1.2.1)
set(GTEST_PATH ${PROJECT_SOURCE_DIR}/libs/googletest-release-1.7.0)
set(RAPIDXML_PATH ${PROJECT_SOURCE_DIR}/libs/rapidxml-1.13)
set(TERMCOLOR_PATH ${PROJECT_SOURCE_DIR}/libs/termcolor)

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
include_directories(SYSTEM ${TCLAP_PATH}/include)
include_directories(SYSTEM ${GTEST_PATH}/include)
include_directories(SYSTEM ${RAPIDXML_PATH})
include_directories(SYSTEM ${TERMCOLOR_PATH}/include)

link_directories(${GTEST_PATH})

function(make_exe_symlink target_name)
  # Symlink the exe to the project root
  add_custom_command(TARGET ${target_name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/bin/${target_name} ${PROJECT_SOURCE_DIR}/lg)
endfunction(make_exe_symlink)
