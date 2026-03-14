#pragma once

#include <stdexcept>
#include <string>

namespace av {

class Error : public std::exception {
  std::string msg;
  int s_, e_;

public:
  Error(std::string msg, int s, int e) : msg(msg), s_(s), e_(e) {}
  const char *what() const noexcept override {
    return msg.c_str();
  }
  int s() { return s_; }
  int e() { return e_; }
};

} // namespace av