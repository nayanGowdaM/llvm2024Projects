cmake_minimum_required(VERSION 3.8)
project(NewCyclomaticComplexityPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the Clang installation path
set(CMAKE_PREFIX_PATH "/usr/local/opt/llvm")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# List of Clang libraries to link against
list(APPEND CLANG_LIBS
  clangAST
  clangASTMatchers
  clangAnalysis
  clangBasic
  clangDriver
  clangEdit
  clangFrontend
  clangFrontendTool
  clangLex
  clangParse
  clangSema
  clangSerialization
  clangTooling
  clangToolingCore
  clangRewrite
  clangRewriteFrontend
)

# List of LLVM libraries to link against
list(APPEND LLVM_LIBS
  LLVMCore
  LLVMSupport
  LLVMIRReader
  LLVMBitReader
  LLVMBitWriter
  LLVMOption
  LLVMFrontendOpenMP
  LLVMMC
  LLVMMCParser
  LLVMProfileData
  LLVMSupport
  LLVMTarget
)

# Create the shared library instead of an executable
add_library(NewCyclomaticComplexityPlugin MODULE NewCyclomaticComplexity.cpp)

target_link_libraries(NewCyclomaticComplexityPlugin
  PRIVATE
  ${CLANG_LIBS}
  ${LLVM_LIBS}
)
