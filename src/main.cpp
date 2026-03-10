#include "ast.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

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
}