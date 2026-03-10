#include "ast.hpp"
#include <string>
#include <algorithm>

std::string av::to_string(av::NodeType type) {
  static const std::array<std::string, int(av::__count_NodeType)> a({
    "Block", "FunctionDecl", "FunctionBody", "VariableDecl",
    "Assign", "Int8Literal", "Int16Literal", "Int32Literal",
    "Int64Literal", "AddressOf", "Dereference", "FunctionCall",
    "Value", "Identifier", "Return"
  });
  return a[int(type)];
}

std::string av::to_string(av::Type type) {
  static const std::array<std::string, int(av::__count_Type)> a({
    "Int8", "Int16", "Int32", "Int64", "Int8Ptr", "Int16Ptr",
    "Int32Ptr", "Int64Ptr", "VoidPtr", "Void"
  });
  return a[int(type)];
}

av::Type av::type_from_string(const std::string &s) {
  static const std::array<std::string, int(av::__count_Type)> a({
    "int8", "int16", "int32", "int64", "int8*", "int16*",
    "int32*", "int64*", "void*", "void"
  });
  return av::Type(std::find(a.begin(), a.end(), s) - a.begin());
}

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

std::ostream &av::Node::print(std::ostream &os, int dep) const {
  std::string tab(dep, ' ');
  os << tab << av::to_string(type) << '\n';
  switch (type) {
  case block: {
    for (Node *&i : ((Block *)this)->stmts) {
      i->print(os, dep + 1);
    }
  } break;
  case functionDecl: {
    FunctionDecl *t = (FunctionDecl *)this;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "ReturnType: " << t->ReturnType << '\n';
    os << tab << "ParamTypes: " << t->ParamTypes << '\n';
  } break;
  case functionBody: {
    FunctionBody *t = (FunctionBody *)this;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "Params: " << t->Params << '\n';
    os << tab << "Block:\n";
    t->Block->print(os, dep + 1);
  } break;
  case variableDecl: {
    VariableDecl *t = (VariableDecl *)this;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "Type: " << t->VariableType << '\n';
  } break;
  case assign: {
    Assign *t = (Assign *)this;
    os << tab << "To:\n";
    t->To->print(os, dep + 1);
    os << tab << "Value:\n";
    t->Value->print(os, dep + 1);
  } break;
  case addressOf: {
    os << "Name: " << ((AddressOf *)this)->Name << '\n';
  } break;
  case dereference: {
    os << "Name: " << ((Dereference *)this)->Name << '\n';
  } break;
  case functionCall: {
    FunctionCall *t = (FunctionCall *)this;
    os << tab << "Name: " << t->Name << '\n';
    os << tab << "Params:\n";
    for (Node *&i : t->Params) {
      i->print(os, dep + 1);
    }
  } break;
  case identifier: {
    os << tab << "Name: " << ((Identifier *)this)->Name << '\n';
  } break;
  case int8Literal: {
    os << tab << "Value: " << ((Int8Literal *)this)->Value << '\n';
  } break;
  case int16Literal: {
    os << tab << "Value: " << ((Int16Literal *)this)->Value << '\n';
  } break;
  case int32Literal: {
    os << tab << "Value: " << ((Int32Literal *)this)->Value << '\n';
  } break;
  case int64Literal: {
    os << tab << "Value: " << ((Int64Literal *)this)->Value << '\n';
  } break;
  case returnNode: {
    os << tab << "Value:\n";
    ((Return* )this)->Value->print(os, dep + 1);
  } break;
  }
  return os;
}

std::ostream &av::operator<<(std::ostream &os, const av::Node &t) {
  return t.print(os);
}