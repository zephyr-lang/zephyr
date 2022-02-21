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

bool is_unary_op(NodeType type) {
	switch(type) {
		case OP_BWNOT:
		case OP_NEG:
		case OP_NOT:
			return true;
		default:
			return false;
	}
}

bool is_binary_op(NodeType type) {
	switch(type) {
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_BWAND:
		case OP_BWOR:
		case OP_XOR:
		case OP_LSH:
		case OP_RSH:
			return true;
		default:
			return false;
	}
}

void print_ast_depth(Node* node, int depth) {
	for(int i = 0; i < depth; i++) printf("  ");

	if(is_unary_op(node->type)) {
		printf("%s ", node_type_to_string(node->type));
		print_ast_depth(node->unary, 0);
		return;
	}

	else if(is_binary_op(node->type)) {
		printf("%s (", node_type_to_string(node->type));
		print_ast_depth(node->binary.lhs, depth);
		printf(") (");
		print_ast_depth(node->binary.rhs, depth);
		printf(")");
		return;
	}

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

		default: break; // Unreachable
	}
}

void print_ast(Node* ast) {
	print_ast_depth(ast, 0);
}