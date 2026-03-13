#include "desugar.hpp"
#include "ast.hpp"
#include <map>
#include <utility>

namespace av {

bool desugar(Node *t) {
  std::map<std::string, std::pair<FunctionDecl *, FunctionBody *>> funcs;
  auto make_if_else_block = [&](std::vector<Node *> &if_block) -> Node * {
    if (if_block.empty()) {
      return nullptr;
    }
    if (if_block.size() == 1) {
      Node *t = if_block[0];
      if_block.clear();
      return t;
    }
    IfElseBlock *block = new IfElseBlock();
    block->Conds.swap(if_block);
    return block;
  };
  int global = 1;
  auto rec = [&](auto &&self, Node *t) -> std::vector<Node *> {
    std::vector<Node *> ret;
    switch (t->type) {
    case block: {
      auto Funcs = funcs;
      int was = std::exchange(global, 0);
      std::vector<Node *> modified_stmts;
      std::vector<Node *> if_block;
      auto push = [&]() {
        auto block = make_if_else_block(if_block);
        if (block != nullptr) {
          modified_stmts.push_back(block);
        }
      };
      for (Node *&i : ((Block *)t)->stmts) {
        std::vector<Node *> des = self(self, i);
        if (des.size() != 1) { // not if, else if or else
          push();
          for (Node *&j : des) {
            modified_stmts.push_back(j);
          }
        } else {
          if (des[0]->type == ifNode) {
            push();
            if_block.push_back(des[0]);
          } else if (des[0]->type == elseIf) {
            if (if_block.empty()) {
              throw std::runtime_error("Cannot use else if without an if before it");
            }
            if_block.push_back(des[0]);
          } else if (des[0]->type == elseNode) {
            if (if_block.empty()) {
              throw std::runtime_error("Cannot use else if without an if before it");
            }
            if_block.push_back(des[0]);
            push();
          } else {
            push();
            modified_stmts.push_back(des[0]);
          }
        }
      }
      ((Block *)t)->stmts = modified_stmts;
      if (!was) {
        funcs = Funcs;
      }
      global = was;
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
    case elseNode: {
      self(self, ((Else *)t)->Body);
    } break;
    case elseIf: {
      self(self, ((ElseIf *)t)->Body);
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
  // if (t->type == block) {
  //   std::vector<Node *> res = rec(rec, t);
  //   Block *flat = new Block();
  //   for (int i = 0; i < int(res.size()); ++i) {
  //     if (res[i]->type != block) {
  //       flat->stmts.push_back(res[i]);
  //     } else {
  //       for (Node *&j : ((Block*)res[i])->stmts) {
  //         flat->stmts.push_back(j);
  //       }
  //       ((Block*)res[i])->stmts.clear();
  //     }
  //   }
  //   t = flat;
  // } else {
    rec(rec, t);
  // }
  return funcs.contains("main");
}

} // namespace av