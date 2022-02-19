#pragma once

#define ENUM_AST(F) \
	F(AST_INT_LITERAL, "int literal") \
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
			char* name;
			Type returnType;
			Node* body;
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

	};
} Node;

void print_ast(Node* ast);
char* data_type_to_string(DataType type);