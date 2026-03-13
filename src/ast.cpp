#include "ast.hpp"
#include <algorithm>
#include <string>

// clang-format off
std::string av::to_string(av::NodeType type) {
  static const std::array<std::string, int(av::__count_NodeType)> a({
    "Block", "FunctionDecl", "FunctionBody", "VariableDecl",
    "Assign", "Int8Literal", "Int16Literal", "Int32Literal",
    "Int64Literal", "AddressOf", "Dereference", "FunctionCall",
    "Identifier", "Return", "Equal", "Less", "Greater",
    "LessEqual", "GreaterEqual", "ShiftLeft", "ShiftRight",
    "Plus", "Minus", "UnaryMinus", "Multiply", "Div", "Modulo",
    "BitwiseAnd", "BitwiseOr", "LogicalNeg", "If", "ElseIf", "Else", "While",
    "IfElseBlock"
  });
  return a[int(type)];
}

std::string av::to_string(av::Type type) {
  static const std::array<std::string, int(av::__count_Type)> a({
    "Int8", "Int16", "Int32", "Int64", "Int8Ptr", "Int16Ptr",
    "Int32Ptr", "Int64Ptr", "VoidPtr", "Void"});
  return a[int(type)];
}

av::Type av::type_from_string(const std::string &s) {
  static const std::array<std::string, int(av::__count_Type)> a({
    "int8", "int16", "int32", "int64", "int8*", "int16*",
    "int32*", "int64*", "void*", "void"
  });
  return av::Type(std::find(a.begin(), a.end(), s) - a.begin());
}
// clang-format on

template <typename T>
  requires std::same_as<T, av::NodeType> || std::same_as<T, av::Type>
std::ostream &av::operator<<(std::ostream &os, const T &type) {
  return os << to_string(type);
}

template <typename T>
std::ostream &av::operator<<(std::ostream &os, const std::vector<T> &v) {
  os << "[";
  for (std::size_t i = 0; i < v.size(); ++i) {
    os << v[i];
    if (i != v.size() - 1) {
      os << ", ";
    }
  }
  return os << "]";
}

std::ostream &av::print(const Node *u, std::ostream &os, int dep) {
  if (u == nullptr) {
    return os;
  }
  std::string tab(dep, ' ');
  os << tab << av::to_string(u->type) << '\n';
  switch (u->type) {
  case block: {
    for (Node *&i : ((Block *)u)->stmts) {
      print(i, os, dep + 1);
    }
  } break;
  case functionDecl: {
    FunctionDecl *t = (FunctionDecl *)u;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "ReturnType: " << t->ReturnType << '\n';
    os << tab << "ParamTypes: " << t->ParamTypes << '\n';
  } break;
  case functionBody: {
    FunctionBody *t = (FunctionBody *)u;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "ReturnType: " << t->ReturnType << '\n';
    os << tab << "ParamTypes: " << t->ParamTypes << '\n';
    os << tab << "Params: " << t->Params << '\n';
    os << tab << "Body:\n";
    print(t->Body, os, dep + 1);
  } break;
  case variableDecl: {
    VariableDecl *t = (VariableDecl *)u;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "Type: " << t->VariableType << '\n';
  } break;
  case assign: {
    Assign *t = (Assign *)u;
    os << tab << "To:\n";
    print(t->To, os, dep + 1);
    os << tab << "Value:\n";
    print(t->Value, os, dep + 1);
  } break;
  case addressOf: {
    os << tab << "Name: " << ((AddressOf *)u)->Name << '\n';
  } break;
  case dereference: {
    os << tab << "Name: " << ((Dereference *)u)->Name << '\n';
  } break;
  case functionCall: {
    FunctionCall *t = (FunctionCall *)u;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "Params:\n";
    for (Node *&i : t->Params) {
      print(i, os, dep + 1);
    }
  } break;
  case identifier: {
    os << tab << "Name: " << ((Identifier *)u)->Name << '\n';
  } break;
  case int8Literal: {
    os << tab << "Value: " << int(((Int8Literal *)u)->Value) << '\n';
  } break;
  case int16Literal: {
    os << tab << "Value: " << ((Int16Literal *)u)->Value << '\n';
  } break;
  case int32Literal: {
    os << tab << "Value: " << ((Int32Literal *)u)->Value << '\n';
  } break;
  case int64Literal: {
    os << tab << "Value: " << ((Int64Literal *)u)->Value << '\n';
  } break;
  case returnNode: {
    os << tab << "Value:\n";
    print(((Return *)u)->Value, os, dep + 1);
  } break;
  case equal:
  case less:
  case greater:
  case lessEqual:
  case greaterEqual:
  case shiftLeft:
  case shiftRight:
  case plus:
  case minus:
  case multiply:
  case div:
  case modulo:
  case bitwiseAnd:
  case bitwiseOr: {
    os << tab << "Lhs:\n";
    print(((Binary *)u)->Lhs, os, dep + 1);
    os << tab << "Rhs:\n";
    print(((Binary *)u)->Rhs, os, dep + 1);
  } break;
  case unaryMinus: {
    os << tab << "To:\n";
    print(((UnaryMinus *)u)->To, os, dep + 1);
  } break;
  case logicalNegate: {
    os << tab << "To:\n";
    print(((LogicalNegate *)u)->To, os, dep + 1);
  } break;
  case ifNode: {
    If *t = (If *)u;
    os << tab << "Cond:\n";
    print(t->Cond, os, dep + 1);
    os << tab << "Body:\n";
    print(t->Body, os, dep + 1);
  } break;
  case elseIf: {
    ElseIf *t = (ElseIf *)u;
    os << tab << "Cond:\n";
    print(t->Cond, os, dep + 1);
    os << tab << "Body:\n";
    print(t->Body, os, dep + 1);
  } break;
  case elseNode: {
    Else *t = (Else *)u;
    os << tab << "Body:\n";
    print(t->Body, os, dep + 1);
  } break;
  case ifElseBlock: {
    IfElseBlock *t = (IfElseBlock *)u;
    os << tab << "Conds:\n";
    for (Node *&i : t->Conds) {
      print(i, os, dep + 1);
    }
  } break;
  case whileNode: {
    While *t = (While *)u;
    os << tab << "Cond:\n";
    print(t->Cond, os, dep + 1);
    os << tab << "Body:\n";
    print(t->Body, os, dep + 1);
  } break;
  }
  return os;
}

std::ostream &av::operator<<(std::ostream &os, const av::Node &t) {
  return print(&t, os);
}