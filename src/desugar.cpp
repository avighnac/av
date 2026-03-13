#include "desugar.hpp"
#include "ast.hpp"
#include <map>
#include <utility>

namespace av {

bool desugar(Node *t) {
  std::map<std::string, std::pair<FunctionDecl *, FunctionBody *>> funcs;
  auto rec = [&](auto &&self, Node *t) -> std::vector<Node *> {
    std::vector<Node *> ret;
    switch (t->type) {
    case block: {
      auto Funcs = funcs;
      std::vector<Node *> modified_stmts;
      for (Node *&i : ((Block *)t)->stmts) {
        for (Node *&j : self(self, i)) {
          modified_stmts.push_back(j);
        }
      }
      ((Block *)t)->stmts = modified_stmts;
      funcs = Funcs;
    } break;
    case functionDecl: {
      FunctionDecl *u = (FunctionDecl *)t;
      funcs[u->Name].first = u;
    } break;
    case assign: {
      // desugar `int32 x = 5`
      Assign *u = (Assign *)t;
      if (u->To->type != variableDecl) {
        break;
      }
      Assign *assign = new Assign();
      Identifier *ident = new Identifier();
      ident->Name = ((VariableDecl *)u->To)->Name;
      assign->To = ident;
      assign->Value = std::exchange(u->Value, nullptr);
      ret.push_back(u->To);
      ret.push_back(assign);
    } break;
    case functionBody: {
      FunctionBody *u = (FunctionBody *)t;
      if (!funcs.contains(u->Name)) {
        // create a functionDecl
        FunctionDecl *decl = new FunctionDecl();
        decl->Name = u->Name;
        decl->ParamTypes = u->ParamTypes;
        decl->ReturnType = u->ReturnType;
        ret.push_back(decl);
      }
      funcs[u->Name].second = u;
      self(self, u->Body);
    } break;
    case ifNode: {
      self(self, ((If *)t)->Body);
    } break;
    case whileNode: {
      self(self, ((While *)t)->Body);
    } break;
    default: {
      break;
    }
    }
    if (t->type != assign || ((Assign *)t)->To->type != variableDecl) {
      ret.push_back(t);
    }
    return ret;
  };
  if (t->type == block) {
    std::vector<Node *> modified_stmts;
    for (Node *&i : ((Block *)t)->stmts) {
      for (Node *&j : rec(rec, i)) {
        modified_stmts.push_back(j);
      }
    }
    ((Block *)t)->stmts = modified_stmts;
  } else {
    rec(rec, t);
  }
  return funcs.contains("main");
}

} // namespace av