#include "codegen.h"
#include "typecheck.h"
#include <assert.h>

void generate_expr_rax(Node* expr, FILE* out);
void generate_statement(Node* stmt, FILE* out);
void generate_block(Node* block, FILE* out);

static const char* ARG_REGISTERS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
static int labelCount = 0;

int ceil_multiple(int num, int n) {
	return ((num + n - 1) / n) * n;
}

char* type_to_qualifier(Type* type) {
	switch(sizeof_type(type)) {
		case 1: return "BYTE";
		case 2: return "WORD";
		case 4: return "DWORD";
		case 8: return "QWORD";
	}
	assert(0 && "Unreachable - invalid size of type");
}

char* type_to_rax_subregister(Type* type) {
	switch(sizeof_type(type)) {
		case 1: return "al";
		case 2: return "ax";
		case 4: return "eax";
		case 8: return "rax";
	}
	assert(0 && "Unreachable - invalid size of type");
}

char* type_movzx(Type* type) {
	int size = sizeof_type(type);
	if(size == 1 || size == 2) {
		return "movzx";
	}
	return "mov";
}

char* type_movzx_rax_subregister(Type* type) {
	if(sizeof_type(type) == 4) return "eax";
	return "rax";
}

char* type_to_res(Type* type) {
	switch(sizeof_type(type)) {
		case 1: return "resb";
		case 2: return "resw";
		case 4: return "resd";
		case 8: return "resq";
	}
	assert(0 && "Unreachable - invalid size of type");
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
	else if(expr->type == OP_ADDROF) {
		if(expr->unary->lvalue == LVALUE_IDENTIFIER) {
			if(expr->unary->type == AST_ACCESS_VAR) {
				fprintf(out, "    lea rax, [rbp-%d]\n", expr->unary->variable.stackOffset);
			}
			else if(expr->unary->type == AST_ACCESS_GLOBAL_VAR) {
				fprintf(out, "    lea rax, [_g_%.*s]\n", (int)expr->unary->variable.name.length, expr->unary->variable.name.start);
			}
			else {
				assert(0 && "Unreachable - unhandled node type");
			}
		}
		else if(expr->unary->lvalue == LVALUE_SUBSCRIPT) {
			generate_expr_rax(expr->unary->binary.lhs, out);
			fprintf(out, "    push rax\n");
			generate_expr_rax(expr->unary->binary.rhs, out);
			fprintf(out, "    mov rbx, rax\n");
			fprintf(out, "    pop rax\n");

			fprintf(out, "    lea rax, [rax+rbx*%d]\n", sizeof_type(&expr->unary->computedType));
		}
		else {
			assert(0 && "Unreachable - unknown lvalue type");
		}
	}
	else if(expr->type == OP_DEREF) {
		generate_expr_rax(expr->unary, out);
		fprintf(out, "    %s %s, %s [rax]\n", type_movzx(&expr->computedType), type_movzx_rax_subregister(&expr->computedType), type_to_qualifier(&expr->computedType));
	}
	else {
		fprintf(stderr, "Unsupported type '%s' in generate_unary_rax\n", node_type_to_string(expr->type));
		exit(1);
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
		fprintf(out, "    mov rax, %ld\n", expr->literal.as.integer);
	}
	else if(expr->type == AST_CHAR_LITERAL) {
		fprintf(out, "    mov rax, %ld\n", expr->literal.as.integer);
	}
	else if(expr->type == AST_ACCESS_VAR) {
		if(expr->variable.type.isArray) {
			fprintf(out, "    lea rax, [rbp-%d]\n", expr->variable.stackOffset);
		}
		else
		fprintf(out, "    %s %s, %s [rbp-%d]\n", type_movzx(&expr->variable.type), type_movzx_rax_subregister(&expr->variable.type), type_to_qualifier(&expr->variable.type), expr->variable.stackOffset);
	}
	else if(expr->type == AST_ASSIGN_VAR) {
		generate_expr_rax(expr->variable.value, out);
		fprintf(out, "    mov %s [rbp-%d], %s\n", type_to_qualifier(&expr->variable.type), expr->variable.stackOffset, type_to_rax_subregister(&expr->variable.type));
	}
	else if(expr->type == AST_ACCESS_GLOBAL_VAR) {
		if(expr->variable.type.isArray)
			fprintf(out, "    lea rax, [_g_%.*s]\n", (int)expr->variable.name.length, expr->variable.name.start);
		else
			fprintf(out, "    %s %s, %s [_g_%.*s]\n", type_movzx(&expr->variable.type), type_movzx_rax_subregister(&expr->variable.type), type_to_qualifier(&expr->variable.type), 
			        (int)expr->variable.name.length, expr->variable.name.start);
	}
	else if(expr->type == AST_ASSIGN_GLOBAL_VAR) {
		generate_expr_rax(expr->variable.value, out);
		fprintf(out, "    mov %s [_g_%.*s], %s\n", type_to_qualifier(&expr->variable.type), (int)expr->variable.name.length, expr->variable.name.start, type_to_rax_subregister(&expr->variable.type));
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
	else if(expr->type == AST_STRING) {
		fprintf(out, "    lea rax, [_s%d]\n", expr->literal.as.string.id);
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
	else if(expr->type == OP_SIZEOF) {
		fprintf(out, "    mov rax, %d\n", sizeof_type_var_offset(&expr->computedType));
	}
	else if(expr->type == OP_ACCESS_SUBSCRIPT) {
		generate_expr_rax(expr->binary.lhs, out);
		fprintf(out, "    push rax\n");
		generate_expr_rax(expr->binary.rhs, out);
		fprintf(out, "    mov rbx, rax\n");
		fprintf(out, "    pop rax\n");

		fprintf(out, "    %s %s, %s [rax+rbx*%d]\n", type_movzx(&expr->computedType), type_movzx_rax_subregister(&expr->computedType), 
		        type_to_qualifier(&expr->computedType), sizeof_type(&expr->computedType));
	}
	else if(expr->type == OP_ASSIGN_SUBSCRIPT) {
		generate_expr_rax(expr->ternary.lhs, out);
		fprintf(out, "    mov rbx, rax\n");
		generate_expr_rax(expr->ternary.mid, out);
		fprintf(out, "    mov rcx, rax\n");
		generate_expr_rax(expr->ternary.rhs, out);

		fprintf(out, "    mov %s [rbx+rcx*%d], %s\n", type_to_qualifier(&expr->computedType), sizeof_type(&expr->computedType), 
		        type_to_rax_subregister(&expr->computedType));
	}
	else if(expr->type == OP_ASSIGN_DEREF) {
		generate_expr_rax(expr->binary.lhs, out);
		fprintf(out, "    push rax\n");
		generate_expr_rax(expr->binary.rhs, out);
		fprintf(out, "    pop rbx\n");

		fprintf(out, "    mov %s [rbx], %s\n", type_to_qualifier(&expr->computedType), type_to_rax_subregister(&expr->computedType));
	}
	else if(expr->type == AST_CAST) {
		generate_expr_rax(expr->unary, out);
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

void generate_for_statement(Node* forStmt, FILE* out) {
	if(forStmt->loop.initial != NULL)
		generate_statement(forStmt->loop.initial, out);

	bool hasCondition = forStmt->loop.condition != NULL;
	int condLabel = labelCount++;
	int bodyLabel = labelCount++;

	if(hasCondition)
		fprintf(out, "    jmp .l%d\n", condLabel);
	
	fprintf(out, ".l%d:\n", bodyLabel);

	generate_statement(forStmt->loop.body, out);

	if(forStmt->loop.iteration != NULL)
		generate_expr_rax(forStmt->loop.iteration, out);

	if(hasCondition) {
		fprintf(out, ".l%d:\n", condLabel);
		generate_expr_rax(forStmt->loop.condition, out);
		fprintf(out, "    test rax, rax\n");
		fprintf(out, "    jne .l%d\n", bodyLabel);
	}
	else {
		fprintf(out, "    jmp .l%d\n", bodyLabel);
	}
}

void generate_while_statement(Node* whileStmt, FILE* out) {
	int condLabel = labelCount++;
	int bodyLabel = labelCount++;

	fprintf(out, "    jmp .l%d\n", condLabel);
	fprintf(out, ".l%d:\n", bodyLabel);

	generate_statement(whileStmt->conditional.doTrue, out);

	fprintf(out, ".l%d:\n", condLabel);
	generate_expr_rax(whileStmt->conditional.condition, out);
	fprintf(out, "    test rax, rax\n");
	fprintf(out, "    jne .l%d\n", bodyLabel);
}

void generate_statement(Node* stmt, FILE* out) {
	if(stmt->type == AST_IF) {
		generate_if_statement(stmt, out);
	}
	else if(stmt->type == AST_FOR) {
		generate_for_statement(stmt, out);
	}
	else if(stmt->type == AST_WHILE) {
		generate_while_statement(stmt, out);
	}
	else if(stmt->type == AST_RETURN) {
		if(stmt->unary)
			generate_expr_rax(stmt->unary, out);
		fprintf(out, "    leave\n");
		fprintf(out, "    ret\n");
	}
	else if(stmt->type == AST_DEFINE_VAR) {
		if(stmt->variable.value != NULL) {
			if(stmt->variable.value->type != AST_ARRAY_INIT) {
				generate_expr_rax(stmt->variable.value, out);
				fprintf(out, "    mov %s [rbp-%d], %s\n", type_to_qualifier(&stmt->variable.type), stmt->variable.stackOffset, type_to_rax_subregister(&stmt->variable.type));
			}
			else {
				Node* array = stmt->variable.value;
				Type itemType = {};
				itemType.type = stmt->variable.type.type;
				itemType.indirection = stmt->variable.type.indirection - 1;
				for(int i = 0; i < array->block.size; i++) {
					generate_expr_rax(array->block.children[i], out);
					fprintf(out, "    mov %s [rbp-%d], %s\n", type_to_qualifier(&itemType), stmt->variable.stackOffset - (i * sizeof_type(&itemType)), type_to_rax_subregister(&itemType));
				}
			}
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

void generate_implicit_printu_impl(FILE* out) {
	fprintf(out, "printu:\n");
	fprintf(out, "		sub     rsp, 40\n");
	fprintf(out, "		mov     eax, 10\n");
	fprintf(out, "		mov     esi, 19\n");
	fprintf(out, "		mov  r10, -3689348814741910323\n");
	fprintf(out, "		mov     WORD [rsp+20], ax\n");
	fprintf(out, "		lea     r8, [rsp+19]\n");
	fprintf(out, ".L2:\n");
	fprintf(out, "		mov     rax, rdi\n");
	fprintf(out, "		movsxd  r9, esi\n");
	fprintf(out, "		sub     r8, 1\n");
	fprintf(out, "		sub     esi, 1\n");
	fprintf(out, "		mul     r10\n");
	fprintf(out, "		mov     rax, rdi\n");
	fprintf(out, "		shr     rdx, 3\n");
	fprintf(out, "		lea     rcx, [rdx+rdx*4]\n");
	fprintf(out, "		add     rcx, rcx\n");
	fprintf(out, "		sub     rax, rcx\n");
	fprintf(out, "		mov     rcx, rdi\n");
	fprintf(out, "		mov     rdi, rdx\n");
	fprintf(out, "		add     eax, 48\n");
	fprintf(out, "		mov     BYTE [r8+1], al\n");
	fprintf(out, "		cmp     rcx, 9\n");
	fprintf(out, "		ja      .L2\n");
	fprintf(out, "		mov     rdi, 1\n");
	fprintf(out, "		mov     edx, 20\n");
	fprintf(out, "		mov     rax, 1\n");
	fprintf(out, "		sub     edx, esi\n");
	fprintf(out, "		lea     rsi, [rsp+r9]\n");
	fprintf(out, "		syscall\n");
	fprintf(out, "		add     rsp, 40\n");
	fprintf(out, "		ret\n");
}

void generate_implicit_syscall3_impl(FILE* out) {
	fprintf(out, "syscall3:\n");
	fprintf(out, "    mov rax, rdi\n");
	fprintf(out, "    mov rdi, rsi\n");
	fprintf(out, "    mov rsi, rdx\n");
	fprintf(out, "    mov rdx, rcx\n");
	fprintf(out, "    syscall\n");
	fprintf(out, "    ret\n");
}

void generate_program(Parser* parser, Node* ast, FILE* out) {
	fprintf(out, "section .text\n");
	generate_implicit_printu_impl(out);
	generate_implicit_syscall3_impl(out);

	for(size_t i = 0; i < ast->block.size; i++) {
		if(ast->block.children[i]->type == AST_FUNCTION) {
			if(!ast->block.children[i]->function.hasImplicitBody)
				generate_function(ast->block.children[i], out);
		}
		else if(ast->block.children[i]->type == AST_DEFINE_GLOBAL_VAR) {
			// Handled elsewhere
		}
		else if(ast->block.children[i]->type == AST_STRUCT) {
			// No code generated
		}
		else {
			fprintf(stderr, "Unsupported type '%s' in generate_program\n", node_type_to_string(ast->block.children[i]->type));
			exit(1);
		}
	}

	// Entry point of _start -> initalises global variables, calls main and exits (via syscall)
	// Uses main's return value as the exit code
	fprintf(out, "global _start\n");
	fprintf(out, "_start:\n");

	for(int i = 0; i < parser->globalVarCount; i++) {
		Node* var = parser->globalVars[i];

		if(var->variable.value != NULL) {
			if(var->variable.value->type == AST_ARRAY_INIT) {
				Node* array = var->variable.value;
				Type itemType = {};
				itemType.type = var->variable.type.type;
				itemType.indirection = var->variable.type.indirection - 1;

				for(int i = 0; i < array->block.size; i++) {
					generate_expr_rax(array->block.children[i], out);
					fprintf(out, "    mov %s [_g_%.*s+%d], %s\n", 
					type_to_qualifier(&var->variable.type), (int)var->variable.name.length, var->variable.name.start, 
					        i * sizeof_type(&itemType), type_to_rax_subregister(&var->variable.type));
				}
			}
			else {
				generate_expr_rax(var->variable.value, out);
				fprintf(out, "    mov %s [_g_%.*s], %s\n", type_to_qualifier(&var->variable.type), (int)var->variable.name.length, var->variable.name.start, type_to_rax_subregister(&var->variable.type));
			}
		}
	}

	fprintf(out, "    call main\n");
	fprintf(out, "    mov rdi, rax\n");
	fprintf(out, "    mov rax, 60\n");
	fprintf(out, "    syscall\n");

	if(parser->globalVarCount > 0) {
		fprintf(out, "section .bss\n");

		for(int i = 0; i < parser->globalVarCount; i++) {
			Node* var = parser->globalVars[i];

			fprintf(out, "_g_%.*s: ", (int)var->variable.name.length, var->variable.name.start);

			Type type = var->variable.type;

			if(type.isArray) {
				Type subType = {};
				subType.indirection = type.indirection - 1;
				subType.type = type.type;
				fprintf(out, "%s %d\n", type_to_res(&subType), type.arrayLength);
			}
			else
				fprintf(out, "%s 1\n", type_to_res(&var->variable.type));
		}
	}

	if(parser->stringCount > 0) {
		fprintf(out, "section .data\n");

		for(size_t i = 0; i < parser->stringCount; i++) {
			Node* str = parser->strings[i];

			fprintf(out, "_s%d: db \"%.*s\", 0\n", str->literal.as.string.id, (int)str->literal.as.string.length, str->literal.as.string.chars);
		}
	}
}
