#pragma once
#include "ast.h"
#include "parser.h"

void type_check(Parser* parser, Node* program);
int sizeof_type(Type* type);
int sizeof_type_var_offset(Type* type);
bool is_structural_type(Type* a);