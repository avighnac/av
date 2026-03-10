#pragma once

#include "tokenizer.hpp"
#include "ast.hpp"

namespace av {

Node *parse(tokenizer &tk);

}