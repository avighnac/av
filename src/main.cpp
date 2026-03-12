#include "ast.hpp"
#include "gen.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include "desugar.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

bool command_exists(const std::string& cmd) {
  std::string test = "command -v " + cmd + " > /dev/null 2>&1";
  return std::system(test.c_str()) == 0;
}

int main(int argc, char **_argv) {
  std::vector<std::string> argv(argc);
  for (int i = 0; i < argc; ++i) {
    argv[i] = _argv[i];
  }

  if (argc == 1) {
    std::cout << "Usage: " << argv[0] << " <filename.av>\n";
    std::cout << "Options:\n";
    std::cout << "  --tokenize: Tokenizes the file\n";
    std::cout << "  --ast: Prints the AST\n";
    std::cout << "  --s: Prints the asm\n";
    return 0;
  }

  std::ifstream f(argv[1], std::ios::binary);
  if (!f) {
    std::cout << "Error: file " << argv[1] << " does not exist.\n";
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

  if (std::find(argv.begin(), argv.end(), "--tokenize") != argv.end()) {
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
    return 0;
  }

  if (std::find(argv.begin(), argv.end(), "--ast") != argv.end()) {
    av::tokenizer tk(code);
    av::Node *root = av::parse(tk);
    std::cout << *root;
    delete root;
    return 0;
  }

  if (std::find(argv.begin(), argv.end(), "--s") != argv.end()) {
    av::tokenizer tk(code);
    av::Node *root = av::parse(tk);
    av::desugar(root);
    av::generate(root, std::cout);
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

  if (!command_exists("nasm")) {
    std::cout << "[warning] nasm is not installed, stopping at the assembly emission step\n";
  } else {
    std::system(("nasm -f elf64 " + argv[1] + ".asm -o " + argv[1] + ".o").c_str());
    if (!command_exists("ld")) {
      std::cout << "[warning] ld is not installed, stopping at the object linking step\n";
    } else {
      std::system(("ld " + argv[1] + ".o -o a.out").c_str());
    }
  }
}