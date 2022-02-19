#include "ast.h"
#include <stdio.h>

char* data_type_to_string(DataType type) {
	switch(type) {
		case DATA_TYPE_VOID: "void";
		case DATA_TYPE_INT: "int";
	}
	return "<unknown type>";
}

void print_ast_depth(Node* node, int depth) {
	for(int i = 0; i < depth; i++) printf("  ");

	switch(node->type) {
		case AST_PROGRAM: {
			for(size_t i = 0; i < node->block.size; i++)
				print_ast_depth(node->block.children[i], depth + 1);
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
			printf("function %s: %s\n", node->function.name, data_type_to_string(node->function.returnType.type));
			print_ast_depth(node->function.body, depth + 1);
			break;
		}

		case AST_RETURN: {
			printf("return ");
			print_ast_depth(node->unary, depth);
			printf("\n");
			break;
		}

		case AST_INT_LITERAL: {
			printf("(literal %d)", node->literal.as.integer);
			break;
		}
	}
}

void print_ast(Node* ast) {
	print_ast_depth(ast, 0);
}