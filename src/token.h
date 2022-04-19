#pragma once
#include <stdint.h>
#include <stddef.h>

#define ENUM_TOKENS(F)                    \
	F(TOKEN_LEFT_PAREN, "left-paren")     \
	F(TOKEN_RIGHT_PAREN, "right-paren")   \
	F(TOKEN_LEFT_BRACE, "left-brace")     \
	F(TOKEN_RIGHT_BRACE, "right-brace")   \
	F(TOKEN_LEFT_SQBR, "left-sqbrkt")     \
	F(TOKEN_RIGHT_SQBR, "right-sqbrkt")   \
	F(TOKEN_COLON, "colon")               \
	F(TOKEN_SEMICOLON, "semicolon")       \
	F(TOKEN_COMMA, "comma")               \
	F(TOKEN_DOT, "dot")                   \
	F(TOKEN_QUESTION, "question")         \
	F(TOKEN_TILDE, "tilde")               \
	F(TOKEN_MINUS, "minus")               \
	F(TOKEN_BANG, "bang")                 \
	F(TOKEN_PLUS, "plus")                 \
	F(TOKEN_STAR, "star")                 \
	F(TOKEN_SLASH, "slash")               \
	F(TOKEN_PERCENT, "percent")           \
	F(TOKEN_AMP, "amp")                   \
	F(TOKEN_AMP_AMP, "amp-amp")           \
	F(TOKEN_BAR, "bar")                   \
	F(TOKEN_BAR_BAR, "bar-bar")           \
	F(TOKEN_XOR, "xor")                   \
	F(TOKEN_LESS, "less")                 \
	F(TOKEN_LSH, "lsh")                   \
	F(TOKEN_GREATER, "greater")           \
	F(TOKEN_RSH, "rsh")                   \
	F(TOKEN_EQ, "eq")                     \
	F(TOKEN_EQEQ, "eq-eq")                \
	F(TOKEN_BANG_EQ, "bang-eq")           \
	F(TOKEN_LEQ, "less-eq")               \
	F(TOKEN_GEQ, "greater-eq")            \
                                          \
	F(TOKEN_INT_LITERAL, "int-literal")   \
	F(TOKEN_CHAR_LITERAL, "char-literal") \
	F(TOKEN_STRING, "string")             \
	F(TOKEN_IDENTIFIER, "identifier")     \
                                          \
	F(TOKEN_AS, "as")                     \
	F(TOKEN_ELSE, "else")                 \
	F(TOKEN_IF, "if")                     \
	F(TOKEN_INT, "int")                   \
	F(TOKEN_I8, "i8")                     \
	F(TOKEN_I16, "i16")                   \
	F(TOKEN_I32, "i32")                   \
	F(TOKEN_I64, "i64")                   \
	F(TOKEN_FOR, "for")                   \
	F(TOKEN_FUNCTION, "function")         \
	F(TOKEN_RETURN, "return")             \
	F(TOKEN_SIZEOF, "sizeof")             \
	F(TOKEN_STRUCT, "struct")             \
	F(TOKEN_VOID, "void")                 \
	F(TOKEN_VAR, "var")                   \
	F(TOKEN_WHILE, "while")               \
                                          \
	F(TOKEN_ERROR, "error")               \
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