#pragma once
#include <stdint.h>
#include <stddef.h>

#define ENUM_TOKENS(F)                  \
	F(TOKEN_LEFT_PAREN, "left-paren")   \
	F(TOKEN_RIGHT_PAREN, "right-paren") \
	F(TOKEN_LEFT_BRACE, "left-brace")   \
	F(TOKEN_RIGHT_BRACE, "right-brace") \
	F(TOKEN_COLON, "colon")             \
	F(TOKEN_SEMICOLON, "semicolon")     \
                                        \
	F(TOKEN_INT_LITERAL, "int-literal") \
	F(TOKEN_IDENTIFIER, "identifier")   \
                                        \
	F(TOKEN_INT, "int")                 \
	F(TOKEN_FUNCTION, "function")       \
	F(TOKEN_RETURN, "return")           \
	F(TOKEN_VOID, "void")               \
                                        \
	F(TOKEN_ERROR, "error")             \
	F(TOKEN_EOF, "eof")

typedef enum TokenType {
#define E(name, str) name,
	ENUM_TOKENS(E)
#undef E
} TokenType;

typedef struct Token {
	TokenType type;
	const char *start;
	size_t length;
	size_t line;
} Token;

char* token_type_to_string(TokenType type);
void print_token(Token* token);