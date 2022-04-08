#include "codegen.h"

void generate_expr_rax(Node* expr, FILE* out);
void generate_statement(Node* stmt, FILE* out);
void generate_block(Node* block, FILE* out);

static const char* ARG_REGISTERS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
static int labelCount = 0;

int ceil_multiple(int num, int n) {
	return ((num + n - 1) / n) * n;
}

void generate_unary_rax(Node* expr, FILE* out) {
	if(expr->type == OP_BWNOT) {
		generate_expr_rax(expr->unary, out);
		fprintf(out, "    not rax\n");
	}
	else if(expr->type == OP_NEG) {
		generate_expr_rax(expr->unary, out);
		fprintf(out, "    neg rax\n");
	}
	else if(expr->type == OP_NOT) {
		generate_expr_rax(expr->unary, out);
		fprintf(out, "    test rax, rax\n");
		fprintf(out, "    sete al\n");
		fprintf(out, "    movzx rax, al\n");
	}
}

void generate_shift_rax(Node* expr, FILE* out) {
	generate_expr_rax(expr->binary.lhs, out);
	fprintf(out, "    push rax\n");
	generate_expr_rax(expr->binary.rhs, out);
	fprintf(out, "    mov rcx, rax\n");
	fprintf(out, "    pop rax\n");

	if(expr->type == OP_LSH) {
		fprintf(out, "    sal rax, cl\n");
	}
	else if(expr->type == OP_RSH) {
		fprintf(out, "    sar rax, cl\n");
	}
	else {
		fprintf(stderr, "Unsupported type '%s' in generate_shift_rax\n", node_type_to_string(expr->type));
		exit(1);
	}
}

void generate_binary_rax(Node* expr, FILE* out) {
	if(expr->type == OP_LSH || expr->type == OP_RSH) {
		generate_shift_rax(expr, out);
		return;
	}

	generate_expr_rax(expr->binary.lhs, out);
	fprintf(out, "    push rax\n");
	generate_expr_rax(expr->binary.rhs, out);
	fprintf(out, "    mov rbx, rax\n");
	fprintf(out, "    pop rax\n");
	if(expr->type == OP_ADD) {
		fprintf(out, "    add rax, rbx\n");
	}
	else if(expr->type == OP_SUB) {
		fprintf(out, "    sub rax, rbx\n");
	}
	else if(expr->type == OP_MUL) {
		fprintf(out, "    imul rax, rbx\n");
	}
	else if(expr->type == OP_DIV) {
		fprintf(out, "    cqo\n");
		fprintf(out, "    idiv rbx\n");
	}
	else if(expr->type == OP_MOD) {
		fprintf(out, "    cqo\n");
		fprintf(out, "    idiv rbx\n");
		fprintf(out, "    mov rax, rdx\n");
	}
	else if(expr->type == OP_BWAND) {
		fprintf(out, "    and rax, rbx\n");
	}
	else if(expr->type == OP_BWOR) {
		fprintf(out, "    or rax, rbx\n");
	}
	else if(expr->type == OP_XOR) {
		fprintf(out, "    xor rax, rbx\n");
	}
	else if(expr->type == OP_EQUAL) {
		fprintf(out, "    cmp rax, rbx\n");
		fprintf(out, "    sete al\n");
		fprintf(out, "    movzx rax, al\n");
	}
	else if(expr->type == OP_NOT_EQUAL) {
		fprintf(out, "    cmp rax, rbx\n");
		fprintf(out, "    setne al\n");
		fprintf(out, "    movzx rax, al\n");
	}
	else if(expr->type == OP_LESS) {
		fprintf(out, "    cmp rax, rbx\n");
		fprintf(out, "    setl al\n");
		fprintf(out, "    movzx rax, al\n");
	}
	else if(expr->type == OP_LESS_EQ) {
		fprintf(out, "    cmp rax, rbx\n");
		fprintf(out, "    setle al\n");
		fprintf(out, "    movzx rax, al\n");
	}
	else if(expr->type == OP_GREATER) {
		fprintf(out, "    cmp rax, rbx\n");
		fprintf(out, "    setg al\n");
		fprintf(out, "    movzx rax, al\n");
	}
	else if(expr->type == OP_GREATER_EQ) {
		fprintf(out, "    cmp rax, rbx\n");
		fprintf(out, "    setge al\n");
		fprintf(out, "    movzx rax, al\n");
	}
	else {
		fprintf(stderr, "Unsupported type '%s' in generate_binary_rax\n", node_type_to_string(expr->type));
		exit(1);
	}
}

