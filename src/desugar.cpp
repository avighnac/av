#include "desugar.hpp"
#include "ast.hpp"
#include <map>
#include <utility>

namespace av {

void desugar(Node *t) {
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
    } break;
    default: {
      break;
    }
    }
    ret.push_back(t);
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
}

} // namespace av