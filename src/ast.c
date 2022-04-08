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

char* type_to_string(Type type) {
	return data_type_to_string(type.type);
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
		case OP_EQUAL:
		case OP_NOT_EQUAL:
		case OP_LESS:
		case OP_LESS_EQ:
		case OP_GREATER:
		case OP_GREATER_EQ:
			return true;
		default:
			return false;
	}
}

void print_ast_depth(Node* node, int depth) {
	for(int i = 0; i < depth; i++) printf("  ");

	if(is_unary_op(node->type)) {
		printf("%s\n", node_type_to_string(node->type));
		print_ast_depth(node->unary, depth + 1);
		return;
	}

	else if(is_binary_op(node->type)) {
		printf("%s\n", node_type_to_string(node->type));
		print_ast_depth(node->binary.lhs, depth + 1);
		printf("\n");
		print_ast_depth(node->binary.rhs, depth + 1);
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
			for(int i = 0; i < depth; i++) printf("  ");
			printf("}\n");
			break;
		}

		case AST_EXPR_STMT: {
			print_ast_depth(node->unary, 0);
			printf("\n");
			break;
		}

		case AST_FUNCTION: {
			printf("function %.*s: %s\n", (int)node->function.name.length, node->function.name.start, type_to_string(node->function.returnType));
			for(int i = 0; i < node->function.argumentCount; i++) {
				Node* arg = node->function.arguments[i];
				printf("(%.*s: %s) ", (int)arg->variable.name.length, arg->variable.name.start, type_to_string(arg->variable.type));
			}
			print_ast_depth(node->function.body, depth);
			break;
		}

		case AST_IF: {
			printf("if\n");
			print_ast_depth(node->conditional.condition, depth + 1);
			printf("\n");
			print_ast_depth(node->conditional.doTrue, depth + 1);
			printf("\n");

			if(node->conditional.doFalse != NULL) {
				for(int i = 0; i < depth; i++) printf("  ");
				printf("else\n");
				print_ast_depth(node->conditional.doFalse, depth + 1);
				printf("\n");
			}
			break;
		}

		case AST_RETURN: {
			printf("return\n");
			print_ast_depth(node->unary, depth + 1);
			printf("\n");
			break;
		}

		case AST_INT_LITERAL: {
			printf("(literal %d)", node->literal.as.integer);
			break;
		}

		case AST_DEFINE_VAR: {
			printf("var %.*s: %s ", (int)node->variable.name.length, node->variable.name.start, type_to_string(node->variable.type));
			if(node->variable.value != NULL) {
				print_ast_depth(node->variable.value, 0);
			}
			printf("\n");
			break;
		}

		case AST_ACCESS_VAR: {
			printf("(var %.*s)", (int)node->variable.name.length, node->variable.name.start);
			break;
		}

		case AST_ASSIGN_VAR: {
			printf("(%.*s = ", (int)node->variable.name.length, node->variable.name.start);
			print_ast_depth(node->variable.value, 0);
			printf(")");
			break;
		}

		case AST_CALL: {
			printf("(call %.*s", (int)node->function.name.length, node->function.name.start);
			for(int i = 0; i < node->function.argumentCount; i++) {
				Node* arg = node->function.arguments[i];
				printf(" ");
				print_ast_depth(arg, 0);
			}
			printf(")");
			break;
		}

		case OP_TERNARY: {
			printf("(");
			print_ast_depth(node->conditional.condition, 0);
			printf(") ?\n");
			print_ast_depth(node->conditional.doTrue, depth + 1);
			printf("\n");
			print_ast_depth(node->conditional.doFalse, depth + 1);
			break;
		}

		default: {
			fprintf(stderr, "Cannot handle type '%s' in print_ast_depth\n", node_type_to_string(node->type));
			break;
		}
	}
}

void print_ast(Node* ast) {
	print_ast_depth(ast, 0);
}