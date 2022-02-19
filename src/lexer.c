#include "lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

Lexer new_lexer(const char* filename, char* source) {
	Lexer lexer = {0};
	lexer.start = source;
	lexer.current = source;
	lexer.line = 1;
	lexer.filename = filename;

	return lexer;
}

static bool is_at_end(Lexer* lexer) {
	return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
	lexer->current++;
	return lexer->current[-1];
}

static char peek(Lexer* lexer) {
	return *lexer->current;
}

// Will return the character *after* the current one without consuming anything.
// E.g., given the string "abc", with the start pointer at the start of the string,
// peek() will return the "a" and peek_next() will return the "b"
static char peek_next(Lexer* lexer) {
	if (is_at_end(lexer)) return '\0';
	return lexer->current[1];
}

static Token make_token(Lexer* lexer, TokenType type) {
	Token token;
	token.type = type;
	token.start = lexer->start;
	token.length = lexer->current - lexer->start;
	token.line = lexer->line;
	return token;
}

static Token error_token(Lexer* lexer, const char* message) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = strlen(message);
	token.line = lexer->line;
	return token;
}

static bool is_digit(char c) {
	return '0' <= c && c <= '9';
}

static bool is_alpha(char c) {
	return ('a' <= c && c <= 'z') ||
		('A' <= c && c <= 'Z') ||
		c == '_';
}

static TokenType check_keyword(Lexer* lexer, size_t start, size_t length, const char* rest, TokenType type) {
	if(lexer->current - lexer->start == start + length && memcmp(lexer->start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer* lexer) {
	switch(lexer->start[0]) {
		case 'f': return check_keyword(lexer, 1, 7, "unction", TOKEN_FUNCTION);
		case 'r': return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
		default: break;
	}
	return TOKEN_IDENTIFIER;
}

static void skip_whitespace(Lexer* lexer) {
	for(;;) {
		char c = peek(lexer);
		switch(c) {
			case ' ':
			case '\r':
			case '\t': {
				advance(lexer);
				break;
			}
			case '\n': {
				lexer->line++;
				advance(lexer);
				break;
			}
			case '/': {
				if(peek_next(lexer) == '/') {
					while(peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
				}
				else return;
				break;
			}
			default: return;
		}
	}
}

static Token number(Lexer* lexer) {
	while(is_digit(peek(lexer))) advance(lexer);
	return make_token(lexer, TOKEN_INT_LITERAL);
}

static Token identifier(Lexer* lexer) {
	while(is_alpha(peek(lexer)) || is_digit(peek(lexer))) advance(lexer);
	return make_token(lexer, identifier_type(lexer));
}

Token lexer_next(Lexer* lexer) {
	skip_whitespace(lexer);
	lexer->start = lexer->current;

	if(is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

	char c = advance(lexer);

	if(is_alpha(c)) return identifier(lexer);
	if(is_digit(c)) return number(lexer);

	switch (c) {
		case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
		case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
		case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
		case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
		case ':': return make_token(lexer, TOKEN_COLON);
		case ';': return make_token(lexer, TOKEN_SEMICOLON);
		default: break;
	}

	return error_token(lexer, "Unexpected character");
}