#pragma once

#include "ast.hpp"
#include <ostream>

namespace av {

int memory_needed(const Type &type);

// this dfs just calculates the maximum memory we'd need by essentially finding the depth of the ast
int maximum_memory(Node *t);

void generate(Node *t, std::ostream &os);

} // namespace av