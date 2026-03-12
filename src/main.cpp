#include "ast.hpp"
#include "desugar.hpp"
#include "gen.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

bool command_exists(const std::string &cmd) {
  std::string test = "command -v " + cmd + " > /dev/null 2>&1";
  return std::system(test.c_str()) == 0;
}

void print_help(const std::string &name) {
  std::cout << "Usage: " << name << " <filename.av>\n";
  std::cout << "Options:\n";
  std::cout << "  --help: prints this message\n";
  std::cout << "  --version: prints the version number\n";
  std::cout << "  -tokenize: Tokenizes the file\n";
  std::cout << "  -ast: Prints the AST\n";
  std::cout << "  -s: Prints the asm\n";
}

void print_version() {
#ifndef VERSION_
  std::cout << "av (developer build)\n";
#else
  // expect OS to be defined too
  std::cout << "av (" << OS << ") " << VERSION_ << " (compiled on " << COMPILEDON << ")\n";
  std::cout << "Copyright (C) " << YEAR << " avighnac under the MIT License\n";
  std::cout << "This is free software; there is absolutely NO warranty\n";
#endif
}

int main(int argc, char **_argv) {
  std::vector<std::string> argv(argc);
  for (int i = 0; i < argc; ++i) {
    argv[i] = _argv[i];
  }

  if (argc == 1) {
    print_help(argv[0]);
    return 0;
  }

  std::vector<std::string> files;
  std::array<std::string, 5> options{"-tokenize", "-ast", "-s", "--help", "--version"};
  std::array values{false, false, false, false, false};
  for (int i = 1; i < argc; ++i) {
    int it = std::find(options.begin(), options.end(), argv[i]) - options.begin();
    if (it != options.size()) {
      if (values[it]) {
        std::cerr << "warning: " << options[it] << " repeated\n";
      }
      values[it] = true;
    } else {
      files.push_back(argv[i]);
    }
  }

  if (values[3]) {
    print_help(argv[1]);
    return 0;
  }
  if (values[4]) {
    print_version();
    return 0;
  }

  if (files.size() == 0) {
    std::cerr << "error: no input file specified\n";
    return 1;
  }
  if (files.size() != 1) {
    std::cerr << "warning: ignoring " << files.size() - 1 << " additional files\n";
    files = {files[0]};
  }

  std::ifstream f(files[0], std::ios::binary);
  if (!f) {
    std::cerr << "error: file " << files[0] << " does not exist.\n";
    return 1;
  }
  f.seekg(0, std::ios::end);
  std::streamsize size = f.tellg();
  f.seekg(0, std::ios::beg);
  std::string code(size > 0 ? std::size_t(size) : 0, '\0');
  if (size > 0) {
    f.read(code.data(), size);
  }
  f.close();

  if (values[0]) {
    av::tokenizer tk(code);
    while (tk.peek()) {
      av::Token token = tk.get();
      std::cout << token.token;
      if (token.token.empty()) {
        std::cout << token.type;
      } else {
        std::cout << "(" << token.type << ")";
      }
      std::cout << ' ';
    }
    std::cout << '\n';
  }

  if (values[1]) {
    av::tokenizer tk(code);
    av::Node *root = av::parse(tk);
    std::cout << *root;
    delete root;
  }

  if (values[2]) {
    av::tokenizer tk(code);
    av::Node *root = av::parse(tk);
    av::desugar(root);
    av::generate(root, std::cout);
    delete root;
    return 0;
  }

  av::tokenizer tk(code);
  av::Node *root = av::parse(tk);
  if (!av::desugar(root)) {
    throw std::runtime_error("main() not found. Did you mean to compile with --s?");
  }
  std::stringstream os;
  os << "section .text\n";
  av::generate(root, os);
  os << "global _start\n";
  os << "_start:\n";
  os << "  call main\n";
  os << "  mov edi, eax\n";
  os << "  mov eax, 60\n";
  os << "  syscall\n";
  std::ofstream oss(argv[1] + ".asm");
  oss << os.str();
  oss.close();
  delete root;

  if (!command_exists("nasm")) {
    std::cerr << "warning: nasm is not installed, stopping at the assembly emission step\n";
  } else {
    std::system(("nasm -f elf64 " + argv[1] + ".asm -o " + argv[1] + ".o").c_str());
    if (!command_exists("ld")) {
      std::cerr << "warning: ld is not installed, stopping at the object linking step\n";
    } else {
      std::system(("ld " + argv[1] + ".o -o a.out").c_str());
    }
  }
}