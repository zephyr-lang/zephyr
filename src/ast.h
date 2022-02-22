#pragma once
#include "token.h"
#include <stdbool.h>

#define ENUM_AST(F) \
	F(OP_ADD, "add") \
	F(OP_SUB, "sub") \
	F(OP_MUL, "mul") \
	F(OP_DIV, "div") \
	F(OP_MOD, "mod") \
	F(OP_BWAND, "bw and") \
	F(OP_BWOR, "bw or") \
	F(OP_XOR, "xor") \
	F(OP_LSH, "lsh") \
	F(OP_RSH, "rsh") \
	F(OP_EQUAL, "equal") \
	F(OP_NOT_EQUAL, "not equal") \
	F(OP_LESS, "less") \
	F(OP_LESS_EQ, "less-equal") \
	F(OP_GREATER, "greater") \
	F(OP_GREATER_EQ, "greater-equal") \
	F(OP_BWNOT, "bw not") \
	F(OP_NEG, "neg") \
	F(OP_NOT, "not") \
	F(AST_INT_LITERAL, "int literal") \
	F(AST_CALL, "call") \
	F(AST_DEFINE_VAR, "define var") \
	F(AST_ACCESS_VAR, "access var") \
	F(AST_ASSIGN_VAR, "assign var") \
	F(AST_EXPR_STMT, "expression statement") \
	F(AST_RETURN, "return") \
	F(AST_FUNCTION, "function") \
	F(AST_BLOCK, "block") \
	F(AST_PROGRAM, "program")

typedef enum NodeType {
#define E(name, str) name,
	ENUM_AST(E)
#undef E
} NodeType;

typedef enum DataType {
	DATA_TYPE_VOID,
	DATA_TYPE_INT
} DataType;

typedef struct Type {
	DataType type;
} Type;

typedef struct Node Node;
typedef struct Node {
	NodeType type;

	union {
		Node* unary;

		struct {
			Node* lhs;
			Node* rhs;
		} binary;

		struct {
			Token name;
			Type returnType;
			Node* body;

			int variableCount;
			Node** variables;
			int currentStackOffset;
		} function;

		struct {
			Node** children;
			size_t size;
		} block;

		struct {
			Type type;

			union {
				int integer;
			} as;
		} literal;

		struct {
			Token name;
			Type type;
			Node* value;

			int stackOffset;
		} variable;

	};
} Node;

void print_ast(Node* ast);
char* data_type_to_string(DataType type);
char* node_type_to_string(NodeType type);
char* type_to_string(Type type);
bool is_unary_op(NodeType type);
bool is_binary_op(NodeType type);