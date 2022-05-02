#include "codegen.h"
#include "typecheck.h"
#include "builtin.h"
#include <assert.h>

void generate_expr_rax(Node* expr, FILE* out);
void generate_statement(Node* stmt, FILE* out);
void generate_block(Node* block, FILE* out);

static const char* ARG_REGISTERS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
static int labelCount = 0;
static int registersInUse = 0;

int ceil_multiple(int num, int n) {
	return ((num + n - 1) / n) * n;
}

int min(int a, int b) {
	return a < b ? a : b;
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

void generate_copy(int fieldCount, Node** fields, int parentOffset, int memberOffset, FILE* out) {
	for(int i = 0; i < fieldCount; i++) {
		Node* field = fields[i];
		if(is_structural_type(&field->variable.type)) {
			generate_copy(field->variable.type.fieldCount, field->variable.type.fields, field->variable.stackOffset, memberOffset, out);
			continue;
		}
		fprintf(out, "    mov rax, %s [rcx+%d]\n", type_to_qualifier(&field->variable.type), 
			    field->variable.stackOffset + parentOffset);
		fprintf(out, "    mov %s [rdx+%d], rax\n", type_to_qualifier(&field->variable.type), 
			    field->variable.stackOffset + parentOffset + memberOffset);
	}
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
		if(expr->unary->lvalue == LVALUE_LOCAL) {
			fprintf(out, "    lea rax, [rbp-%d]\n", expr->unary->variable.stackOffset);
		}
		else if(expr->unary->lvalue == LVALUE_GLOBAL) {
			fprintf(out, "    lea rax, [_g_%.*s]\n", (int)expr->unary->variable.name.length, expr->unary->variable.name.start);
		}
		else if(expr->unary->lvalue == LVALUE_SUBSCRIPT) {
			generate_expr_rax(expr->unary->binary.lhs, out);
			fprintf(out, "    push rax\n");
			generate_expr_rax(expr->unary->binary.rhs, out);
			fprintf(out, "    mov rbx, rax\n");
			fprintf(out, "    pop rax\n");

			fprintf(out, "    lea rax, [rax+rbx*%d]\n", sizeof_type(&expr->unary->computedType));
		}
		else if(expr->unary->lvalue == LVALUE_MEMBER) {
			generate_expr_rax(expr->unary->member.parent, out);
			Node* field = expr->unary->member.memberRef;
			fprintf(out, "    lea rax, [rax+%d]\n", field->variable.stackOffset);
		}
		else {
			assert(0 && "Unreachable - unknown lvalue type");
		}
	}
	else if(expr->type == OP_DEREF) {
		if(sizeof_type(&expr->computedType) > 8) {
			//TODO: Copy structs
			fprintf(stderr, "Cannot copy structs (yet)\n");
			exit(1);
		}
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

void generate_logical_or_rax(Node* expr, FILE* out) {
	int label = labelCount++;
	generate_expr_rax(expr->binary.lhs, out);
	fprintf(out, "    mov rcx, rax\n");
	fprintf(out, "    mov rax, 1\n");
	fprintf(out, "    test rcx, rcx\n");
	fprintf(out, "    jne .l%d\n", label);
	generate_expr_rax(expr->binary.rhs, out);
	fprintf(out, "    test rax, rax\n");
	fprintf(out, "    setne al\n");
	fprintf(out, "    movzx rax, al\n");
	fprintf(out, ".l%d:\n", label);
}

void generate_logical_and_rax(Node* expr, FILE* out) {
	int label = labelCount++;
	generate_expr_rax(expr->binary.lhs, out);
	fprintf(out, "    test rax, rax\n");
	fprintf(out, "    je .l%d\n", label);
	generate_expr_rax(expr->binary.rhs, out);
	fprintf(out, "    test rax, rax\n");
	fprintf(out, "    setne al\n");
	fprintf(out, "    movzx rax, al\n");
	fprintf(out, ".l%d:\n", label);
}

void generate_binary_rax(Node* expr, FILE* out) {
	if(expr->type == OP_LSH || expr->type == OP_RSH) {
		generate_shift_rax(expr, out);
		return;
	}

	else if(expr->type == OP_OR) {
		generate_logical_or_rax(expr, out);
		return;
	}
	else if(expr->type == OP_AND) {
		generate_logical_and_rax(expr, out);
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
		if(expr->variable.type.isArray || is_structural_type(&expr->variable.type)) {
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
		if(expr->variable.type.isArray || is_structural_type(&expr->variable.type))
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
		for(int i = 0; i < registersInUse; i++) {
			fprintf(out, "    push %s\n", ARG_REGISTERS[i]);
		}

		int localRegUse = 0;

		for(int i = 0; i < min(expr->function.argumentCount, 6); i++) {
			generate_expr_rax(expr->function.arguments[i], out);
			fprintf(out, "    mov %s, rax\n", ARG_REGISTERS[i]);
			registersInUse++;
			localRegUse++;
		}

		for(int i = 0; i < expr->function.argumentCount - 6; i++) {
			generate_expr_rax(expr->function.arguments[i + 6], out);
			fprintf(out, "    push rax\n");
		}

		registersInUse -= localRegUse;

		fprintf(out, "    call _f_%.*s\n", (int)expr->function.name.length, expr->function.name.start);

		for(int i = 0; i < registersInUse; i++) {
			fprintf(out, "    pop %s\n", ARG_REGISTERS[registersInUse - i - 1]);
		}
	}
	else if(expr->type == AST_CALL_METHOD) {
		for(int i = 0; i < registersInUse; i++) {
			fprintf(out, "    push %s\n", ARG_REGISTERS[i]);
		}

		generate_expr_rax(expr->function.parent, out);
		fprintf(out, "    mov rdi, rax\n");

		int localRegUse = 0;

		for(int i = 0; i < min(expr->function.argumentCount, 5); i++) {
			generate_expr_rax(expr->function.arguments[i], out);
			fprintf(out, "    mov %s, rax\n", ARG_REGISTERS[i + 1]);
			registersInUse++;
			localRegUse++;
		}

		for(int i = 0; i < expr->function.argumentCount - 5; i++) {
			generate_expr_rax(expr->function.arguments[i + 5], out);
			fprintf(out, "    push rax\n");
		}

		registersInUse -= localRegUse;


		fprintf(out, "    call _m_%.*s_%.*s\n",  (int)expr->function.parentType.name.length, expr->function.parentType.name.start, 
		       (int)expr->function.name.length, expr->function.name.start);

		for(int i = 0; i < registersInUse; i++) {
			fprintf(out, "    pop %s\n", ARG_REGISTERS[registersInUse - i - 1]);
		}
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
		fprintf(out, "    push rax\n");
		generate_expr_rax(expr->ternary.mid, out);
		fprintf(out, "    push rax\n");
		generate_expr_rax(expr->ternary.rhs, out);
		fprintf(out, "    pop rcx\n");
		fprintf(out, "    pop rbx\n");

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
	// Two different instructions is redundant
	else if(expr->type == OP_ACCESS_MEMBER || expr->type == OP_ACCESS_MEMBER_PTR) {
		generate_expr_rax(expr->member.parent, out);
		Node* field = expr->member.memberRef;

		if(is_structural_type(&field->variable.type)) {
			fprintf(out, "    lea rax, [rax+%d]\n", field->variable.stackOffset);
		}
		else {
			fprintf(out, "    %s %s, %s [rax+%d]\n", type_movzx(&field->variable.type), type_movzx_rax_subregister(&field->variable.type),
		    	    type_to_qualifier(&field->variable.type), field->variable.stackOffset);
		}
	}
	else if(expr->type == OP_ASSIGN_MEMBER || expr->type == OP_ASSIGN_MEMBER_PTR) {
		generate_expr_rax(expr->member.parent, out);
		fprintf(out, "    push rax\n");
		generate_expr_rax(expr->member.value, out);
		fprintf(out, "    pop rbx\n");
		Node* field = expr->member.memberRef;
		fprintf(out, "    mov %s [rbx+%d], %s\n", type_to_qualifier(&field->variable.type), field->variable.stackOffset,
		        type_to_rax_subregister(&field->variable.type));
	}
	else if(expr->type == OP_COPY_STRUCT) {
		generate_expr_rax(expr->variable.value, out);
		fprintf(out, "    mov rcx, rax\n");
		fprintf(out, "    lea rdx, [rbp-%d]\n", expr->variable.stackOffset);
		
		generate_copy(expr->variable.type.fieldCount, expr->variable.type.fields, 0, 0, out);
		fprintf(out, "    mov rax, rdx\n");
	}
	else if(expr->type == OP_COPY_STRUCT_DEREF) {
		generate_expr_rax(expr->binary.lhs, out);
		fprintf(out, "    mov rdx, rax\n");
		generate_expr_rax(expr->binary.rhs, out);
		fprintf(out, "    mov rcx, rax\n");

		generate_copy(expr->computedType.fieldCount, expr->computedType.fields, 0, 0, out);
		fprintf(out, "    mov rax, rdx\n");
	}
	else if(expr->type == OP_COPY_STRUCT_MEMBER) {
		generate_expr_rax(expr->member.parent, out);
		fprintf(out, "    mov rdx, rax\n");
		generate_expr_rax(expr->member.value, out);
		fprintf(out, "    mov rcx, rax\n");
		
		Node* member = expr->member.memberRef;
		int memberOffset = member->variable.stackOffset;

		generate_copy(member->variable.type.fieldCount, member->variable.type.fields, 0, memberOffset, out);
		fprintf(out, "    mov rax, rdx\n");
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
	if(!function->function.isMethod) {
		fprintf(out, "global _f_%.*s\n", (int)function->function.name.length, function->function.name.start);
		fprintf(out, "_f_%.*s:\n", (int)function->function.name.length, function->function.name.start);
	}
	else {
		fprintf(out, "global _m_%.*s_%.*s\n", (int)function->function.parentType.name.length, function->function.parentType.name.start, 
		       (int)function->function.name.length, function->function.name.start);
		fprintf(out, "_m_%.*s_%.*s:\n", (int)function->function.parentType.name.length, function->function.parentType.name.start, 
		       (int)function->function.name.length, function->function.name.start);
	}

	fprintf(out, "    push rbp\n");
	fprintf(out, "    mov rbp, rsp\n");
	int stackDepth = ceil_multiple(function->function.localVariableStackOffset, 16);
	if(stackDepth != 0) {
		fprintf(out, "    sub rsp, %d\n", stackDepth);
	}

	for(int i = 0; i < min(function->function.argumentCount, 6); i++) {
		fprintf(out, "    mov QWORD [rbp-%d], %s\n", function->function.arguments[i]->variable.stackOffset, ARG_REGISTERS[i]);
	}

	for(int i = 0; i < function->function.argumentCount - 6; i++) {
		fprintf(out, "    mov rax, QWORD [rbp+%d]\n", 8 + (8 * (function->function.argumentCount - 6 - i)));
		fprintf(out, "    mov QWORD [rbp-%d], rax\n", function->function.arguments[i + 6]->variable.stackOffset);
	}

	generate_block(function->function.body, out);
}

void generate_program(Parser* parser, Node* ast, FILE* out) {
	fprintf(out, "section .text\n");
	
	write_builtin_functions(out);

	for(size_t i = 0; i < ast->block.size; i++) {
		if(ast->block.children[i]->type == AST_FUNCTION) {
			if(!ast->block.children[i]->function.hasImplicitBody)
				generate_function(ast->block.children[i], out);
		}
		else if(ast->block.children[i]->type == AST_DEFINE_GLOBAL_VAR) {
			// Handled elsewhere
		}
		else if(ast->block.children[i]->type == AST_DEFINE_CONST) {
			// No code generated - expanded inline
		}
		else if(ast->block.children[i]->type == AST_STRUCT) {
			// No code generated
		}
		else if(ast->block.children[i]->type == AST_UNION) {
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

	fprintf(out, "    call _f_main\n");
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
