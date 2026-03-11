#include "parser.hpp"
#include "ast.hpp"
#include "tokenizer.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace av {

Node *parse(tokenizer &tk) {
  if (tk.size() == 0) {
    return nullptr;
  }
  int is_block = 0;
  for (int i = 0, bal = 0; i < int(tk.size()) && !is_block; ++i) {
    bal += (tk[i].type == Tk_OpenBrace) - (tk[i].type == Tk_CloseBrace);
    is_block |= bal == 0 && tk[i].type == Tk_Semicolon;
  }
  // block
  if (is_block) {
    int bal = 0;
    Block *t = new Block();
    tokenizer stmt;
    while (tk.peek()) {
      Token token = tk.get();
      stmt.push_back(token);
      bal += (token.type == Tk_OpenBrace) - (token.type == Tk_CloseBrace);
      if ((token.type == Tk_Semicolon || token.type == Tk_CloseBrace) && bal == 0) {
        if (token.type == Tk_Semicolon) {
          stmt.pop_back();
        }
        // for functions, keep everything intact, but for pure blocks we do wanna pop the first { and last }
        if (stmt[0].type == Tk_OpenBrace && stmt[stmt.size() - 1].type == Tk_CloseBrace) {
          stmt.get();
          stmt.pop_back();
        }
        // desugar `int32 x = 5`
        if (stmt.size() >= 3 && stmt[0].type == Tk_Type && stmt[1].type == Tk_Identifier && stmt[2].type == Tk_Equal) {
          tokenizer left;
          left.push_back(stmt.get());
          Token token = stmt.get();
          left.push_back(token);
          t->stmts.push_back(parse(left));
          stmt.push_front(token);
        }
        // desugar `int32 main() {`
        t->stmts.push_back(parse(stmt));
        stmt.clear();
      }
    }
    return t;
  }
  // variabledecl: int32 x
  if (tk.size() == 2 && tk[0].type == Tk_Type && tk[1].type == Tk_Identifier) {
    VariableDecl *t = new VariableDecl();
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
      throw std::runtime_error("No matching ) found for ( in function " + tk[2].token);
    }
    if (loc == tk.size()) {
      // decl: int32 f(...)
      FunctionDecl *t = new FunctionDecl();
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
            throw std::runtime_error(token.token + " is not a type");
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
      throw std::runtime_error("Expected { before start of function body for " + tk[2].token);
    }
    if (tk[tk.size() - 1].type != Tk_CloseBrace) {
      throw std::runtime_error("No matching } found for { in function " + tk[2].token);
    }
    tk.pop_back();
    FunctionBody *t = new FunctionBody;
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
          throw std::runtime_error(token.token + " is not a type");
        }
        t->ParamTypes.push_back(type_from_string(token.token));
      }
      if (loc == 1) {
        if (token.type != Tk_Identifier) {
          throw std::runtime_error(token.token + " is not an identifier");
        }
        t->Params.push_back(token.token);
      }
      loc++;
      if (token.type == Tk_Comma) {
        loc = 0;
      }
    }
    tk.get();
    t->Body = parse(tk);
    return t;
  }
  // addressof
  if (tk.size() == 2 && tk[0].type == Tk_Amp && tk[1].type == Tk_Identifier) {
    AddressOf *t = new AddressOf();
    t->Name = tk[1].token;
    return t;
  }
  // dereference
  if (tk.size() == 2 && tk[0].type == Tk_Star && tk[1].type == Tk_Identifier) {
    Dereference *t = new Dereference();
    t->Name = tk[1].token;
    return t;
  }
  // numeric literals
  // default case (we assume int32 literal)
  if (tk.size() == 1 && tk[0].type == Tk_Number) {
    Int32Literal *t = new Int32Literal();
    try {
      t->Value = std::stoi(tk[0].token);
    } catch (const std::exception &e) {
      throw std::runtime_error(tk[0].token + " is out of range for an int32 literal");
    }
    return t;
  }
  if (tk.size() == 3 && tk[0].type == Tk_Number && tk[1].type == Tk_Underscore && tk[2].type == Tk_Number) {
    if (tk[2].token != "8" && tk[2].token != "16" && tk[2].token != "32" && tk[2].token != "64") {
      throw std::runtime_error("Invalid underscore suffix (_" + tk[2].token + "). Only 8/16/32/64 are valid");
    }
    int64_t x;
    try {
      x = std::stoll(tk[0].token);
    } catch (const std::exception &e) {
      throw std::runtime_error(tk[0].token + " is too huge");
    }
    if (tk[2].token == "8") {
      if (x < std::numeric_limits<int8_t>::min() || x > std::numeric_limits<int8_t>::max()) {
        throw std::runtime_error(tk[0].token + " is out of range for an int8 literal");
      }
      Int8Literal *t = new Int8Literal();
      t->Value = x;
      return t;
    }
    if (tk[2].token == "16") {
      if (x < std::numeric_limits<int16_t>::min() || x > std::numeric_limits<int16_t>::max()) {
        throw std::runtime_error(tk[0].token + " is out of range for an int16 literal");
      }
      Int16Literal *t = new Int16Literal();
      t->Value = x;
      return t;
    }
    if (tk[2].token == "32") {
      if (x < std::numeric_limits<int32_t>::min() || x > std::numeric_limits<int32_t>::max()) {
        throw std::runtime_error(tk[0].token + " is out of range for an int32 literal");
      }
      Int32Literal *t = new Int32Literal();
      t->Value = x;
      return t;
    }
    Int64Literal *t = new Int64Literal();
    t->Value = x;
    return t;
  }
  int has_equal = -1;
  for (int i = 0, bal = 0; i < int(tk.size()) && has_equal == -1; ++i) {
    bal += (tk[i].type == Tk_OpenBrace) - (tk[i].type == Tk_CloseBrace);
    if (bal == 0 && tk[i].type == Tk_Equal) {
      has_equal = i;
    }
  }
  // assign
  if (has_equal != -1) {
    tokenizer left, right;
    for (int i = 0; i < has_equal; ++i) {
      left.push_back(tk[i]);
    }
    for (int i = has_equal + 1; i < int(tk.size()); ++i) {
      right.push_back(tk[i]);
    }
    Assign *t = new Assign();
    t->To = parse(left);
    t->Value = parse(right);
    return t;
  }
  // functioncall: f(...
  if (tk.size() >= 2 && tk[0].type == Tk_Identifier && tk[1].type == Tk_OpenParen) {
    FunctionCall *t = new FunctionCall();
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
        t->Params.push_back(parse(param));
        param.clear();
      }
    }
    param.pop_back();
    t->Params.push_back(parse(param));
    return t;
  }
  // identifier
  if (tk.size() == 1 && tk[0].type == Tk_Identifier) {
    Identifier *t = new Identifier();
    t->Name = tk[0].token;
    return t;
  }
  // return
  if (tk.size() >= 1 && tk[0].type == Tk_Return) {
    Return *t = new Return();
    tk.get();
    t->Value = parse(tk);
    return t;
  }
  throw std::runtime_error("Unknown node type");
}

} // namespace av