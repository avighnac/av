#pragma once

#include <deque>
#include <string>

namespace av {

enum TokenType {
  Tk_Type,
  Tk_Identifier,
  Tk_Number,
  Tk_Assign,
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
  Tk_Equal,
  Tk_Less,
  Tk_Greater,
  Tk_LessEqual,
  Tk_GreaterEqual,
  Tk_ShiftLeft,
  Tk_ShiftRight,
  Tk_Plus,
  Tk_Minus,
  Tk_Div,
  Tk_Percent,
  Tk_PlusPlus,
  Tk_MinusMinus,
  Tk_If,
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

  std::deque<Token> tokens;

public:
  tokenizer() : ptr_(0) {}
  tokenizer(std::string buf_) : buf(buf_), ptr_(0) {
    while (!buf.empty() && std::isspace(buf.back())) {
      buf.pop_back();
    }
    while (peek_()) {
      tokens.push_back(get_());
    }
  }

  bool peek();
  Token get();
  template <typename F>
  int has(const F &&f) {
    int ans = -1;
    for (int i = 0, bal = 0; i < int(tokens.size()) && ans == -1; ++i) {
      bal += (tokens[i].type == Tk_OpenBrace) - (tokens[i].type == Tk_CloseBrace);
      if (bal == 0 && f(tokens[i])) {
        ans = i;
      }
    }
    return ans;
  }
  void push_back(const Token &t);
  void push_front(const Token &t);
  void pop_back();
  std::size_t size() const;
  Token &operator[](std::size_t i);
  void clear();
};

} // namespace av