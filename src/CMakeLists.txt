cmake_minimum_required(VERSION 3.28)

project(av)

add_executable(av main.cpp ast.cpp tokenizer.cpp parser.cpp gen.cpp desugar.cpp)

if (DEFINED VERSION_)
  target_compile_definitions(av PRIVATE VERSION_="${VERSION_}")
  target_link_options(av PRIVATE -static -static-libstdc++ -static-libgcc)
endif()
if (DEFINED OS)
  target_compile_definitions(av PRIVATE OS="${OS}")
endif()
if (DEFINED YEAR)
  target_compile_definitions(av PRIVATE YEAR="${YEAR}")
endif()
if (DEFINED COMPILEDON)
target_compile_definitions(av PRIVATE COMPILEDON="${COMPILEDON}")
endif()