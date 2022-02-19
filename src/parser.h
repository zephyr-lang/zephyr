#pragma once
#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

typedef struct Parser {
	Lexer* lexer;
	Token current;
	Token previous;

	bool error;
	bool panic;
} Parser;

Parser new_parser(Lexer* lexer);
Node* parse_program(Parser* parser);