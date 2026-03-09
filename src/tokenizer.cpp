#include "tokenizer.hpp"
#include <string>
#include <array>
#include <algorithm>
#include <utility>
#include <stdexcept>

namespace av {

bool is_keyword(std::string s) {
  static std::array<std::string, __count_TokenType> a({
    "int8", "int16", "int32", "int64", "int8*",
    "int16*", "int32*", "int64*", "return"
  });
  return find(a.begin(), a.end(), s) != a.end();
}

std::string to_string(TokenType token) {
  static const std::array<std::string, int(av::__count_TokenType)> a({
    "Datatype", "Identifier", "Number", "Equal", "Semicolon",
    "OpenParen", "CloseParen", "OpenBrace", "CloseBrace",
    "Amp", "Bar", "Comma", "Underscore", "LogicalAnd", "LogicalOr", "Return"
  });
  return a[int(token)];
}
std::ostream &operator<<(std::ostream &os, const TokenType &token) { return os << to_string(token); }

char tokenizer::peek() { return ptr == buf.length() ? 0 : buf[ptr]; }
char tokenizer::advance() { return buf[ptr++]; }
void tokenizer::skip_space() { while (std::isspace(peek())) { advance(); } }

std::string tokenizer::get_word() {
  std::string ret;
  while (std::isalnum(peek()) || peek() == '_') {
    ret.push_back(advance());
  }
  if (peek() == '*') {
    ret.push_back(advance());
  }
  return ret;
}

std::string tokenizer::get_number() {
  std::string ret;
  while (std::isdigit(peek())) {
    ret.push_back(advance());
  }
  return ret;
}

Token tokenizer::get() {
  skip_space();
  Token token;
  char c = peek();

  if (std::isalpha(c)) {
    if (!is_keyword(token.token = get_word())) {
      token.type = Identifier;
      return token;
    }
    if (token.token == "return") {
      token.token.clear();
      token.type = Return;
      return token;
    }
    token.type = Datatype; // right now the only keywords are types
    return token;
  }

  if (std::isdigit(c)) {
    token.type = Number;
    token.token = get_number();
    return token;
  }

  if (c == '=') {
    advance();
    token.type = Equal;
    return token;
  }
  if (c == ';') {
    advance();
    token.type = Semicolon;
    return token;
  }
  if (c == '(') {
    advance();
    token.type = OpenParen;
    return token;
  }
  if (c == ')') {
    advance();
    token.type = CloseParen;
    return token;
  }
  if (c == '{') {
    advance();
    token.type = OpenBrace;
    return token;
  }
  if (c == '}') {
    advance();
    token.type = CloseBrace;
    return token;
  }
  if (c == '_') {
    advance();
    token.type = Underscore;
    return token;
  }
  if (c == ',') {
    advance();
    token.type = Comma;
    return token;
  }

  if (c == '&') {
    advance();
    if (peek() == '&') {
      advance();
      token.type = LogicalAnd;
    }
    token.type = Amp;
    return token;
  }
  if (c == '|') {
    advance();
    if (peek() == '|') {
      advance();
      token.type = LogicalOr;
    }
    token.type = Bar;
    return token;
  }

  throw std::runtime_error("Syntax error");
}

}
