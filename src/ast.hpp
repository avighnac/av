#pragma once

#include <array>
#include <vector>
#include <string>
#include <ostream>
#include <concepts>
#include <cstdint>
#include "tokenizer.hpp"

namespace av {

enum Type {
  Int8,
  Int16,
  Int32,
  Int64,
  Int8Ptr,
  Int16Ptr,
  Int32Ptr,
  Int64Ptr,
  VoidPtr,
  Void,
  __count_Type
};

Type type_from_string(const std::string &s);

enum NodeType {
  block,
  functionDecl,
  functionBody,
  variableDecl,
  assign,
  int8Literal,
  int16Literal,
  int32Literal,
  int64Literal,
  addressOf,
  dereference,
  functionCall,
  identifier,
  returnNode,
  __count_NodeType
};

std::string to_string(NodeType type);
std::string to_string(Type type);
template <typename T>
  requires std::same_as<T, NodeType> || std::same_as<T, Type> 
std::ostream &operator<<(std::ostream &os, const T &type);

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v);

struct Node {
  NodeType type;
  Node(NodeType type) : type(type) {}
  virtual ~Node() = default;
  std::ostream &print(std::ostream &os, int dep = 0) const;
};

std::ostream &operator<<(std::ostream &os, const Node &t);

struct Block : Node {
  std::vector<Node *> stmts;
  Block() : Node(block) {}
  ~Block() {
    for (Node *&i : stmts) {
      delete i;
    }
  }
};

struct FunctionDecl : Node {
  std::string Name;
  Type ReturnType;
  std::vector<Type> ParamTypes;
  FunctionDecl() : Node(functionDecl) {}
};

struct FunctionBody : Node {
  std::string Name;
  std::vector<std::string> Params;
  Node *Body;
  FunctionBody() : Node(functionBody) {}
  ~FunctionBody() {
    delete Body;
  }
};

struct VariableDecl : Node {
  std::string Name;
  Type VariableType;
  VariableDecl() : Node(variableDecl) {}
};

struct Assign : Node {
  Node *To;
  Node *Value;
  Assign() : Node(assign), To(nullptr), Value(nullptr) {}
  ~Assign() {
    delete To;
    delete Value;
  }
};

struct AddressOf : Node {
  std::string Name;
  AddressOf() : Node(addressOf) {}
};

struct Dereference : Node {
  std::string Name;
  Dereference() : Node(dereference) {}
};

struct FunctionCall : Node {
  std::string Name;
  std::vector<Node *> Params;
  FunctionCall() : Node(functionCall) {}
  ~FunctionCall() {
    for (Node *&i : Params) {
      delete i;
    }
  }
};

struct Identifier : Node {
  std::string Name;
  Identifier() : Node(identifier) {}
};

struct Int8Literal : Node {
  int8_t Value;
  Int8Literal() : Node(int8Literal) {}
};

struct Int16Literal : Node {
  int16_t Value;
  Int16Literal() : Node(int16Literal) {}
};

struct Int32Literal : Node {
  int32_t Value;
  Int32Literal() : Node(int32Literal) {}
};

struct Int64Literal : Node {
  int64_t Value;
  Int64Literal() : Node(int64Literal) {}
};

struct Return : Node {
  Node *Value;
  Return() : Node(returnNode) {}
  ~Return() {
    delete Value;
  }
};

} // namespace av