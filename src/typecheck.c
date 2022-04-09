#include "typecheck.h"
#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void type_check_expr(Parser* parser, Node* expr);
void type_check_block(Parser* parser, Node* block);

static Type typeStack[128];
static int typeStackDepth = 0;

static Type* intType = &(Type) { .type = DATA_TYPE_INT };

void print_position(Token position) {
	fprintf(stderr, "[%zu] Error ", position.line);

	if(position.type == TOKEN_EOF) {
		fprintf(stderr, "@ EOF");
	}
	else {
		fprintf(stderr, "@ '%.*s'", (int)position.length, position.start);
	}
	fprintf(stderr, ": ");
}

void push_type_stack(Type* type) {
	assert(typeStackDepth < 128);
	typeStack[typeStackDepth++] = *type;
}

Type pop_type_stack() {
	assert(typeStackDepth > 0);
	return typeStack[--typeStackDepth];
}

bool types_assignable(Type* a, Type* b) {
	return a->type == b->type && a->indirection == b->indirection;
}

int sizeof_type(Type* type) {
	switch(type->type) {
		case DATA_TYPE_INT: return 8;
		case DATA_TYPE_VOID: assert(0 && "Not reached - sizeof_type(void)");
		default: assert(0 && "Unreachable - sizeof_type");
	}
	return 0;
}

Node* lookup_variable(Parser* parser, Token name) {
	Node* block = parser->currentBlock;

	while(block != NULL) {
		for(int i = 0; i < block->block.variableCount; i++) {
			Token varName = block->block.variables[i]->variable.name;
			if(name.length == varName.length && memcmp(name.start, varName.start, name.length) == 0) {
				return block->block.variables[i];
			}
		}
		block = block->block.parent;
	}

	for(int i = 0; i < parser->currentFunction->function.argumentCount; i++) {
		Token argName = parser->currentFunction->function.arguments[i]->variable.name;
		if(name.length == argName.length && memcmp(name.start, argName.start, name.length) == 0) {
			return parser->currentFunction->function.arguments[i];
		}
	}

	for(int i = 0; i < parser->functionCount; i++) {
		Token funcName = parser->functions[i]->function.name;
		if(name.length == funcName.length && memcmp(name.start, funcName.start, name.length) == 0) {
			return parser->functions[i];
		}
	}

	return NULL;
}

void type_check_unary(Parser* parser, Node* expr) {
	if(expr->type == OP_ADDROF) {
		if(!expr->unary->lvalue) {
			print_position(expr->position);
			fprintf(stderr, "Expected lvalue as operand for unary '&'\n");
			exit(1);
		}
		
		type_check_expr(parser, expr->unary);
		Type type = pop_type_stack();

		type.indirection++;

		push_type_stack(&type);
		return;
	}

	type_check_expr(parser, expr->unary);

	Type type = pop_type_stack();

	if(!types_assignable(&type, intType)) {
		print_position(expr->position);
		fprintf(stderr, "Cannot perform operation '%s' on type %s\n", node_type_to_string(expr->type), type_to_string(type));
		exit(1);
	}
	push_type_stack(&type);
}

void type_check_binary(Parser* parser, Node* expr) {
	type_check_expr(parser, expr->binary.lhs);
	Type left = pop_type_stack();

	type_check_expr(parser, expr->binary.rhs);
	Type right = pop_type_stack();

	if(!types_assignable(&left, intType) || !types_assignable(&right, intType)) {
		print_position(expr->position);
		fprintf(stderr, "Cannot perform operation '%s' on types %s and %s\n", node_type_to_string(expr->type), type_to_string(left), type_to_string(right));
		exit(1);
	}

	//FIXME: The left may not always have the priority type
	push_type_stack(&left);
}

void type_check_access_var(Parser* parser, Node* expr) {
	Node* variable = lookup_variable(parser, expr->variable.name);

	if(variable == NULL) {
		print_position(expr->position);
		fprintf(stderr, "Unknown variable '%.*s' in current scope\n", (int)expr->variable.name.length, expr->variable.name.start);
		exit(1);
	}

	if(variable->type != AST_DEFINE_VAR) {
		print_position(expr->position);
		fprintf(stderr, "Can only access variables\n");
		exit(1);
	}

	expr->variable.type = variable->variable.type;
	expr->variable.stackOffset = variable->variable.stackOffset;

	push_type_stack(&expr->variable.type);
}

