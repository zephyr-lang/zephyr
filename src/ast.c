#include "ast.h"
#include <stdio.h>

char* data_type_to_string(DataType type) {
	switch(type) {
		case DATA_TYPE_VOID: return "void";
		case DATA_TYPE_INT: return "int";
	}
	return "<unknown type>";
}

char* node_type_to_string(NodeType type) {
	switch(type) {
#define E(name, str) case name: return str;
	ENUM_AST(E)
#undef E
	}
	return "<unknown node type>";
}

void print_ast_depth(Node* node, int depth) {
	for(int i = 0; i < depth; i++) printf("  ");

	switch(node->type) {
		case AST_PROGRAM: {
			for(size_t i = 0; i < node->block.size; i++)
				print_ast_depth(node->block.children[i], depth);
			break;
		}
		case AST_BLOCK: {
			printf("{\n");
			for(size_t i = 0; i < node->block.size; i++)
				print_ast_depth(node->block.children[i], depth + 1);
			printf("}\n");
			break;
		}

		case AST_FUNCTION: {
			printf("function %.*s: %s ", (int)node->function.name.length, node->function.name.start, data_type_to_string(node->function.returnType.type));
			print_ast_depth(node->function.body, depth);
			break;
		}

		case AST_RETURN: {
			printf("return ");
			print_ast_depth(node->unary, 0);
			printf("\n");
			break;
		}

		case AST_INT_LITERAL: {
			printf("(literal %d)", node->literal.as.integer);
			break;
		}

		case OP_BWNOT:
		case OP_NEG:
		case OP_NOT: {
			printf("%s ", node_type_to_string(node->type));
			print_ast_depth(node->unary, 0);
			break;
		}
	}
}

void print_ast(Node* ast) {
	print_ast_depth(ast, 0);
}