#pragma once
#include "ast.h"
#include <stdio.h>
Node** parser_builtin_functions(int* funcCount);
void write_builtin_functions(FILE* out);