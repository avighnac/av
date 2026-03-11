#include "gen.hpp"
#include "ast.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace av {

int memory_needed(const Type &type) {
  static std::array<int, __count_Type> a({1, 2, 4, 8, 8, 8, 8, 8, 8, 0});
  return a[int(type)];
}

std::string asm_keyword_for(const Type &type) {
  int sz = memory_needed(type);
  if (sz == 4) {
    return "dword";
  }
  if (sz == 8) {
    return "qword";
  }
  if (sz == 2) {
    return "word";
  }
  if (sz == 1) {
    return "byte";
  }
  throw std::runtime_error("No asm keyword found for size " + std::to_string(sz));
}

bool is_literal(const NodeType &type) {
  std::array a{
      int8Literal, int16Literal, int32Literal, int64Literal};
  return std::find(a.begin(), a.end(), type) != a.end();
}

// clang-format off
std::array<std::string, 7> registers = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax"
};

std::array<std::array<std::string, 4>, 7> register_variants = {{
  {"rdi", "edi", "di", "dil"},
  {"rsi", "esi", "si", "sil"},
  {"rdx", "edx", "dx", "dl"},
  {"rcx", "ecx", "cx", "cl"},
  {"r8", "r8d", "r8w", "r8b"},
  {"r9", "r9d", "r9w", "r9b"},
  {"rax", "eax", "ax", "al"}
}};
// clang-format on

std::string get_register(const std::string &s, const Type &type) {
  auto it = std::find(registers.begin(), registers.end(), s);
  assert(it != registers.end());
  unsigned sz = memory_needed(type);
  int w = std::bit_width(sz) - 1;
  return register_variants[it - registers.begin()][3 - w];
}
std::string rax_for(const Type &type) { return get_register("rax", type); }

int maximum_memory(Node *t) {
  int ans = 0;
  switch (t->type) {
  case block: {
    int sum_non_block = 0, max_block = 0;
    for (Node *&i : ((Block *)t)->stmts) {
      if (i->type == block) {
        max_block = std::max(max_block, maximum_memory(i));
      } else {
        sum_non_block += maximum_memory(i);
      }
    }
    ans = sum_non_block + max_block;
  } break;
  case variableDecl: {
    ans = memory_needed(((VariableDecl *)t)->VariableType);
  } break;
  default: {
  } break;
  }
  return ans;
}

// clang-format off
std::array compiler_intrinsics{
  "write", "read"
};
// clang-format on
std::array intrinsic_code{
    R"(write:    
  mov eax, 1
  syscall
  ret
)",
    R"(read:    
  xor eax, eax
  syscall
  ret
)",

};

bool is_intrinsic(const std::string &s) {
  return std::find(compiler_intrinsics.begin(), compiler_intrinsics.end(), s) != compiler_intrinsics.end();
}

