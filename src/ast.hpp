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
  equal,
  less,
  greater,
  lessEqual,
  greaterEqual,
  shiftLeft,
  shiftRight,
  plus,
  minus,
  unaryMinus,
  multiply,
  div,
  modulo,
  bitwiseAnd,
  bitwiseOr,
  logicalNegate,
  ifNode,
  elseIf,
  elseNode,
  whileNode,
  ifElseBlock,
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
};

std::ostream &print(const Node *u, std::ostream &os, int dep = 0);
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
  Type ReturnType;
  std::vector<Type> ParamTypes;
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

struct Binary : Node {
  Node *Lhs;
  Node *Rhs;
  Binary(const NodeType &type) : Node(type) {}
  ~Binary() {
    delete Lhs;
    delete Rhs;
  }
};

struct Equal : Binary {
  Equal() : Binary(equal) {}
};

struct Less : Binary {
  Less() : Binary(less) {}
};

struct Greater : Binary {
  Greater() : Binary(greater) {}
};

struct LessEqual : Binary {
  LessEqual() : Binary(lessEqual) {}
};

struct GreaterEqual : Binary {
  GreaterEqual() : Binary(greaterEqual) {}
};

struct ShiftLeft : Binary {
  ShiftLeft() : Binary(shiftLeft) {}
};

struct ShiftRight : Binary {
  ShiftRight() : Binary(shiftRight) {}
};

struct Plus : Binary {
  Plus() : Binary(plus) {}
};

struct UnaryMinus : Node {
  Node *To;
  UnaryMinus() : To(nullptr), Node(unaryMinus) {}
  ~UnaryMinus() {
    delete To;
  }
};

struct LogicalNegate : Node {
  Node *To;
  LogicalNegate() : To(nullptr), Node(logicalNegate) {}
  ~LogicalNegate() {
    delete To;
  }
};

struct Minus : Binary {
  Minus() : Binary(minus) {}
};

struct Multiply : Binary {
  Multiply() : Binary(multiply) {}
};

struct Div : Binary {
  Div() : Binary(div) {}
};

struct Modulo : Binary {
  Modulo() : Binary(modulo) {}
};

struct BitwiseAnd : Binary {
  BitwiseAnd() : Binary(bitwiseAnd) {}
};

struct BitwiseOr : Binary {
  BitwiseOr() : Binary(bitwiseOr) {}
};

struct If : Node {
  Node *Cond;
  Node *Body;
  If() : Cond(nullptr), Body(nullptr), Node(ifNode) {}
  ~If() {
    delete Cond;
    delete Body;
  }
};

struct ElseIf : Node {
  Node *Cond;
  Node *Body;
  ElseIf() : Cond(nullptr), Body(nullptr), Node(elseIf) {}
  ~ElseIf() {
    delete Cond;
    delete Body;
  }
};

struct Else : Node {
  Node *Body;
  Else() : Body(nullptr), Node(elseNode) {}
  ~Else() {
    delete Body;
  }
};

struct IfElseBlock : Node {
  std::vector<Node *> Conds;
  IfElseBlock() : Node(ifElseBlock) {}
};

struct While : Node {
  Node *Cond;
  Node *Body;
  While() : Cond(nullptr), Body(nullptr), Node(whileNode) {}
  ~While() {
    delete Cond;
    delete Body;
  }
};

} // namespace av