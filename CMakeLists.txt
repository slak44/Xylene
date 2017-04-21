cmake_minimum_required(VERSION 3.6.1)
project(Xylene)

subdirs(src tests)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14 /Wall")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /DDEBUG /DCRASH_ON_INTERNAL_ERROR")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Os /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Zi /DCRASH_ON_INTERNAL_ERROR")
  if(XYLENE_MEASURE_TIME)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DXYLENE_MEASURE_TIME")
  endif()
  if(XYLENE_BLOCK_RETURN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DXYLENE_BLOCK_RETURN")
  endif()
else()
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -pedantic -Wlogical-op -Wstrict-null-sentinel -Wnoexcept \
      -Wzero-as-null-pointer-constant -Wuseless-cast -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wfloat-equal -Wold-style-cast \
      -Winit-self -Wmissing-declarations -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wundef -Wstrict-overflow=5 \
      -Wsuggest-override")
  elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -stdlib=libstdc++ \
      -fshow-column -fcaret-diagnostics -fcolor-diagnostics \
      -Weverything -Wno-missing-braces -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded \
      -Wno-weak-vtables -Wno-global-constructors -Wno-exit-time-destructors -Wno-shadow-field-in-constructor -Wno-documentation-unknown-command \
      -Wno-used-but-marked-unused -Winconsistent-missing-override")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
  endif()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0 -DDEBUG -DCRASH_ON_INTERNAL_ERROR")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O4 -DNDEBUG -Wdisabled-optimization")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DCRASH_ON_INTERNAL_ERROR -Wdisabled-optimization")
  if(XYLENE_MEASURE_TIME)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXYLENE_MEASURE_TIME")
  endif()
  if(XYLENE_BLOCK_RETURN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXYLENE_BLOCK_RETURN")
  endif()
endif()

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
  PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/windows_include_issues.patch
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

set(FMT_LIB_PREFIX ${PROJECT_SOURCE_DIR}/libs/fmtlib)
ExternalProject_Add(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 3.0.1
  PREFIX ${FMT_LIB_PREFIX}
  CMAKE_ARGS "-DFMT_TEST=OFF"
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)
include_directories(SYSTEM ${FMT_LIB_PREFIX}/src/fmtlib/)
link_directories(${FMT_LIB_PREFIX}/src/fmtlib-build/fmt)

set(VARIANT_PREFIX ${PROJECT_SOURCE_DIR}/libs/variant)
ExternalProject_Add(
  variant
  GIT_REPOSITORY https://github.com/mpark/variant.git
  GIT_TAG v1.0.0
  PREFIX ${VARIANT_PREFIX}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)
include_directories(SYSTEM ${VARIANT_PREFIX}/src/variant/include/)
include_directories(SYSTEM ${VARIANT_PREFIX}/src/variant/include/mpark/)

find_package(LLVM REQUIRED)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
llvm_map_components_to_libnames(
  REQ_LLVM_LIBRARIES
  aarch64 aarch64asmparser aarch64asmprinter aarch64codegen aarch64desc aarch64disassembler aarch64info aarch64utils
  amdgpu amdgpuasmparser amdgpuasmprinter amdgpucodegen amdgpudesc amdgpudisassembler amdgpuinfo amdgpuutils
  arm armasmparser armasmprinter armcodegen armdesc armdisassembler arminfo
  bpf bpfasmprinter bpfcodegen bpfdesc bpfinfo
  hexagon hexagonasmparser hexagoncodegen hexagondesc hexagondisassembler hexagoninfo
  lanai lanaiasmparser lanaicodegen lanaidesc lanaidisassembler lanaiinfo lanaiinstprinter
  mips mipsasmparser mipsasmprinter mipscodegen mipsdesc mipsdisassembler mipsinfo
  msp430 msp430asmprinter msp430codegen msp430desc msp430info
  nvptx nvptxasmprinter nvptxcodegen nvptxdesc nvptxinfo
  powerpc powerpcasmparser powerpcasmprinter powerpccodegen powerpcdesc powerpcdisassembler powerpcinfo
  riscv riscvcodegen riscvdesc riscvinfo
  sparc sparcasmparser sparcasmprinter sparccodegen sparcdesc sparcdisassembler sparcinfo
  systemz systemzasmparser systemzasmprinter systemzcodegen systemzdesc systemzdisassembler systemzinfo
  x86 x86asmparser x86asmprinter x86codegen x86desc x86disassembler x86info x86utils
  xcore xcoreasmprinter xcorecodegen xcoredesc xcoredisassembler xcoreinfo
  
  analysis asmparser asmprinter bitreader bitwriter codegen core coverage coroutines demangle debuginfocodeview debuginfodwarf debuginfopdb
  executionengine globalisel instcombine instrumentation interpreter ipo irreader libdriver lineeditor linker lto
  mc mcdisassembler mcjit mcparser mirparser native nativecodegen objcarcopts object objectyaml option orcjit passes
  profiledata runtimedyld scalaropts selectiondag support symbolize tablegen target transformutils vectorize
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_SOURCES
  ${SRC_DIR}/utils/util.cpp
  ${SRC_DIR}/utils/error.cpp
  ${SRC_DIR}/utils/typeInfo.cpp
  ${SRC_DIR}/operator.cpp
  ${SRC_DIR}/token.cpp
  ${SRC_DIR}/lexer.cpp
  ${SRC_DIR}/ast.cpp
  ${SRC_DIR}/parser/tokenParser.cpp
  ${SRC_DIR}/parser/xmlParser.cpp
  ${SRC_DIR}/llvm/compiler.cpp
  ${SRC_DIR}/llvm/runner.cpp
  ${SRC_DIR}/llvm/values.cpp
  ${SRC_DIR}/llvm/typeId.cpp
  ${SRC_DIR}/llvm/typeData.cpp
  ${SRC_DIR}/runtime/runtime.cpp
  ${SRC_DIR}/runtime/io.cpp
)
set(COMMON_DEPS rapidxml termcolor tclap fmtlib variant)
set(COMMON_LINK_LIBS ${REQ_LLVM_LIBRARIES} stdc++fs fmt)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)

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
  add_custom_command(TARGET ${target_name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/bin/${target_name}_bin ${SYMLINK_LOCATION})
endfunction(make_exe_symlink)