void generate_expr_rax(Node* expr, FILE* out) {
	if(is_unary_op(expr->type)) {
		generate_unary_rax(expr, out);
	}
	else if(is_binary_op(expr->type)) {
		generate_binary_rax(expr, out);
	}
	else if(expr->type == AST_INT_LITERAL) {
		fprintf(out, "    mov rax, %d\n", expr->literal.as.integer);
	}
	else if(expr->type == AST_ACCESS_VAR) {
		//TODO get word based on size
		fprintf(out, "    mov rax, QWORD [rbp-%d]\n", expr->variable.stackOffset);
	}
	else if(expr->type == AST_ASSIGN_VAR) {
		generate_expr_rax(expr->variable.value, out);
		//TODO get word based on size
		fprintf(out, "    mov QWORD [rbp-%d], rax\n", expr->variable.stackOffset);
	}
	else if(expr->type == AST_CALL) {
		if(expr->function.argumentCount > 6) {
			fprintf(stderr, "Functions cannot have more than 6 arguments (func '%.*s')\n", (int)expr->function.name.length, expr->function.name.start);
			exit(1);
		}

		for(int i = 0; i < expr->function.argumentCount; i++) {
			generate_expr_rax(expr->function.arguments[i], out);
			fprintf(out, "    mov %s, rax\n", ARG_REGISTERS[i]);
		}

		fprintf(out, "    call %.*s\n", (int)expr->function.name.length, expr->function.name.start);
	}
	else if(expr->type == OP_TERNARY) {
		generate_expr_rax(expr->conditional.condition, out);
		fprintf(out, "    test rax, rax\n");
		int fLabel = labelCount++;
		int endLabel = labelCount++;

		fprintf(out, "    je .l%d\n", fLabel);
		generate_expr_rax(expr->conditional.doTrue, out);
		fprintf(out, "    jmp .l%d\n", endLabel);

		fprintf(out, ".l%d:\n", fLabel);
		generate_expr_rax(expr->conditional.doFalse, out);

		fprintf(out, ".l%d:\n", endLabel);
	}
	else {
		fprintf(stderr, "Unsupported type '%s' in generate_expr_rax\n", node_type_to_string(expr->type));
		exit(1);
	}
}

void generate_if_statement(Node* ifStmt, FILE* out) {
	generate_expr_rax(ifStmt->conditional.condition, out);
	fprintf(out, "    test rax, rax\n");
	int falseLabel = labelCount++;
	int endLabel = labelCount++;

	fprintf(out, "    je .l%d\n", falseLabel);
	generate_statement(ifStmt->conditional.doTrue, out);

	if(ifStmt->conditional.doFalse != NULL)
		fprintf(out, "    jmp .l%d\n", endLabel);

	fprintf(out, ".l%d:\n", falseLabel);

	if(ifStmt->conditional.doFalse != NULL) {
		generate_statement(ifStmt->conditional.doFalse, out);
		fprintf(out, ".l%d:\n", endLabel);
	}
}

void generate_statement(Node* stmt, FILE* out) {
	if(stmt->type == AST_IF) {
		generate_if_statement(stmt, out);
	}
	else if(stmt->type == AST_RETURN) {
		generate_expr_rax(stmt->unary, out);
		fprintf(out, "    leave\n");
		fprintf(out, "    ret\n");
	}
	else if(stmt->type == AST_DEFINE_VAR) {
		if(stmt->variable.value != NULL) {
			generate_expr_rax(stmt->variable.value, out);
			//TODO get word based on size
			fprintf(out, "    mov QWORD [rbp-%d], rax\n", stmt->variable.stackOffset);
		}
	}
	else if(stmt->type == AST_EXPR_STMT) {
		generate_expr_rax(stmt->unary, out);
	}
	else if(stmt->type == AST_BLOCK) {
		generate_block(stmt, out);
	}
	else {
		fprintf(stderr, "Unsupported type '%s' in generate_statement\n", node_type_to_string(stmt->type));
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

	fprintf(out, "    push rbp\n");
	fprintf(out, "    mov rbp, rsp\n");
	
	int stackDepth = ceil_multiple(function->function.localVariableStackOffset, 16);
	if(stackDepth != 0)
		fprintf(out, "    sub rsp, %d\n", stackDepth);

	if(function->function.argumentCount > 6) {
		fprintf(stderr, "Functions cannot have more than 6 arguments (func '%.*s')\n", (int)function->function.name.length, function->function.name.start);
		exit(1);
	}

	for(int i = 0; i < function->function.argumentCount; i++) {
		fprintf(out, "    mov QWORD [rbp-%d], %s\n", function->function.arguments[i]->variable.stackOffset, ARG_REGISTERS[i]);
	}

	generate_block(function->function.body, out);
}

void generate_program(Node* ast, FILE* out) {
	for(size_t i = 0; i < ast->block.size; i++) {
		if(ast->block.children[i]->type == AST_FUNCTION) {
			generate_function(ast->block.children[i], out);
		}
		else {
			fprintf(stderr, "Unsupported type '%s' in generate_program\n", node_type_to_string(ast->block.children[i]->type));
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
