#pragma once

#include <string>

namespace av {

enum TokenType {
  Datatype,
  Identifier,
  Number,
  Equal,
  Semicolon,
  OpenParen,
  CloseParen,
  OpenBrace,
  CloseBrace,
  Amp,
  Bar,
  Comma,
  Underscore,
  LogicalAnd,
  LogicalOr,
  Return,
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
  std::size_t ptr;
  char advance();
  void skip_space();

  std::string get_word();
  std::string get_number();

public:
  tokenizer(std::string buf) : buf(buf), ptr(0) {}

  char peek();
  Token get();
};

} // namespace av