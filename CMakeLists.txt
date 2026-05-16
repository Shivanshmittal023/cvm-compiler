cmake_minimum_required(VERSION 3.10)
project(MiniCppCompiler)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where to look for header files
include_directories(include)

# Create an executable called 'mycc' using our C++ files
add_executable(mycc 
    src/main.cpp
    src/Token.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/Parser.cpp
    src/Opcode.cpp
    src/Chunk.cpp
    src/Compiler.cpp
    src/VM.cpp
)

# Test executable for bytecode infrastructure
add_executable(test_bytecode
    tests/test_bytecode.cpp
    src/Opcode.cpp
    src/Chunk.cpp
)

# Test executable for compiler
add_executable(test_compiler
    tests/test_compiler.cpp
    src/Token.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/Parser.cpp
    src/Opcode.cpp
    src/Chunk.cpp
    src/Compiler.cpp
)
