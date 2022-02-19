#include "codegen.h"

void generate_expr_rax(Node* expr, FILE* out) {
	if(expr->type == AST_INT_LITERAL) {
		fprintf(out, "    mov rax, %d\n", expr->literal.as.integer);
	}
	else {
		fprintf(stderr, "Unsupported type '%d' in generate_expr_rax", expr->type);
		exit(1);
	}
}

void generate_statement(Node* stmt, FILE* out) {
	if(stmt->type == AST_RETURN) {
		generate_expr_rax(stmt->unary, out);
		fprintf(out, "    ret\n");
	}
	else {
		fprintf(stderr, "Unsupported type '%d' in generate_statement", stmt->type);
		exit(1);
	}
}

void generate_block(Node* block, FILE* out) {
	for(size_t i = 0; i < block->block.size; i++) {
		generate_statement(block->block.children[i], out);
	}
}

void generate_function(Node* function, FILE* out) {
	fprintf(out, "global %.*s\n", (int)function->function.name.length, function->function.name.start);
	fprintf(out, "%.*s:\n", (int)function->function.name.length, function->function.name.start);

	generate_block(function->function.body, out);
}

void generate_program(Node* ast, FILE* out) {
	for(size_t i = 0; i < ast->block.size; i++) {
		if(ast->block.children[i]->type == AST_FUNCTION) {
			generate_function(ast->block.children[i], out);
		}
		else {
			fprintf(stderr, "Unsupported type '%d' in generate_program", ast->block.children[i]->type);
			exit(1);
		}
	}

	// Entry point of _start -> calls main and exits (via syscall)
	// Uses main's return value as the exit code
	fprintf(out, "global _start\n");
	fprintf(out, "_start:\n");
	fprintf(out, "    call main\n");
	fprintf(out, "    mov rdi, rax\n");
	fprintf(out, "    mov rax, 60\n");
	fprintf(out, "    syscall\n");
}