void type_check_assign_var(Parser* parser, Node* expr) {
	Node* variable = lookup_variable(parser, expr->variable.name);

	if(variable == NULL) {
		print_position(expr->position);
		fprintf(stderr, "Unknown variable '%.*s' in current scope\n", (int)expr->variable.name.length, expr->variable.name.start);
		exit(1);
	}

	if(variable->type != AST_DEFINE_VAR) {
		print_position(expr->position);
		fprintf(stderr, "Can only assign to variables\n");
		exit(1);
	}

	type_check_expr(parser, expr->variable.value);
	Type valueType = pop_type_stack();

	if(!types_assignable(&valueType, &variable->variable.type)) {
		print_position(expr->position);
		fprintf(stderr, "Cannot assign type %s to variable of type %s\n", type_to_string(valueType), type_to_string(variable->variable.type));
		exit(1);
	}

	expr->variable.type = variable->variable.type;
	expr->variable.stackOffset = variable->variable.stackOffset;

	push_type_stack(&expr->variable.type);
}

void type_check_call(Parser* parser, Node* expr) {
	Node* function = lookup_variable(parser, expr->function.name);

	if(function == NULL) {
		print_position(expr->position);
		fprintf(stderr, "Unknown function '%.*s' in current scope\n", (int)expr->variable.name.length, expr->variable.name.start);
		exit(1);
	}

	if(function->type != AST_FUNCTION) {
		print_position(expr->position);
		fprintf(stderr, "Can only call functions\n");
		exit(1);
	}

	if(function->function.argumentCount != expr->function.argumentCount) {
		print_position(expr->position);
		fprintf(stderr, "Call expected %d arguments but got %d\n", function->function.argumentCount, expr->function.argumentCount);
		exit(1);
	}

	for(int i = 0; i < function->function.argumentCount; i++) {
		Node* arg = expr->function.arguments[i];
		type_check_expr(parser, arg);

		Type argType = pop_type_stack();
		Type* paramType = &function->function.arguments[i]->variable.type;

		if(!types_assignable(&argType, paramType)) {
			print_position(expr->position);
			fprintf(stderr, "Function argument %d expected type %s but got %s\n", i, type_to_string(*paramType), type_to_string(argType));
			exit(1);
		}
	}

	push_type_stack(&function->function.returnType);
}

void type_check_ternary_expr(Parser* parser, Node* expr) {
	type_check_expr(parser, expr->conditional.condition);
	Type condition = pop_type_stack();

	if(!types_assignable(&condition, intType)) {
		print_position(expr->position);
		fprintf(stderr, "Cannot use type '%s' as condition", type_to_string(condition));
		exit(1);
	}

	type_check_expr(parser, expr->conditional.doTrue);
	Type doTrue = pop_type_stack();

	type_check_expr(parser, expr->conditional.doFalse);
	Type doFalse = pop_type_stack();

	if(!types_assignable(&doTrue, &doFalse)) {
		print_position(expr->position);
		fprintf(stderr, "Cannot conditionally return differing types '%s' and '%s'", type_to_string(doTrue), type_to_string(doFalse));
		exit(1);
	}

	push_type_stack(&doTrue);
}

void type_check_expr(Parser* parser, Node* expr) {
	if(is_unary_op(expr->type)) {
		type_check_unary(parser, expr);
	}
	else if(is_binary_op(expr->type)) {
		type_check_binary(parser, expr);
	}
	else if(expr->type == AST_INT_LITERAL) {
		push_type_stack(intType);
	}
	else if(expr->type == AST_ACCESS_VAR) {
		type_check_access_var(parser, expr);
	}
	else if(expr->type == AST_ASSIGN_VAR) {
		type_check_assign_var(parser, expr);
	}
	else if(expr->type == AST_CALL) {
		type_check_call(parser, expr);
	}
	else if(expr->type == OP_TERNARY) {
		type_check_ternary_expr(parser, expr);
	}
	else {
		assert(0 && "Unreachable - type_check_expr");
	}
}

