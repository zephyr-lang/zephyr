#include "lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

Lexer* new_lexer(const char* filename, char* source) {
	Lexer* lexer = malloc(sizeof(Lexer));
	lexer->start = source;
	lexer->current = source;
	lexer->line = 1;
	lexer->filename = filename;

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

static bool match(Lexer* lexer, char expected) {
	if (is_at_end(lexer)) return false;
	if (*lexer->current != expected) return false;
	lexer->current++;
	return true;
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
		case 'a': return check_keyword(lexer, 1, 1, "s", TOKEN_AS);
		case 'c': return check_keyword(lexer, 1, 4, "onst", TOKEN_CONST);
		case 'e': return check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
		case 'i': {
			if(lexer->current - lexer->start > 1) {
				switch(lexer->start[1]) {
					case 'f': return check_keyword(lexer, 2, 0, "", TOKEN_IF);
					case 'n': return check_keyword(lexer, 2, 1, "t", TOKEN_INT);
					case 'm': return check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
					case '8': return check_keyword(lexer, 2, 0, "", TOKEN_I8);
					case '1': return check_keyword(lexer, 2, 1, "6", TOKEN_I16);
					case '3': return check_keyword(lexer, 2, 1, "2", TOKEN_I32);
					case '6': return check_keyword(lexer, 2, 1, "4", TOKEN_I64);
				}
			}
			break;
		}
		case 'f': {
			if(lexer->current - lexer->start > 1) {
				switch(lexer->start[1]) {
					case 'u': return check_keyword(lexer, 2, 6, "nction", TOKEN_FUNCTION);
					case 'o': return check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
				}
			}
			break;
		}
		case 'r': return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
		case 's': {
			if(lexer->current - lexer->start > 1) {
				switch(lexer->start[1]) {
					case 'i': return check_keyword(lexer, 2, 4, "zeof", TOKEN_SIZEOF);
					case 't': return check_keyword(lexer, 2, 4, "ruct", TOKEN_STRUCT);
				}
			}
			break;
		}
		case 'u': return check_keyword(lexer, 1, 4, "nion", TOKEN_UNION);
		case 'v': {
			if(lexer->current - lexer->start > 1) {
				switch(lexer->start[1]) {
					case 'a': return check_keyword(lexer, 2, 1, "r", TOKEN_VAR);
					case 'o': return check_keyword(lexer, 2, 2, "id", TOKEN_VOID);
				}
			}
			break;
		}
		case 'w': return check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
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

TokenType less_than(Lexer* lexer) {
	TokenType type = TOKEN_LESS;
	
	if(match(lexer, '=')) {
		type = TOKEN_LEQ;
	}
	else if(match(lexer, '<')) {
		type = TOKEN_LSH;
	}
	return type;
}

TokenType greater_than(Lexer* lexer) {
	TokenType type = TOKEN_GREATER;
	
	if(match(lexer, '=')) {
		type = TOKEN_GEQ;
	}
	else if(match(lexer, '>')) {
		type = TOKEN_RSH;
	}
	return type;
}

Token character(Lexer* lexer) {
	if(is_at_end(lexer)) return error_token(lexer, "Expected character literal");
	advance(lexer);
	if(!match(lexer, '\'')) return error_token(lexer, "Expected closing single quote");
	return make_token(lexer, TOKEN_CHAR_LITERAL);
}

Token string(Lexer* lexer) {
	while(!is_at_end(lexer) && peek(lexer) != '"') {
		advance(lexer);
	}

	if(!match(lexer, '"')) {
		return error_token(lexer, "Expected closing double quote");
	}

	return make_token(lexer, TOKEN_STRING);
}

Token lexer_next(Lexer* lexer) {
	skip_whitespace(lexer);
	lexer->start = lexer->current;

	if(is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

	char c = advance(lexer);

	if(is_alpha(c)) return identifier(lexer);
	if(is_digit(c)) return number(lexer);

	if(c == '\'') return character(lexer);
	if(c == '"') return string(lexer);

	switch (c) {
		case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
		case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
		case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
		case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
		case '[': return make_token(lexer, TOKEN_LEFT_SQBR);
		case ']': return make_token(lexer, TOKEN_RIGHT_SQBR);
		case ':': return make_token(lexer, TOKEN_COLON);
		case ';': return make_token(lexer, TOKEN_SEMICOLON);
		case ',': return make_token(lexer, TOKEN_COMMA);
		case '.': return make_token(lexer, TOKEN_DOT);
		case '?': return make_token(lexer, TOKEN_QUESTION);
		case '~': return make_token(lexer, TOKEN_TILDE);
		case '-': return make_token(lexer, TOKEN_MINUS);
		case '!': return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQ : TOKEN_BANG);
		case '+': return make_token(lexer, TOKEN_PLUS);
		case '*': return make_token(lexer, TOKEN_STAR);
		case '/': return make_token(lexer, TOKEN_SLASH);
		case '%': return make_token(lexer, TOKEN_PERCENT);
		case '&': return make_token(lexer, match(lexer, '&') ? TOKEN_AMP_AMP : TOKEN_AMP);
		case '|': return make_token(lexer, match(lexer, '|') ? TOKEN_BAR_BAR : TOKEN_BAR);
		case '^': return make_token(lexer, TOKEN_XOR);
		case '<': return make_token(lexer, less_than(lexer));
		case '>': return make_token(lexer, greater_than(lexer));
		case '=': return make_token(lexer, match(lexer, '=') ? TOKEN_EQEQ : TOKEN_EQ);
		default: break;
	}

	return error_token(lexer, "Unexpected character");
}