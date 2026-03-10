#pragma once

#include <string>
#include <vector>

namespace av {

enum TokenType {
  Tk_Type,
  Tk_Identifier,
  Tk_Number,
  Tk_Equal,
  Tk_Semicolon,
  Tk_OpenParen,
  Tk_CloseParen,
  Tk_OpenBrace,
  Tk_CloseBrace,
  Tk_Amp,
  Tk_Bar,
  Tk_Comma,
  Tk_Star,
  Tk_Underscore,
  Tk_LogicalAnd,
  Tk_LogicalOr,
  Tk_Return,
  __count_TokenType
};

std::string to_string(TokenType token);
std::ostream &operator<<(std::ostream &os, const TokenType &token);

struct Token {
  std::string token;
  TokenType type;
};

bool is_keyword(std::string s);

class tokenizer {
private:
  std::string buf;
  std::size_t ptr_;
  char advance();
  void skip_space();
  char peek_(int adv = 0);

  std::string get_word();
  std::string get_number();

  Token get_();

  std::vector<Token> tokens;
  std::size_t ptr;
public:
  tokenizer() : ptr_(0), ptr(0) {}
  tokenizer(std::string buf) : buf(buf), ptr_(0), ptr(0) {
    while (peek_()) {
      tokens.push_back(get_());
    }
  }

  bool peek();
  Token get();
  // template <typename F>
  // bool has(const F &&f);
  void push(const Token &t);
  void pop();
  std::size_t size() const;
  Token &operator[](std::size_t i);
  void clear();
};

} // namespace av