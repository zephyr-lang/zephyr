#pragma once
#include "ast.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

void generate_program(Parser* parser, Node* ast, FILE* out);