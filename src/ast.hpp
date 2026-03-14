#pragma once

#include "tokenizer.hpp"
#include <array>
#include <concepts>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

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
  int s, e;
  Node(NodeType type, int s, int e) : type(type), s(s), e(e) {}
  virtual ~Node() = default;
};

std::ostream &print(const Node *u, std::ostream &os, int dep = 0);
std::ostream &operator<<(std::ostream &os, const Node &t);

struct Block : Node {
  std::vector<Node *> stmts;
  Block(int s, int e) : Node(block, s, e) {}
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
  FunctionDecl(int s, int e) : Node(functionDecl, s, e) {}
};

struct FunctionBody : Node {
  std::string Name;
  Type ReturnType;
  std::vector<Type> ParamTypes;
  std::vector<std::string> Params;
  Node *Body;
  FunctionBody(int s, int e) : Node(functionBody, s, e) {}
  ~FunctionBody() {
    delete Body;
  }
};

struct VariableDecl : Node {
  std::string Name;
  Type VariableType;
  VariableDecl(int s, int e) : Node(variableDecl, s, e) {}
};

struct Assign : Node {
  Node *To;
  Node *Value;
  Assign(int s, int e) : Node(assign, s, e), To(nullptr), Value(nullptr) {}
  ~Assign() {
    delete To;
    delete Value;
  }
};

struct AddressOf : Node {
  std::string Name;
  AddressOf(int s, int e) : Node(addressOf, s, e) {}
};

struct Dereference : Node {
  std::string Name;
  Dereference(int s, int e) : Node(dereference, s, e) {}
};

struct FunctionCall : Node {
  std::string Name;
  std::vector<Node *> Params;
  FunctionCall(int s, int e) : Node(functionCall, s, e) {}
  ~FunctionCall() {
    for (Node *&i : Params) {
      delete i;
    }
  }
};

struct Identifier : Node {
  std::string Name;
  Identifier(int s, int e) : Node(identifier, s, e) {}
};

struct Int8Literal : Node {
  int8_t Value;
  Int8Literal(int s, int e) : Node(int8Literal, s, e) {}
};

struct Int16Literal : Node {
  int16_t Value;
  Int16Literal(int s, int e) : Node(int16Literal, s, e) {}
};

struct Int32Literal : Node {
  int32_t Value;
  Int32Literal(int s, int e) : Node(int32Literal, s, e) {}
};

struct Int64Literal : Node {
  int64_t Value;
  Int64Literal(int s, int e) : Node(int64Literal, s, e) {}
};

struct Return : Node {
  Node *Value;
  Return(int s, int e) : Node(returnNode, s, e) {}
  ~Return() {
    delete Value;
  }
};

struct Binary : Node {
  Node *Lhs;
  Node *Rhs;
  Binary(const NodeType &type, int s, int e) : Node(type, s, e) {}
  ~Binary() {
    delete Lhs;
    delete Rhs;
  }
};

struct Equal : Binary {
  Equal(int s, int e) : Binary(equal, s, e) {}
};

struct Less : Binary {
  Less(int s, int e) : Binary(less, s, e) {}
};

struct Greater : Binary {
  Greater(int s, int e) : Binary(greater, s, e) {}
};

struct LessEqual : Binary {
  LessEqual(int s, int e) : Binary(lessEqual, s, e) {}
};

struct GreaterEqual : Binary {
  GreaterEqual(int s, int e) : Binary(greaterEqual, s, e) {}
};

struct ShiftLeft : Binary {
  ShiftLeft(int s, int e) : Binary(shiftLeft, s, e) {}
};

struct ShiftRight : Binary {
  ShiftRight(int s, int e) : Binary(shiftRight, s, e) {}
};

struct Plus : Binary {
  Plus(int s, int e) : Binary(plus, s, e) {}
};

struct UnaryMinus : Node {
  Node *To;
  UnaryMinus(int s, int e) : To(nullptr), Node(unaryMinus, s, e) {}
  ~UnaryMinus() {
    delete To;
  }
};

struct LogicalNegate : Node {
  Node *To;
  LogicalNegate(int s, int e) : To(nullptr), Node(logicalNegate, s, e) {}
  ~LogicalNegate() {
    delete To;
  }
};

struct Minus : Binary {
  Minus(int s, int e) : Binary(minus, s, e) {}
};

struct Multiply : Binary {
  Multiply(int s, int e) : Binary(multiply, s, e) {}
};

struct Div : Binary {
  Div(int s, int e) : Binary(div, s, e) {}
};

struct Modulo : Binary {
  Modulo(int s, int e) : Binary(modulo, s, e) {}
};

struct BitwiseAnd : Binary {
  BitwiseAnd(int s, int e) : Binary(bitwiseAnd, s, e) {}
};

struct BitwiseOr : Binary {
  BitwiseOr(int s, int e) : Binary(bitwiseOr, s, e) {}
};

struct If : Node {
  Node *Cond;
  Node *Body;
  If(int s, int e) : Cond(nullptr), Body(nullptr), Node(ifNode, s, e) {}
  ~If() {
    delete Cond;
    delete Body;
  }
};

struct ElseIf : Node {
  Node *Cond;
  Node *Body;
  ElseIf(int s, int e) : Cond(nullptr), Body(nullptr), Node(elseIf, s, e) {}
  ~ElseIf() {
    delete Cond;
    delete Body;
  }
};

struct Else : Node {
  Node *Body;
  Else(int s, int e) : Body(nullptr), Node(elseNode, s, e) {}
  ~Else() {
    delete Body;
  }
};

struct IfElseBlock : Node {
  std::vector<Node *> Conds;
  IfElseBlock(int s, int e) : Node(ifElseBlock, s, e) {}
};

struct While : Node {
  Node *Cond;
  Node *Body;
  While(int s, int e) : Cond(nullptr), Body(nullptr), Node(whileNode, s, e) {}
  ~While() {
    delete Cond;
    delete Body;
  }
};

} // namespace av