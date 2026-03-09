#pragma once

#include <array>
#include <vector>
#include <string>
#include <ostream>
#include <concepts>

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
  __count_Type
};

enum NodeType {
  root,
  functionDecl,
  functionBody,
  variableDecl,
  assign,
  int8Literal,
  int16Literal,
  int32Literal,
  int64Literal,
  addressOf,
  functionCall,
  value,
  identifier,
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
  void parse(std::string code);
};

std::ostream &operator<<(std::ostream &os, const Node &t);

struct Root : Node {
  std::vector<Node *> stmts;
  Root() : Node(root) {}
  ~Root() {
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
  std::vector<Node *> stmts;
  FunctionBody() : Node(functionBody) {}
  ~FunctionBody() {
    for (Node *&i : stmts) {
      delete i;
    }
  }
};

struct VariableDecl : Node {
  std::string Name;
  Type VariableType;
  VariableDecl() : Node(variableDecl) {}
};

struct Assign : Node {
  std::string To;
  Node *Value;
  Assign() : Node(assign), Value(nullptr) {}
  ~Assign() {
    delete Value;
  }
};

struct AddressOf : Node {
  std::string Name;
  AddressOf() : Node(addressOf) {}
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

} // namespace av