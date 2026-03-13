#include "tokenizer.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace av {

// clang-format off
bool is_keyword(std::string s) {
  static std::array<std::string, __count_TokenType> a({
    "int8", "int16", "int32", "int64", "void", "int8*",
    "int16*", "int32*", "int64*", "void*", "return", "if",
    "else", "while", "for"
  });
  return find(a.begin(), a.end(), s) != a.end();
}

std::string to_string(TokenType token) {
  static const std::array<std::string, int(av::__count_TokenType)> a({
    "Datatype", "Identifier", "Number", "Assign", "Semicolon",
    "OpenParen", "CloseParen", "OpenBrace", "CloseBrace",
    "Amp", "Bar", "Comma", "Star", "Underscore", "LogicalAnd", "LogicalOr", "LogicalNegate",
    "Return", "Equal", "Less", "Greater", "LessEqual", "GreaterEqual",
    "ShiftLeft", "ShiftRight", "Plus", "Minus", "Div", "Percent", "PlusPlus",
    "MinusMinus", "If", "Else", "While", "For"
  });
  return a[int(token)];
}
// clang-format on
std::ostream &operator<<(std::ostream &os, const TokenType &token) { return os << to_string(token); }

char tokenizer::peek_(int adv) { return ptr_ + adv >= buf.length() ? 0 : buf[ptr_ + adv]; }
char tokenizer::advance() { return buf[ptr_++]; }
void tokenizer::skip_space() {
  while (std::isspace(peek_())) {
    advance();
  }
}

std::string tokenizer::get_word() {
  std::string ret;
  while (std::isalnum(peek_()) || peek_() == '_') {
    ret.push_back(advance());
  }
  if (peek_() == '*') {
    ret.push_back(advance());
  }
  return ret;
}

std::string tokenizer::get_number() {
  std::string ret;
  ret.push_back(advance());
  // if (ret.back() != '-' && !std::isdigit(ret.back())) {
  //   throw std::runtime_error("get_number() called but first character was " + std::string(1, ret.back()));
  // }
  while (std::isdigit(peek_())) {
    ret.push_back(advance());
  }
  return ret;
}

Token tokenizer::get_() {
  skip_space();
  Token token;
  char c = peek_();

  if (std::isalpha(c)) {
    if (!is_keyword(token.token = get_word())) {
      token.type = Tk_Identifier;
      return token;
    }
    if (token.token == "return") {
      token.token.clear();
      token.type = Tk_Return;
      return token;
    }
    if (token.token == "if") {
      token.token.clear();
      token.type = Tk_If;
      return token;
    }
    if (token.token == "else") {
      token.token.clear();
      token.type = Tk_Else;
      return token;
    }
    if (token.token == "while") {
      token.token.clear();
      token.type = Tk_While;
      return token;
    }
    if (token.token == "for") {
      token.token.clear();
      token.type = Tk_For;
      return token;
    }
    token.type = Tk_Type; // right now the only keywords are types
    return token;
  }

  if (std::isdigit(c)) {
    token.type = Tk_Number;
    token.token = get_number();
    return token;
  }

  if (c == ';') {
    advance();
    token.type = Tk_Semicolon;
    return token;
  }
  if (c == '(') {
    advance();
    token.type = Tk_OpenParen;
    return token;
  }
  if (c == ')') {
    advance();
    token.type = Tk_CloseParen;
    return token;
  }
  if (c == '{') {
    advance();
    token.type = Tk_OpenBrace;
    return token;
  }
  if (c == '}') {
    advance();
    token.type = Tk_CloseBrace;
    return token;
  }
  if (c == '_') {
    advance();
    token.type = Tk_Underscore;
    return token;
  }
  if (c == ',') {
    advance();
    token.type = Tk_Comma;
    return token;
  }
  if (c == '*') {
    advance();
    token.type = Tk_Star;
    return token;
  }
  if (c == '/') {
    advance();
    token.type = Tk_Div;
    return token;
  }
  if (c == '%') {
    advance();
    token.type = Tk_Percent;
    return token;
  }
    if (c == '~') {
    advance();
    token.type = Tk_LogicalNegate;
    return token;
  }

  if (c == '&') {
    advance();
    if (peek_() == '&') {
      advance();
      token.type = Tk_LogicalAnd;
    } else {
      token.type = Tk_Amp;
    }
    return token;
  }
  if (c == '|') {
    advance();
    if (peek_() == '|') {
      advance();
      token.type = Tk_LogicalOr;
    } else {
      token.type = Tk_Bar;
    }
    return token;
  }
  if (c == '+') {
    advance();
    if (peek_() == '+') {
      advance();
      token.type = Tk_PlusPlus;
    } else {
      token.type = Tk_Plus;
    }
    return token;
  }
  if (c == '-') {
    advance();
    if (peek_() == '-') {
      advance();
      token.type = Tk_MinusMinus;
    } else {
      token.type = Tk_Minus;
    }
    return token;
  }
  if (c == '=') {
    advance();
    if (peek_() == '=') {
      advance();
      token.type = Tk_Equal;
    } else {
      token.type = Tk_Assign;
    }
    return token;
  }
  if (c == '<') {
    advance();
    if (peek_() == '=') {
      advance();
      token.type = Tk_LessEqual;
    } else if (peek_() == '<') {
      advance();
      token.type = Tk_ShiftLeft;
    } else {
      token.type = Tk_Less;
    }
    return token;
  }
  if (c == '>') {
    advance();
    if (peek_() == '=') {
      advance();
      token.type = Tk_GreaterEqual;
    } else if (peek_() == '<') {
      advance();
      token.type = Tk_ShiftRight;
    } else {
      token.type = Tk_Greater;
    }
    return token;
  }

  throw std::runtime_error("Syntax error");
}

bool tokenizer::peek() { return !tokens.empty(); }
Token tokenizer::get() {
  Token token = tokens[0];
  tokens.pop_front();
  return token;
}

void tokenizer::push_back(const Token &t) {
  tokens.push_back(t);
}

void tokenizer::push_front(const Token &t) {
  tokens.push_front(t);
}

void tokenizer::pop_back() {
  tokens.pop_back();
}

std::size_t tokenizer::size() const {
  return tokens.size();
}

Token &tokenizer::operator[](std::size_t i) {
  return tokens[i];
}

void tokenizer::clear() {
  *this = tokenizer();
}

} // namespace av
