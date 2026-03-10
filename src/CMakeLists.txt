cmake_minimum_required(VERSION 3.28)

project(test)

add_executable(test main.cpp ast.cpp tokenizer.cpp parser.cpp gen.cpp)