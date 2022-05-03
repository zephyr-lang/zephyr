#include "builtin.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

Node* add_implicit_printu_function() {
	Token name = (Token) { .type = TOKEN_IDENTIFIER, .start = "printu", .length = 6, .line = 0 };
	Node* function = new_node(AST_FUNCTION, name);
	function->function.name = name;

	function->function.arguments = malloc(1 * sizeof(Node*));
	function->function.argumentCount = 1;

	Token arg0Name = (Token) { .type = TOKEN_IDENTIFIER, .start = "value", .length = 5, .line = 0 };
	Node* arg0 = new_node(AST_DEFINE_VAR, arg0Name);
	arg0->variable.name = arg0Name;
	arg0->variable.type = (Type) { .type = DATA_TYPE_INT, .indirection = 0 };
	function->function.arguments[0] = arg0;

	function->function.returnType = (Type) {.type = DATA_TYPE_VOID, .indirection = 0 };

	function->function.hasImplicitBody = true;

	return function;
}

Node* add_implicit_syscall_function(char* sysName, int argCount) {
	Token name = (Token) { .type = TOKEN_IDENTIFIER, .start = sysName, .length = strlen(sysName), .line = 0 };
	Node* function = new_node(AST_FUNCTION, name);
	function->function.name = name;

	function->function.arguments = malloc((argCount + 1) * sizeof(Node*));
	function->function.argumentCount = argCount + 1;

	Token nrTok = (Token) { .type = TOKEN_IDENTIFIER, .start = "nr", .length = 2, .line = 0 };
	Node* nr = new_node(AST_DEFINE_VAR, nrTok);
	nr->variable.name = nrTok;
	nr->variable.type = (Type) { .type = DATA_TYPE_INT, .indirection = 0 };
	function->function.arguments[0] = nr;

	// Argument names after the write syscall

	for(int i = 0; i < argCount; i++) {
		Token argTok = (Token) { .type = TOKEN_IDENTIFIER, .start = "arg", .length = 3, .line = 0 };
		Node* arg = new_node(AST_DEFINE_VAR, argTok);
		arg->variable.name = argTok;
		arg->variable.type = (Type) { .type = DATA_TYPE_ANY, .indirection = 0 };
		function->function.arguments[i + 1] = arg;
	}

	function->function.returnType = (Type) {.type = DATA_TYPE_INT, .indirection = 0 };

	function->function.hasImplicitBody = true;

	return function;
}

Node** parser_builtin_functions(int* funcCount) {
	*funcCount = 7;

	Node** funcs = malloc(*funcCount * sizeof(Node*));

	int count = 0;

	funcs[count++] = add_implicit_printu_function();
	funcs[count++] = add_implicit_syscall_function("syscall0", 0);
	funcs[count++] = add_implicit_syscall_function("syscall1", 1);
	funcs[count++] = add_implicit_syscall_function("syscall2", 2);
	funcs[count++] = add_implicit_syscall_function("syscall3", 3);
	funcs[count++] = add_implicit_syscall_function("syscall4", 4);
	funcs[count++] = add_implicit_syscall_function("syscall5", 5);
	//TODO: Currently only supports calling with up to 6 arguments
	//funcs[count++] = add_implicit_syscall_function("syscall6", 6);
	//funcs[count++] = add_implicit_syscall_function("syscall7", 7);

	assert(count == *funcCount);
	return funcs;
}

void generate_implicit_printu_impl(FILE* out) {
	fprintf(out, "_f_printu:\n");
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

void write_builtin_functions(FILE* out) {
	generate_implicit_printu_impl(out);

	// TODO: Taken from codegen.c
	const char* ARG_REGISTERS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
	const char* SYS_REGISTERS[] = { "rax", "rdi", "rsi", "rdx", "r10", "r8" };

	// syscall0-5
	for(int i = 0; i <= 5; i++) {
		fprintf(out, "_f_syscall%d:\n", i);

		for(int arg = 0; arg <= i; arg++) {
			fprintf(out, "    mov %s, %s\n", SYS_REGISTERS[arg], ARG_REGISTERS[arg]);
		}
		fprintf(out, "    syscall\n");
		fprintf(out, "    ret\n");
	}
}