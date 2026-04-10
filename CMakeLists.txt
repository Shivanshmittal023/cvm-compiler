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
)