void generate(Node *t, std::ostream &os) {
  std::map<std::string, std::pair<FunctionDecl *, FunctionBody *>> funcs;
  std::map<std::string, VariableDecl *> vars;
  std::map<std::string, std::string> func_label;
  std::map<std::string, int> var_offset;

  // compiler intrinsics
  for (int i = 0; i < compiler_intrinsics.size(); ++i) {
    func_label[compiler_intrinsics[i]] = compiler_intrinsics[i];
  }

  // make yourself clean up when you go out of scope
  // so that's end of Block only
  std::string prefix = "__av__", cur_func;
  std::vector<int> prefix_sizes;
  std::vector<std::string> used_intrinsics;
  auto push_prefix = [&](const std::string &s) {
    prefix_sizes.push_back(int(s.length()));
    prefix += s;
  };
  auto pop_prefix = [&]() {
    for (int i = 0; i < prefix_sizes.back(); ++i) {
      prefix.pop_back();
    }
    prefix_sizes.pop_back();
  };
  int global = 1, memory = 0;
  auto gen = [&](auto &&self, Node *t) -> void {
    auto gen_cmp = [&](const std::string &asm_comp) {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rbx\n";
      os << "  cmp rax, rbx\n";
      os << "  " << asm_comp << " al\n";
      os << "  movzx eax, al\n";
    };
    auto gen_shift = [&](const std::string &asm_shift) {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rcx\n";
      os << "  " << asm_shift << " rax, cl\n";
    };
    auto handle_mov = [&](const std::string &s, const Type &type) {
      int idx = std::find(registers.begin(), registers.end(), s) - registers.begin();
      int sz = memory_needed(type);
      if (sz == 8) {
        os << "  mov " << register_variants[idx][0];
        return;
      }
      if (sz == 4) {
        os << "  mov " << register_variants[idx][1];
        return;
      }
      if (sz == 2 || sz == 1) {
        os << "  movzx " << register_variants[idx][1];
        return;
      }
      assert(false);
    };
    switch (t->type) {
    case block: {
      auto Funcs = funcs;
      auto Vars = vars;
      auto Func_label = func_label;
      auto Var_offset = var_offset;
      auto Memory = memory;
      for (Node *&i : ((Block *)t)->stmts) {
        self(self, i);
      }
      funcs = Funcs;
      vars = Vars;
      func_label = Func_label;
      var_offset = Var_offset;
      memory = Memory;
    } break;
    case functionDecl: {
      FunctionDecl *u = (FunctionDecl *)t;
      funcs[u->Name].first = u;
    } break;
    case functionBody: {
      FunctionBody *u = (FunctionBody *)t;
      if (!funcs.contains(u->Name)) {
        throw std::runtime_error("Missing declaration for function " + u->Name + " before body");
      }
      funcs[u->Name].second = u;
      push_prefix(u->Name + "__");
      std::string prev_func = std::exchange(cur_func, u->Name);
      int was = std::exchange(global, 0);
      if (was) {
        func_label[u->Name] = u->Name;
      } else {
        func_label[u->Name] = prefix;
      }
      os << func_label[u->Name] << ":\n";
      int b = maximum_memory(u->Body);
      FunctionDecl *v = funcs[u->Name].first;
      // size needed for function arguments
      for (int i = 0; i < int(u->Params.size()); ++i) {
        b += memory_needed(v->ParamTypes[i]);
      }
      b = ((b + 16) / 16) * 16; // memory alignment
      os << "  push rbp\n";
      os << "  mov rbp, rsp\n";
      // first we need to restore arguments
      int cur_size = 0;
      for (int i = 0; i < std::min(int(u->Params.size()), 6); ++i) {
        cur_size += memory_needed(v->ParamTypes[i]);
        os << "  mov " << asm_keyword_for(v->ParamTypes[i])
           << "[rbp-" << cur_size << "], " << get_register(registers[i], v->ParamTypes[i]) << '\n';
      }
      // stack based
      for (int i = 6; i < int(u->Params.size()); ++i) {
        cur_size += memory_needed(v->ParamTypes[i]);
        // again works because alignment
        handle_mov("rax", v->ParamTypes[i]);
        os << ", [rsp+" << 8 * (i - 5) << "]\n";
        os << "  mov " << asm_keyword_for(v->ParamTypes[i])
           << "[rbp-" << cur_size << "], rax\n";
      }
      os << "  sub rsp, " << b << "\n";
      self(self, u->Body);
      os << func_label[u->Name] << "__end:\n";
      os << "  mov rsp, rbp\n";
      os << "  pop rbp\n";
      os << "  ret\n";
      pop_prefix();
      cur_func = prev_func;
    } break;
    case variableDecl: {
      VariableDecl *u = (VariableDecl *)t;
      memory += memory_needed(u->VariableType);
      if (vars.contains(u->Name)) {
        throw std::runtime_error("Redefinition of symbol " + u->Name);
      }
      vars[u->Name] = u;
      var_offset[u->Name] = memory;
    } break;
    case assign: {
      // let's just make our lives easy and assume lhs is *x or x
      Assign *u = (Assign *)t;
      if (u->To->type != dereference && u->To->type != identifier) {
        throw std::runtime_error("Assigning to anything other than *x or x is invalid");
      }
      if (u->To->type == identifier) {
        Identifier *ident = (Identifier *)u->To;
        if (!vars.contains(ident->Name)) {
          throw std::runtime_error(ident->Name + ": unknown symbol");
        }
        // assume the codegen for rhs puts the result in rax
        self(self, u->Value);
        Type type = vars[ident->Name]->VariableType;
        os << "  mov " << asm_keyword_for(type)
           << "[rbp-" << var_offset[ident->Name] << "], " << rax_for(type) << '\n';
      }
    } break;
    case int8Literal: {
      os << "  mov eax, " << int(((Int8Literal *)t)->Value) << '\n';
    } break;
    case int16Literal: {
      os << "  mov eax, " << ((Int16Literal *)t)->Value << '\n';
    } break;
    case int32Literal: {
      os << "  mov eax, " << ((Int32Literal *)t)->Value << '\n';
    } break;
    case int64Literal: {
      os << "  mov rax, " << ((Int64Literal *)t)->Value << '\n';
    } break;
    case addressOf: {
      AddressOf *u = (AddressOf *)t;
      if (!vars.contains(u->Name)) {
        throw std::runtime_error("Cannot get the address of unknown symbol " + u->Name);
      }
      os << "  lea rax, [rbp-" << var_offset[u->Name] << "]\n";
    } break;
    case dereference: {
      Dereference *u = (Dereference *)t;
      if (!vars.contains(u->Name)) {
        throw std::runtime_error("Cannot dereference unknown symbol " + u->Name);
      }
      Type type = vars[u->Name]->VariableType;
      handle_mov("rax", type);
      os << ", " << asm_keyword_for(type) << "[rbp-" << var_offset[u->Name] << "]\n";
    };
    case functionCall: {
      FunctionCall *u = (FunctionCall *)t;
      if (!funcs.contains(u->Name)) {
        throw std::runtime_error("Cannot call function " + u->Name + " since it hasn't been defined");
      }
      for (int i = 0; i < int(u->Params.size()); ++i) {
        self(self, u->Params[i]);
        // for now, just assume we're either a literal or variable
        if (!is_literal(u->Params[i]->type) && u->Params[i]->type != identifier) {
          throw std::runtime_error("Function argument in function call " + u->Name + " has to be either a variable or a literal");
        }
        Type type;
        switch (u->Params[i]->type) {
        case int8Literal: {
          type = Int8;
        } break;
        case int16Literal: {
          type = Int16;
        } break;
        case int32Literal: {
          type = Int32;
        } break;
        case int64Literal: {
          type = Int64;
        } break;
        case identifier: {
          if (!vars.contains(((Identifier *)u->Params[i])->Name)) {
            throw std::runtime_error("Unknown symbol: " + ((Identifier *)u->Params[i])->Name);
          }
          type = vars[((Identifier *)u->Params[i])->Name]->VariableType;
        } break;
        }
        if (i < 6) {
          handle_mov(registers[i], type);
          os << ", " << rax_for(type) << '\n';
        } else {
          if (i == 6) {
            os << "  sub rsp, " << 8 * (u->Params.size() - 5) << '\n';
          }
          os << "  mov " << asm_keyword_for(type) << "[rsp+" << 8 * (i - 6) << "], " << rax_for(type) << '\n';
        }
      }
      // to 16-bit align, we must do this
      if (u->Params.size() > 6 && u->Params.size() % 2 != 0) {
        os << "  sub rsp, 8\n";
      }
      os << "  call " << func_label[u->Name] << '\n';
      if (is_intrinsic(u->Name)) {
        used_intrinsics.push_back(u->Name);
      }
      if (u->Params.size() > 6 && u->Params.size() % 2 != 0) {
        os << "  add rsp, 8\n";
      }
    } break;
    case identifier: {
      Identifier *u = (Identifier *)t;
      if (!vars.contains(u->Name)) {
        throw std::runtime_error("Cannot find symbol: " + u->Name);
      }
      Type type = vars[u->Name]->VariableType;
      handle_mov("rax", type);
      os << ", " << asm_keyword_for(type) << "[rbp-" << var_offset[u->Name] << "]\n";
    } break;
    case returnNode: {
      Return *u = (Return *)t;
      self(self, u->Value);
      os << "  jmp " << func_label[cur_func] << "__end\n";
    } break;
    case equal: {
      gen_cmp("sete");
    } break;
    case less: {
      gen_cmp("setl");
    } break;
    case greater: {
      gen_cmp("setg");
    } break;
    case lessEqual: {
      gen_cmp("setle");
    } break;
    case greaterEqual: {
      gen_cmp("setge");
    } break;
    case shiftLeft: {
      gen_shift("sal");
    } break;
    case shiftRight: {
      gen_shift("sar");
    } break;
    case plus: {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rbx\n";
      os << "  add rax, rbx\n";
    } break;
    case minus: {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rbx\n";
      os << "  sub rax, rbx\n";
    } break;
    case multiply: {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rbx\n";
      os << "  mul rbx\n";
    } break;
    case div: {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rbx\n";
      os << "  xor edx, edx\n";
      os << "  div rbx\n";
    } break;
    case modulo: {
      Binary *u = (Binary *)t;
      self(self, u->Rhs);
      os << "  push rax\n";
      self(self, u->Lhs);
      os << "  pop rbx\n";
      os << "  xor edx, edx\n";
      os << "  div rbx\n";
      os << "  mov rax, rdx\n";
    } break;
    default: {
    } break;
    }
  };
  gen(gen, t);

  // now write intrinsics
  std::sort(used_intrinsics.begin(), used_intrinsics.end());
  used_intrinsics.erase(std::unique(used_intrinsics.begin(), used_intrinsics.end()), used_intrinsics.end());
  for (std::string &i : used_intrinsics) {
    os << intrinsic_code[std::find(compiler_intrinsics.begin(), compiler_intrinsics.end(), i) - compiler_intrinsics.begin()];
  }
}

} // namespace av