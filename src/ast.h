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
	F(OP_AND, "&&") \
	F(OP_OR, "||") \
	F(OP_BWNOT, "~") \
	F(OP_NEG, "-") \
	F(OP_NOT, "!") \
	F(OP_TERNARY, "?") \
	F(OP_DEREF, "*") \
	F(OP_ADDROF, "&") \
	F(OP_SIZEOF, "sizeof") \
	F(OP_ACCESS_SUBSCRIPT, "access subscript") \
	F(OP_ASSIGN_SUBSCRIPT, "assign subscript") \
	F(OP_ASSIGN_DEREF, "assign deref") \
	F(OP_ACCESS_MEMBER, "access member") \
	F(OP_ASSIGN_MEMBER, "assign member") \
	F(OP_ACCESS_MEMBER_PTR, "access member ptr") \
	F(OP_ASSIGN_MEMBER_PTR, "assign member ptr") \
	F(AST_ARRAY_INIT, "array initialization") \
	F(AST_INT_LITERAL, "int literal") \
	F(AST_CHAR_LITERAL, "char literal") \
	F(AST_STRING, "string") \
	F(AST_CAST, "cast") \
	F(AST_CALL, "call") \
	F(AST_CALL_METHOD, "call method") \
	F(AST_DEFINE_VAR, "define var") \
	F(AST_ACCESS_VAR, "access var") \
	F(AST_ASSIGN_VAR, "assign var") \
	F(AST_DEFINE_GLOBAL_VAR, "define gvar") \
	F(AST_ACCESS_GLOBAL_VAR, "access gvar") \
	F(AST_ASSIGN_GLOBAL_VAR, "assign gvar") \
	F(AST_DEFINE_CONST, "define const") \
	F(OP_COPY_STRUCT, "copy struct") \
	F(OP_COPY_STRUCT_MEMBER, "copy struct member") \
	F(OP_COPY_STRUCT_DEREF, "copy struct deref") \
	F(AST_UNION, "union") \
	F(AST_STRUCT, "struct") \
	F(AST_MEMBER, "member") \
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
	DATA_TYPE_I64,

	DATA_TYPE_ANY,

	DATA_TYPE_STRUCT,
	DATA_TYPE_UNION,

	DATA_TYPE_UNRESOLVED
} DataType;

typedef struct Node Node;

typedef struct Type {
	DataType type;
	int indirection; // 0 = int, 1 = int*, 2 = int**, etc.
	bool isArray;
	int arrayLength;

	Token name;
	Node** fields;
	int fieldCount;

	Node** methods;
	int methodCount;
} Type;

typedef enum LValueType {
	LVALUE_NONE = 0,
	LVALUE_LOCAL,
	LVALUE_GLOBAL,
	LVALUE_SUBSCRIPT,
	LVALUE_DEREF,
	LVALUE_MEMBER
} LValueType;

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
			Node* lhs;
			Node* mid;
			Node* rhs;
		} ternary;

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
			
			bool isMethod;
			Node* parent;
			Type parentType;

			Node** arguments;
			int argumentCount;
			int localVariableStackOffset;
		} function;

		struct {
			Node** children;
			size_t size;
			size_t capacity;

			bool hasReturned;

			Node* parent;
			int variableCount;
			Node** variables;
			int currentStackOffset;
		} block;

		struct {
			Type type;

			union {
				int64_t integer;
				struct {
					const char* chars;
					size_t length;
					int id;
				} string;
			} as;
		} literal;

		struct {
			Token name;
			Type type;
			Node* value;

			int stackOffset;
		} variable;

		struct {
			Token name;
			int64_t value;
		} constant;

		struct {
			Token name;
			Node* parent;
			Node* value;
			Node* memberRef;
		} member;

	};
} Node;

void print_ast(Node* ast);
char* data_type_to_string(DataType type);
char* node_type_to_string(NodeType type);
char* type_to_string(Type type);
bool is_unary_op(NodeType type);
bool is_binary_op(NodeType type);
Node* new_node(NodeType type, Token position);
void node_add_child(Node* parent, Node* child);