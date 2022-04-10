#pragma once
#include "token.h"
#include <stdbool.h>

#define ENUM_AST(F) \
	F(OP_ADD, "+") \
	F(OP_SUB, "-") \
	F(OP_MUL, "*") \
	F(OP_DIV, "/") \
	F(OP_MOD, "%") \
	F(OP_BWAND, "&") \
	F(OP_BWOR, "|") \
	F(OP_XOR, "^") \
	F(OP_LSH, "<<") \
	F(OP_RSH, ">>") \
	F(OP_EQUAL, "==") \
	F(OP_NOT_EQUAL, "!=") \
	F(OP_LESS, "<") \
	F(OP_LESS_EQ, "<=") \
	F(OP_GREATER, ">") \
	F(OP_GREATER_EQ, ">=") \
	F(OP_BWNOT, "~") \
	F(OP_NEG, "-") \
	F(OP_NOT, "!") \
	F(OP_TERNARY, "?") \
	F(OP_DEREF, "*") \
	F(OP_ADDROF, "&") \
	F(AST_INT_LITERAL, "int literal") \
	F(AST_CALL, "call") \
	F(AST_DEFINE_VAR, "define var") \
	F(AST_ACCESS_VAR, "access var") \
	F(AST_ASSIGN_VAR, "assign var") \
	F(AST_DEFINE_GLOBAL_VAR, "define gvar") \
	F(AST_EXPR_STMT, "expression statement") \
	F(AST_IF, "if") \
	F(AST_FOR, "for") \
	F(AST_WHILE, "while") \
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
	DATA_TYPE_INT,
	DATA_TYPE_I8,
	DATA_TYPE_I16,
	DATA_TYPE_I32,
	DATA_TYPE_I64
} DataType;

typedef struct Type {
	DataType type;
	int indirection; // 0 = int, 1 = int*, 2 = int**, etc.
} Type;

typedef enum LValueType {
	LVALUE_NONE = 0,
	LVALUE_IDENTIFIER
} LValueType;

typedef struct Node Node;
typedef struct Node {
	NodeType type;
	Token position;
	LValueType lvalue;
	Type computedType;

	union {
		Node* unary;

		struct {
			Node* lhs;
			Node* rhs;
		} binary;

		struct {
			Node* condition;
			Node* doTrue;
			Node* doFalse;
		} conditional;

		struct {
			Node* initial;
			Node* condition;
			Node* iteration;
			Node* body;
		} loop;

		struct {
			Token name;
			Type returnType;
			Node* body;
			bool hasImplicitBody;

			Node** arguments;
			int argumentCount;
			int localVariableStackOffset;
		} function;

		struct {
			Node** children;
			size_t size;
			size_t capacity;

			Node* parent;
			int variableCount;
			Node** variables;
			int currentStackOffset;
		} block;

		struct {
			Type type;

			union {
				int64_t integer;
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