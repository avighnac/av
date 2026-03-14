#include "ast.hpp"
#include "desugar.hpp"
#include "errors.hpp"
#include "gen.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

bool command_exists(const std::string &cmd) {
  std::string test = "command -v " + cmd + " > /dev/null 2>&1";
  return std::system(test.c_str()) == 0;
}

std::string platform() {
#ifdef __APPLE__
  return "macos";
#elif _WIN32
  return "windows";
#elif __linux__
  return "linux";
#endif
  return "unknown";
}

std::string def() {
  std::string p = platform();
  if (p != "linux" && p != "macos") {
    p = "linux";
  }
  return p;
}

std::string white(const std::string &s) { return "\033[1m" + s + "\033[0m"; }
std::string red(const std::string &s) { return "\033[1;31m" + s + "\033[0m"; }
std::string blue(const std::string &s) { return "\033[1;36m" + s + "\033[0m"; }

void print_help(const std::string &name) {
  std::cout << "Usage: " << name << " <filename.av>\n";
  std::cout << "Options:\n";
  std::cout << "  --help: prints this message\n";
  std::cout << "  --version: prints the version number\n";
  std::cout << "  -tokenize: Tokenizes the file\n";
  std::cout << "  -ast: Prints the AST (without desugaring)\n";
  std::cout << "  --ast: Prints the AST (after desugaring)\n";
  std::cout << "  -s: Prints the asm\n";
  std::cout << "\n";
  std::cout << "Parameters:\n";
  std::cout << "  -target=<platform>: default: " << white(def()) << ", can be either linux or macos\n";
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

std::vector<std::string> lines;
void print_error(std::ostream &os, std::string file, int r1, int c1, int r2, int c2, std::string err) {
  std::string l = std::to_string(r1);
  os << white(file) << white(":") << white(l) << white(":") << white(std::to_string(c1));
  if (r1 != r2) {
    os << white("-") << white(std::to_string(r2)) << white(":") << white(std::to_string(c2));
  }
  os << white(": ") << red("error: ") << err << '\n';

  if (r1 == r2) {
    os << " " << l << " | " << lines[r1 - 1] << '\n';
    c1 += 3 + l.length(), c2 += 3 + l.length();
    for (int i = 0; i < lines[r1 - 1].length() + 3 + l.length(); ++i) {
      if (i == c1) {
        os << red("^");
      } else if (i >= c1 && i <= c2) {
        os << red("~");
      } else {
        os << ' ';
      }
    }
    return;
  }

  int len = std::to_string(r2 - 1).length();
  auto fix = [&](const std::string &s) { return std::string(len - s.length(), '0') + s; };
  for (int r = r1 - 1; r <= r2 - 1; ++r) {
    os << " " << fix(std::to_string(r)) << " | " << (r == r1 - 1 ? red(lines[r]) : blue(lines[r]));
    if (r != r2 - 1) {
      os << '\n';
    }
  }
}

std::string nasm_platform(const std::string &platform) {
  if (platform == "linux") {
    return "elf64";
  } else if (platform == "macos") {
    return "macho64";
  }
  assert(false);
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

  std::vector<bool> used(argc);
  std::array<std::string, 6> options{"-tokenize", "-ast", "-s", "--help", "--version", "--ast"};
  std::array values{false, false, false, false, false, false};
  for (int i = 1; i < argc; ++i) {
    int it = std::find(options.begin(), options.end(), argv[i]) - options.begin();
    if (it != options.size()) {
      used[i] = true;
      if (values[it]) {
        std::cerr << "warning: " << options[it] << " repeated\n";
      }
      values[it] = true;
    }
  }

  std::array<std::string, 1> params{"-target="};
  std::array<std::string, 1> choices = {};
  for (int i = 1; i < argc; ++i) {
    int it = std::find_if(params.begin(), params.end(), [&](const std::string &s) { return argv[i].find(s) == 0; }) - params.begin();
    if (it == params.size()) {
      continue;
    }
    used[i] = true;
    if (!choices[it].empty()) {
      std::cerr << red("error: ") << "parameter " << params[it].substr(0, params[it].length() - 1) << " specified more than 1 time\n";
      return 1;
    }
    choices[it] = argv[i].substr(params[it].length());
  }
  if (choices[0].empty()) {
    choices[0] = def();
  }

  std::vector<std::string> files;
  for (int i = 1; i < argc; ++i) {
    if (!used[i]) {
      files.push_back(argv[i]);
    }
  }

  if (values[3]) {
    print_help(argv[0]);
    return 0;
  }
  if (values[4]) {
    print_version();
    return 0;
  }

  if (choices[0] != "linux" && choices[0] != "macos") {
    std::cerr << red("error: ") << "value " << choices[0] << " for parameter " << params[0] << " is invalid\n";
    return 1;
  }

  if (files.size() == 0) {
    std::cerr << red("error: ") << "no input file specified\n";
    return 1;
  }
  if (files.size() != 1) {
    std::cerr << "warning: ignoring " << files.size() - 1 << " additional files\n";
    files = {files[0]};
  }

  std::ifstream f(files[0], std::ios::binary);
  if (!f) {
    std::cerr << red("error: ") << "file " << files[0] << " does not exist.\n";
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

  std::vector<int> line_ends = {-1};
  std::string line;
  for (int i = 0; i < size; ++i) {
    if (code[i] == '\n') {
      line_ends.push_back(i);
      lines.push_back(line);
      line.clear();
    } else {
      line.push_back(code[i]);
    }
  }
  lines.push_back(line);
  auto get_rc = [&](int loc) -> std::pair<int, int> {
    // 'a' 'b' 'c' '\n' 'd'
    //  0.  1.  2.   3.  4
    auto it = std::lower_bound(line_ends.begin(), line_ends.end(), loc);
    int r = it - line_ends.begin();
    int c = loc - line_ends[r - 1];
    return {r, c};
  };

  if (values[0]) {
    try {
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
    } catch (av::Error e) {
      auto [r1, c1] = get_rc(e.s());
      auto [r2, c2] = get_rc(e.e());
      print_error(std::cerr, files[0], r1, c1, r2, c2, e.what());
      std::cerr << "\n\ncompilation failed.\n";
      return 1;
    }
  }

  if (values[1]) {
    try {
      av::tokenizer tk(code);
      av::Node *root = av::parse(tk);
      std::cout << *root;
      delete root;
    } catch (av::Error e) {
      auto [r1, c1] = get_rc(e.s());
      auto [r2, c2] = get_rc(e.e());
      print_error(std::cerr, files[0], r1, c1, r2, c2, e.what());
      std::cerr << "\n\ncompilation failed.\n";
      return 1;
    }
  }
  if (values[5]) {
    try {
      av::tokenizer tk(code);
      av::Node *root = av::parse(tk);
      av::desugar(root);
      std::cout << *root;
      delete root;
    } catch (av::Error e) {
      auto [r1, c1] = get_rc(e.s());
      auto [r2, c2] = get_rc(e.e());
      print_error(std::cerr, files[0], r1, c1, r2, c2, e.what());
      std::cerr << "\n\ncompilation failed.\n";
      return 1;
    }
  }

  if (values[2]) {
    try {
      av::tokenizer tk(code);
      av::Node *root = av::parse(tk);
      av::desugar(root);
      av::generate(root, av::platform_from_string(choices[0]), std::cout);
      delete root;
      return 0;
    } catch (av::Error e) {
      auto [r1, c1] = get_rc(e.s());
      auto [r2, c2] = get_rc(e.e());
      print_error(std::cerr, files[0], r1, c1, r2, c2, e.what());
      std::cerr << "\n\ncompilation failed.\n";
      return 1;
    }
  }

  if (values[0] || values[1] || values[2] || values[5]) {
    return 0;
  }

  try {
    av::tokenizer tk(code);
    av::Node *root = av::parse(tk);
    if (!av::desugar(root)) {
      std::cerr << red("error: ") << "int32 main() not found; did you mean to compile with -s?\n";
      return 1;
    }
    std::stringstream os;
    os << "section .text\n";
    av::generate(root, av::platform_from_string(choices[0]), os);
    os << "global _start\n";
    os << "_start:\n";
    os << "  call main\n";
    os << "  mov edi, eax\n";
    if (choices[0] == "linux") {
      os << "  mov eax, 60\n";
    } else {
      os << "  mov eax, 33554433\n";
    }
    os << "  syscall\n";
    std::ofstream oss(argv[1] + ".asm");
    oss << os.str();
    oss.close();
    delete root;
  } catch (av::Error e) {
    auto [r1, c1] = get_rc(e.s());
    auto [r2, c2] = get_rc(e.e());
    print_error(std::cerr, files[0], r1, c1, r2, c2, e.what());
    std::cerr << "\n\ncompilation failed.\n";
    return 1;
  }

  if (!command_exists("nasm")) {
    std::cerr << "warning: nasm is not installed, stopping at the assembly emission step\n";
  } else {
    std::system(("nasm -f " + nasm_platform(choices[0]) + " " + argv[1] + ".asm -o " + argv[1] + ".o").c_str());
    if (!command_exists("ld")) {
      std::cerr << "warning: ld is not installed, stopping at the object linking step\n";
    } else {
      if (choices[0] == "linux") {
        std::system(("ld " + argv[1] + ".o -o a.out").c_str());
      } else if (choices[0] == "macos") {
        std::system(("ld -platform_version macos 13.0 13.0 -e _start " + argv[1] + ".o -o a.out 2>/dev/null").c_str());
      } else {
        assert(false);
      }
    }
  }
}