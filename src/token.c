#include "token.h"
#include <stdio.h>
#include <string.h>

char* token_type_to_string(TokenType type) {
	switch(type) {
#define E(name, str) case name: return str;
	ENUM_TOKENS(E)
#undef E
	}
	return "<unknown type>";
}

void print_token(Token* token) {
	printf("%4zu %s '%.*s'", token->line, token_type_to_string(token->type), (int)token->length, token->start);
}

bool tokens_equal(Token a, Token b) {
	return a.length == b.length && memcmp(a.start, b.start, a.length) == 0;
}