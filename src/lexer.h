#pragma once
#include "token.h"

typedef struct Lexer {
	char* start;
	char* current;

	size_t line;

	const char* filename;
} Lexer;

Lexer new_lexer(const char* filename, char* source);
Token lexer_next(Lexer* lexer);