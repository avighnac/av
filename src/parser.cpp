#include "parser.hpp"
#include "ast.hpp"
#include "errors.hpp"
#include "tokenizer.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace av {

Node *_parse(tokenizer &tk) {
  if (tk.size() == 0) {
    return nullptr;
  }
  int s = tk[0].s, e = tk[tk.size() - 1].e;
  if (tk.size() >= 2 && tk[0].type == Tk_OpenParen && tk[tk.size() - 1].type == Tk_CloseParen) {
    tk.get();
    tk.pop_back();
  }
  int bracepair_cnt = 0;
  for (int i = 0, bal = 0; i < tk.size(); ++i) {
    int pbal = bal;
    bal += (tk[i].type == Tk_OpenBrace) - (tk[i].type == Tk_CloseBrace);
    bracepair_cnt += pbal == 1 && bal == 0;
  }
  if (tk.has([&](const Token &t) { return t.type == Tk_Semicolon; }) != -1 || bracepair_cnt > 1) {
    int bal = 0;
    Block *t = new Block(s, e);
    tokenizer stmt;
    while (tk.peek()) {
      Token token = tk.get();
      stmt.push_back(token);
      bal += (token.type == Tk_OpenBrace) + (token.type == Tk_OpenParen) - (token.type == Tk_CloseBrace) - (token.type == Tk_CloseParen);
      if ((token.type == Tk_Semicolon || token.type == Tk_CloseBrace) && bal == 0) {
        if (token.type == Tk_Semicolon) {
          stmt.pop_back();
        }
        // for functions, keep everything intact, but for pure blocks we do wanna pop the first { and last }
        if (stmt[0].type == Tk_OpenBrace && stmt[stmt.size() - 1].type == Tk_CloseBrace) {
          stmt.get();
          stmt.pop_back();
        }
        t->stmts.push_back(_parse(stmt));
        stmt.clear();
      }
    }
    return t;
  }
  // variabledecl: int32 x
  if (tk.size() == 2 && tk[0].type == Tk_Type && tk[1].type == Tk_Identifier) {
    VariableDecl *t = new VariableDecl(s, e);
    t->VariableType = type_from_string(tk[0].token);
    t->Name = tk[1].token;
    return t;
  }
  // function
  if (tk.size() >= 3 && tk[0].type == Tk_Type && tk[1].type == Tk_Identifier && tk[2].type == Tk_OpenParen) {
    // could either be body or decl
    // to find out, we check the character after the matching )
    int loc = 3, bal = 1;
    while (bal != 0 && loc < tk.size()) {
      bal += (tk[loc].type == Tk_OpenParen) - (tk[loc].type == Tk_CloseParen);
      loc++;
    }
    if (bal != 0) {
      throw Error("No matching ) found for ( in function " + tk[2].token, s, e);
    }
    if (loc == tk.size()) {
      // decl: int32 f(...)
      FunctionDecl *t = new FunctionDecl(s, e);
      t->ReturnType = type_from_string(tk[0].token);
      t->Name = tk[1].token;
      bal = 1;
      tk.get();
      tk.get();
      tk.get();
      int grab = tk[0].type != Tk_CloseParen;
      while (bal != 0) {
        Token token = tk.get();
        bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
        if (grab) {
          if (token.type != Tk_Type) {
            throw Error(token.token + " is not a type", token.s, token.e);
          }
          t->ParamTypes.push_back(type_from_string(token.token));
          grab = 0;
        }
        if (token.type == Tk_Comma) {
          grab = 1;
        }
      }
      return t;
    }
    // body: int32 f(...) {
    //   ...
    // }
    if (tk[loc].type != Tk_OpenBrace) {
      throw Error("expected { before start of function body for " + tk[2].token, s, e);
    }
    if (tk[tk.size() - 1].type != Tk_CloseBrace) {
      throw Error("mo matching } found for { in function " + tk[2].token, s, e);
    }
    tk.pop_back();
    FunctionBody *t = new FunctionBody(s, e);
    t->Name = tk[1].token;
    t->ReturnType = type_from_string(tk[0].token);
    bal = 1;
    tk.get();
    tk.get();
    tk.get();
    loc = 0;
    while (bal != 0) {
      Token token = tk.get();
      bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
      if (loc == 0 && token.type != Tk_CloseParen) {
        if (token.type != Tk_Type) {
          throw Error(token.token + " is not a type", token.s, token.e);
        }
        t->ParamTypes.push_back(type_from_string(token.token));
      }
      if (loc == 1) {
        if (token.type != Tk_Identifier) {
          throw Error(token.token + " is not an identifier", token.s, token.e);
        }
        t->Params.push_back(token.token);
      }
      loc++;
      if (token.type == Tk_Comma) {
        loc = 0;
      }
    }
    tk.get();
    t->Body = _parse(tk);
    return t;
  }
  // addressof
  if (tk.size() == 2 && tk[0].type == Tk_Amp && tk[1].type == Tk_Identifier) {
    AddressOf *t = new AddressOf(s, e);
    t->Name = tk[1].token;
    return t;
  }
  // dereference
  if (tk.size() == 2 && tk[0].type == Tk_Star && tk[1].type == Tk_Identifier) {
    Dereference *t = new Dereference(s, e);
    t->Name = tk[1].token;
    return t;
  }
  // numeric literals
  // default case (we assume int32 literal)
  if (tk.size() == 1 && tk[0].type == Tk_Number) {
    Int32Literal *t = new Int32Literal(s, e);
    try {
      t->Value = std::stoi(tk[0].token);
    } catch (const std::exception &e) {
      throw Error(tk[0].token + " is out of range for an int32 literal", tk[0].s, tk[0].e);
    }
    return t;
  }
  if (tk.size() == 3 && tk[0].type == Tk_Number && tk[1].type == Tk_Underscore && tk[2].type == Tk_Number) {
    if (tk[2].token != "8" && tk[2].token != "16" && tk[2].token != "32" && tk[2].token != "64") {
      throw Error("invalid underscore suffix (_" + tk[2].token + "); must be one of 8/16/32/64", tk[2].s, tk[2].e);
    }
    int64_t x;
    try {
      x = std::stoll(tk[0].token);
    } catch (const std::exception &e) {
      throw Error(tk[0].token + " is too huge", tk[0].s, tk[0].e);
    }
    if (tk[2].token == "8") {
      if (x < std::numeric_limits<int8_t>::min() || x > std::numeric_limits<int8_t>::max()) {
        throw Error(tk[0].token + " is out of range for an int8 literal", tk[0].s, tk[0].e);
      }
      Int8Literal *t = new Int8Literal(s, e);
      t->Value = x;
      return t;
    }
    if (tk[2].token == "16") {
      if (x < std::numeric_limits<int16_t>::min() || x > std::numeric_limits<int16_t>::max()) {
        throw Error(tk[0].token + " is out of range for an int16 literal", tk[0].s, tk[0].e);
      }
      Int16Literal *t = new Int16Literal(s, e);
      t->Value = x;
      return t;
    }
    if (tk[2].token == "32") {
      if (x < std::numeric_limits<int32_t>::min() || x > std::numeric_limits<int32_t>::max()) {
        throw Error(tk[0].token + " is out of range for an int32 literal", tk[0].s, tk[0].e);
      }
      Int32Literal *t = new Int32Literal(s, e);
      t->Value = x;
      return t;
    }
    Int64Literal *t = new Int64Literal(s, e);
    t->Value = x;
    return t;
  }
  int has_assign = tk.has([&](const Token &t) { return t.type == Tk_Assign; });
  // assign
  if (has_assign != -1) {
    tokenizer left, right;
    for (int i = 0; i < has_assign; ++i) {
      left.push_back(tk[i]);
    }
    for (int i = has_assign + 1; i < int(tk.size()); ++i) {
      right.push_back(tk[i]);
    }
    Assign *t = new Assign(s, e);
    t->To = _parse(left);
    t->Value = _parse(right);
    return t;
  }
  // if
  if (tk.size() >= 1 && tk[0].type == Tk_If) {
    if (tk.size() == 1 || tk[1].type != Tk_OpenParen) {
      throw Error("Expected a statement after 'if'", tk[0].s, tk[1].e);
    }
    int bal = 1;
    tk.get();
    tk.get();
    tokenizer cond;
    while (bal != 0) {
      Token token = tk.get();
      bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
      cond.push_back(token);
    }
    cond.pop_back();
    If *t = new If(s, e);
    t->Cond = _parse(cond);
    tk.get();
    tk.pop_back();
    t->Body = _parse(tk);
    return t;
  }
  // else if
  if (tk.size() >= 2 && tk[0].type == Tk_Else && tk[1].type == Tk_If) {
    tk.get();
    if (tk.size() == 1 || tk[1].type != Tk_OpenParen) {
      throw Error("expected a statement after 'else if'", tk[0].s, tk[1].e);
    }
    int bal = 1;
    tk.get();
    tk.get();
    tokenizer cond;
    while (bal != 0) {
      Token token = tk.get();
      bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
      cond.push_back(token);
    }
    cond.pop_back();
    ElseIf *t = new ElseIf(s, e);
    t->Cond = _parse(cond);
    tk.get();
    tk.pop_back();
    t->Body = _parse(tk);
    return t;
  }
  // else
  if (tk.size() >= 1 && tk[0].type == Tk_Else) {
    if (tk.size() == 1 || tk[1].type != Tk_OpenBrace) {
      throw Error("expected a block after 'else'", tk[0].s, tk[1].e);
    }
    tk.get();
    tk.get();
    tk.pop_back();
    Else *t = new Else(s, e);
    t->Body = _parse(tk);
    return t;
  }
  // while
  if (tk.size() >= 1 && tk[0].type == Tk_While) {
    if (tk.size() == 1 || tk[1].type != Tk_OpenParen) {
      throw Error("expected a statement after 'while'", tk[0].s, tk[1].e);
    }
    int bal = 1;
    tk.get();
    tk.get();
    tokenizer cond;
    while (bal != 0) {
      Token token = tk.get();
      bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
      cond.push_back(token);
    }
    cond.pop_back();
    While *t = new While(s, e);
    t->Cond = _parse(cond);
    tk.get();
    tk.pop_back();
    t->Body = _parse(tk);
    return t;
  }
  // for: desugar to while
  if (tk.size() >= 1 && tk[0].type == Tk_For) {
    int s = tk[0].s;
    if (tk.size() == 1 || tk[1].type != Tk_OpenParen) {
      throw Error("expected a statement after 'for'", tk[0].s, tk[1].e);
    }
    int bal = 1;
    tk.get();
    tk.get();
    tokenizer middle;
    while (bal != 0) {
      Token token = tk.get();
      bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
      middle.push_back(token);
    }
    int e = middle[middle.size() - 1].e;
    middle.pop_back();

    std::vector<tokenizer> parts;
    tokenizer part;
    for (int i = 0; i < middle.size(); ++i) {
      if (middle[i].type == Tk_Semicolon) {
        parts.push_back(part);
        part.clear();
        continue;
      }
      part.push_back(middle[i]);
    }
    parts.push_back(part);

    if (parts.size() != 3) {
      throw Error("for (...) conditions invalid (need exactly 3 statements)", s, e);
    }

    tk.get();
    tk.pop_back();
    Block *t = new Block(s, e);
    t->stmts.push_back(_parse(parts[0]));
    While *w = new While(0, 0); // fixed3...
    w->Cond = _parse(parts[1]);
    Block *bodyBlock = new Block(0, 0); // fixed1...
    bodyBlock->stmts.push_back(_parse(tk));
    bodyBlock->s = bodyBlock->stmts[0]->s, bodyBlock->e = bodyBlock->stmts[0]->e; // ...here1
    Block *body = new Block(bodyBlock->s, 0);                                     // fixed2...
    body->stmts.push_back(bodyBlock);
    body->stmts.push_back(_parse(parts[2]));
    body->e = body->stmts[1]->e; // ...here2
    w->Body = body;
    w->s = w->Cond->s, w->e = w->Body->e; // ...here3
    t->stmts.push_back(w);
    return t;
  }
  // unaryMinus
  if (tk.size() >= 1 && tk[0].type == Tk_Minus) {
    UnaryMinus *t = new UnaryMinus(s, e);
    tk.get();
    t->To = _parse(tk);
    return t;
  }
  // logical negate
  if (tk.size() >= 1 && tk[0].type == Tk_LogicalNegate) {
    LogicalNegate *t = new LogicalNegate(s, e);
    tk.get();
    t->To = _parse(tk);
    return t;
  }
  // functioncall: f(...
  if (tk.size() >= 2 && tk[0].type == Tk_Identifier && tk[1].type == Tk_OpenParen) {
    FunctionCall *t = new FunctionCall(s, e);
    t->Name = tk[0].token;
    tk.get();
    tk.get();
    tokenizer param;
    int bal = 1;
    while (bal != 0) {
      Token token = tk.get();
      bal += (token.type == Tk_OpenParen) - (token.type == Tk_CloseParen);
      param.push_back(token);
      if (token.type == Tk_Comma) {
        param.pop_back();
        t->Params.push_back(_parse(param));
        param.clear();
      }
    }
    param.pop_back();
    if (param.size() != 0) {
      t->Params.push_back(_parse(param));
    }
    return t;
  }
  // return
  if (tk.size() >= 1 && tk[0].type == Tk_Return) {
    Return *t = new Return(s, e);
    tk.get();
    t->Value = _parse(tk);
    return t;
  }
  // 14 binops: bitwand, bitwor, equal, less, greater, lessEqual, greaterEqual, shiftLeft, shiftRight, plus, minus, multiply, div, modulo
  // &, |, = < > <= >= << >> + - * / %
  std::array binops{bitwiseAnd, bitwiseOr, equal, less, greater, lessEqual, greaterEqual, shiftLeft, shiftRight, plus, minus, multiply, div, modulo};
  std::array binops_tk{Tk_Amp, Tk_Bar, Tk_Equal, Tk_Less, Tk_Greater, Tk_LessEqual, Tk_GreaterEqual, Tk_ShiftLeft, Tk_ShiftRight, Tk_Plus, Tk_Minus, Tk_Star, Tk_Div, Tk_Percent};
  for (int i = 0; i < int(binops.size()); ++i) {
    int has = tk.has([&](const Token &t) { return t.type == binops_tk[i]; });
    if (has == -1) {
      continue;
    }
    tokenizer left, right;
    for (int i = 0; i < has; ++i) {
      left.push_back(tk[i]);
    }
    for (int i = has + 1; i < int(tk.size()); ++i) {
      right.push_back(tk[i]);
    }
    Binary *t = new Binary(binops[i], s, e);
    t->Lhs = _parse(left);
    t->Rhs = _parse(right);
    return t;
  }
  // identifier
  if (tk.size() == 1 && tk[0].type == Tk_Identifier) {
    Identifier *t = new Identifier(s, e);
    t->Name = tk[0].token;
    return t;
  }
  throw Error("unknown node type", s, e);
}

Node *parse(tokenizer &tk) {
  Node *t = _parse(tk);
  if (t->type != block) {
    Block *b = new Block(0, 0); // fixed...
    b->stmts.push_back(t);
    b->s = b->stmts[0]->s, b->e = b->stmts[0]->e; // ...here
    return b;
  }
  return t;
}

} // namespace av