void type_check_statement(Parser* parser, Node* stmt) {
	if(stmt->type == AST_IF) {
		type_check_expr(parser, stmt->conditional.condition);

		Type conditionType = pop_type_stack();

		if(!types_assignable(&conditionType, intType)) {
			print_position(stmt->position);
			fprintf(stderr, "Expected condition to be type 'int' but got '%s'\n", type_to_string(conditionType));
			exit(1);
		}

		type_check_statement(parser, stmt->conditional.doTrue);

		if(stmt->conditional.doFalse != NULL)
			type_check_statement(parser, stmt->conditional.doFalse);
	}
	else if(stmt->type == AST_FOR) {
		if(stmt->loop.initial != NULL) type_check_statement(parser, stmt->loop.initial);
		if(stmt->loop.condition != NULL) {
			type_check_expr(parser, stmt->loop.condition);
			Type conditionType = pop_type_stack();

			if(!types_assignable(&conditionType, intType)) {
				print_position(stmt->position);
				fprintf(stderr, "Expected condition to be type 'int' but got '%s'\n", type_to_string(conditionType));
				exit(1);
			}
		}
		if(stmt->loop.iteration != NULL) {
			type_check_expr(parser, stmt->loop.iteration);
			pop_type_stack();
		}

		type_check_statement(parser, stmt->loop.body);
	}
	else if(stmt->type == AST_WHILE) {
		type_check_expr(parser, stmt->conditional.condition);

		Type conditionType = pop_type_stack();

		if(!types_assignable(&conditionType, intType)) {
			print_position(stmt->position);
			fprintf(stderr, "Expected condition to be type 'int' but got '%s'\n", type_to_string(conditionType));
			exit(1);
		}

		type_check_statement(parser, stmt->conditional.doTrue);
	}
	else if(stmt->type == AST_RETURN) {
		type_check_expr(parser, stmt->unary);

		Type returnType = pop_type_stack();
		Type* expectedType = &parser->currentFunction->function.returnType;
		if(!types_assignable(&returnType, expectedType)) {
			print_position(stmt->position);
			fprintf(stderr, "Cannot return type '%s' from function expecting '%s'\n", type_to_string(returnType), type_to_string(*expectedType));
			exit(1);
		}
	}
	else if(stmt->type == AST_DEFINE_VAR) {
		Type* declType = &stmt->variable.type;
		Token name = stmt->variable.name;

		for(int i = 0; i < parser->currentBlock->block.variableCount; i++) {
			Token varName = parser->currentBlock->block.variables[i]->variable.name;
			if(name.length == varName.length && memcmp(name.start, varName.start, name.length) == 0) {
				print_position(stmt->position);
				fprintf(stderr, "Redeclaration of variable '%.*s' in current scope\n", (int)name.length, name.start);
				exit(1);
			}
		}

		stmt->variable.stackOffset = parser->currentBlock->block.currentStackOffset += sizeof_type(declType);

		if(stmt->variable.stackOffset > parser->currentFunction->function.localVariableStackOffset) {
			parser->currentFunction->function.localVariableStackOffset = stmt->variable.stackOffset;
		}

		if(stmt->variable.value) {
			type_check_expr(parser, stmt->variable.value);

			Type valueType = pop_type_stack();

			if(!types_assignable(&valueType, declType)) {
				print_position(stmt->position);
				fprintf(stderr, "Cannot assign type '%s' to variable expecting '%s'\n", type_to_string(valueType), type_to_string(*declType));
				exit(1);
			}
		}

		parser->currentBlock->block.variables = realloc(parser->currentBlock->block.variables, ++parser->currentBlock->block.variableCount);
		parser->currentBlock->block.variables[parser->currentBlock->block.variableCount - 1] = stmt;
	}
	else if(stmt->type == AST_EXPR_STMT) {
		type_check_expr(parser, stmt->unary);
		pop_type_stack();
	}
	else if(stmt->type == AST_BLOCK) {
		type_check_block(parser, stmt);
	}
	else {
		assert(0 && "Unreachable");
	}
}

void type_check_block(Parser* parser, Node* block) {
	block->block.currentStackOffset = parser->currentBlock->block.currentStackOffset;
	parser->currentBlock = block;
	for(int i = 0; i < block->block.size; i++) {
		type_check_statement(parser, block->block.children[i]);
	}
	parser->currentBlock = block->block.parent;
}

void type_check_function(Parser* parser, Node* function) {
	assert(function->type == AST_FUNCTION);

	int stackOffset = 0;

	for(int i = 0; i < function->function.argumentCount; i++) {
		Node* arg = function->function.arguments[i];

		stackOffset += sizeof_type(&arg->variable.type);

		arg->variable.stackOffset = stackOffset;
	}

	function->function.body->block.currentStackOffset = stackOffset;
	function->function.localVariableStackOffset = stackOffset;

	parser->currentFunction = function;

	parser->currentBlock = function->function.body;
	type_check_block(parser, function->function.body);
}

void type_check(Parser* parser, Node* program) {
	assert(program->type == AST_PROGRAM);

	for(int i = 0; i < parser->functionCount; i++) {
		if(!parser->functions[i]->function.hasImplicitBody)
			type_check_function(parser, parser->functions[i]);
	}
}