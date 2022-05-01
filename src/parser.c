#include "parser.h"
#include "builtin.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node* parse_expression(Parser* parser);
Node* parse_statement(Parser* parser);
Node* parse_block(Parser* parser);
Node* parse_union_definition(Parser* parser, bool member);
Node* parse_struct_definition(Parser* parser, bool member);

Parser new_parser(Lexer* lexer) {
	Parser parser = {0};
	parser.lexer = lexer;
	return parser;
}

static void error_at_token(Parser* parser, Token* token, const char* message) {
	if(parser->panic) return;
	parser->panic = true;
	fprintf(stderr, "[%zu] Error", token->line);

	if(token->type == TOKEN_EOF) {
		fprintf(stderr, " @ EOF");
	}
	else if(token->type == TOKEN_ERROR) {
		// No token value to print
	}
	else {
		fprintf(stderr, " @ '%.*s'", (int)token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);

	parser->error = true;
}

static void error(Parser* parser, const char* message) {
	error_at_token(parser, &parser->previous, message);
}

static void error_current(Parser* parser, const char* message) {
	error_at_token(parser, &parser->current, message);
}

static void advance(Parser* parser) {
	parser->previous = parser->current;

	for(;;) {
		parser->current = lexer_next(parser->lexer);
		if(parser->current.type != TOKEN_ERROR) break;

		error_current(parser, parser->current.start);
	}
}

static Token consume(Parser* parser, TokenType type, const char* message) {
	if(parser->current.type == type) {
		advance(parser);
		return parser->previous;
	}

	error_current(parser, message);
	return parser->current;
}

static bool check(Parser* parser, TokenType type) {
	return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
	if(!check(parser, type)) return false;
	advance(parser);
	return true;
}

bool allow_expr_stmt(Node* expr) {
	return expr->type == AST_CALL ||
	       expr->type == AST_CALL_METHOD ||
	       expr->type == AST_ASSIGN_VAR ||
	       expr->type == AST_ASSIGN_GLOBAL_VAR ||
		   expr->type == OP_ASSIGN_SUBSCRIPT ||
		   expr->type == OP_ASSIGN_DEREF ||
		   expr->type == OP_ASSIGN_MEMBER;
}

//TODO: Remove duplicate code between parser and typechecker
Type* parser_lookup_type(Parser* parser, Token name) {

	for(size_t i = 0; i < parser->definedTypeCount; i++) {
		Token typeName = parser->definedTypes[i].name;
		if(tokens_equal(typeName, name)) {
			return &parser->definedTypes[i];
		}
	}

	error(parser, "Unknown Type\n");

	return NULL;
}

static int64_t parse_constant(Parser* parser) {
	if(match(parser, TOKEN_INT_LITERAL)) {
		Token literal = parser->previous;
		if(parser->error) return 0;

		return strtol(literal.start, NULL, 10);
	}
	else if(match(parser, TOKEN_CHAR_LITERAL)) {
		Token literal = parser->previous;
		if(parser->error) return 0;

		return literal.start[1];
	}
	else {
		error_current(parser, "Expected constant value");
		return 0;
	}
}

Type parse_type(Parser* parser) {
	advance(parser);

	Type type = { 0 };

	switch (parser->previous.type) {
		case TOKEN_INT: {
			type = (Type) { .type = DATA_TYPE_INT, .indirection = 0 };
			break;
		}
		case TOKEN_I8: {
			type = (Type) { .type = DATA_TYPE_I8, .indirection = 0 };
			break;
		}
		case TOKEN_I16: {
			type = (Type) { .type = DATA_TYPE_I16, .indirection = 0 };
			break;
		}
		case TOKEN_I32: {
			type = (Type) { .type = DATA_TYPE_I32, .indirection = 0 };
			break;
		}
		case TOKEN_I64: {
			type = (Type) { .type = DATA_TYPE_I64, .indirection = 0 };
			break;
		}
		case TOKEN_IDENTIFIER: {
			type = (Type) { .type = DATA_TYPE_UNRESOLVED, .indirection = 0, .name = parser->previous };
			break;
		}
		default: 
			error(parser, "Expected type");
			return (Type) { .type = DATA_TYPE_VOID, .indirection = 0 };
	}

	while(match(parser, TOKEN_STAR)) {
		type.indirection++;
	}

	if(match(parser, TOKEN_LEFT_SQBR)) {
		type.isArray = true;
		type.arrayLength = parse_constant(parser);
		type.indirection++;
		consume(parser, TOKEN_RIGHT_SQBR, "Expected ']' after array length");
	}
	
	return type;
}

Node* parse_call(Parser* parser, Token name) {
	Node* call = new_node(AST_CALL, name);
	call->function.name = name;
	if(!check(parser, TOKEN_RIGHT_PAREN)) {
		do {
			Node* arg = parse_expression(parser);

			call->function.arguments = realloc(call->function.arguments, ++call->function.argumentCount * sizeof(Node*));
			call->function.arguments[call->function.argumentCount - 1] = arg;
		} while(match(parser, TOKEN_COMMA));
	}

	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function arguments");
	
	return call;
}

Node* parse_identifier(Parser* parser) {
	Token name = parser->previous;

	if(match(parser, TOKEN_LEFT_PAREN)) {
		return parse_call(parser, name);
	}

	Node* access = new_node(AST_ACCESS_VAR, name);
	access->variable.name = name;
	access->lvalue = LVALUE_IDENTIFIER;
	return access;
}

Node* parse_value(Parser* parser) {
	if(match(parser, TOKEN_INT_LITERAL)) {
		Token literal = parser->previous;
		if(parser->error) return NULL;

		Node* literalNode = new_node(AST_INT_LITERAL, literal);
		literalNode->literal.type = (Type) { .type = DATA_TYPE_INT, .indirection = 0 };
		literalNode->literal.as.integer = strtol(literal.start, NULL, 10);
		
		return literalNode;
	}
	else if(match(parser, TOKEN_CHAR_LITERAL)) {
		Token literal = parser->previous;
		if(parser->error) return NULL;

		Node* literalNode = new_node(AST_CHAR_LITERAL, literal);
		literalNode->literal.type = (Type) { .type = DATA_TYPE_I8, .indirection = 0 };
		literalNode->literal.as.integer = literal.start[1];
		
		return literalNode;
	}
	else if(match(parser, TOKEN_STRING)) {
		Token literal = parser->previous;
		if(parser->error) return NULL;

		Node* literalNode = new_node(AST_STRING, literal);
		literalNode->literal.type = (Type) { .type = DATA_TYPE_I8, .indirection = 1 };
		literalNode->literal.as.string.chars = literal.start + 1;
		literalNode->literal.as.string.length = literal.length - 2;
		literalNode->literal.as.string.id = parser->stringCount;
		
		parser->strings = realloc(parser->strings, ++parser->stringCount * sizeof(Node*));
		parser->strings[parser->stringCount - 1] = literalNode;

		return literalNode;
	}
	else if(match(parser, TOKEN_IDENTIFIER)) {
		return parse_identifier(parser);
	}
	else if(match(parser, TOKEN_SIZEOF)) {
		Node* expr = new_node(OP_SIZEOF, parser->previous);
		consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after sizeof");
		Type type = parse_type(parser);
		consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof type");
		expr->computedType = type;
		return expr;
	}
	else if(match(parser, TOKEN_LEFT_PAREN)) {
		Node* expr = parse_expression(parser);
		consume(parser, TOKEN_RIGHT_PAREN, "Expected closing ')' after expression");
		return expr;
	}
	else {
		error_current(parser, "Expected value");
		return NULL;
	}
}

Node* parse_member_access(Parser* parser) {
	Node* left = parse_value(parser);

	while(match(parser, TOKEN_LEFT_SQBR) || match(parser, TOKEN_DOT)) {
		Token op = parser->previous;

		if(op.type == TOKEN_LEFT_SQBR) {
			Node* right = parse_expression(parser);

			consume(parser, TOKEN_RIGHT_SQBR, "Expected ']' after subscript index");

			Node* subscript = new_node(OP_ACCESS_SUBSCRIPT, op);
			subscript->binary.lhs = left;
			subscript->binary.rhs = right;
			subscript->lvalue = LVALUE_SUBSCRIPT;
			left = subscript;
		}
		else {
			Token memberName = consume(parser, TOKEN_IDENTIFIER, "Expected member name");
			
			if(match(parser, TOKEN_LEFT_PAREN)) {
				Node* call = parse_call(parser, memberName);
				call->type = AST_CALL_METHOD;
				call->function.name = memberName;
				call->function.isMethod = true;
				call->function.parent = left;
				left = call;
			}
			else {
				Node* member = new_node(OP_ACCESS_MEMBER, op);
				member->member.name = memberName;
				member->member.parent = left;
				member->lvalue = LVALUE_MEMBER;
				left = member;
			}
		}
	}

	return left;
}

Node* parse_unary(Parser* parser) {
	if(match(parser, TOKEN_TILDE)) {
		Token op = parser->previous;
		Node* expr = parse_unary(parser);
		Node* bwnot = new_node(OP_BWNOT ,op);
		bwnot->unary = expr;
		return bwnot;
	}
	else if(match(parser, TOKEN_MINUS)) {
		Token op = parser->previous;
		Node* expr = parse_unary(parser);
		Node* neg = new_node(OP_NEG, op);
		neg->unary = expr;
		return neg;
	}
	else if(match(parser, TOKEN_BANG)) {
		Token op = parser->previous;
		Node* expr = parse_unary(parser);
		Node* not = new_node(OP_NOT, op);
		not->unary = expr;
		return not;
	}
	else if(match(parser, TOKEN_AMP)) {
		Token op = parser->previous;
		Node* expr = parse_unary(parser);
		Node* addrOf = new_node(OP_ADDROF, op);
		addrOf->unary = expr;
		return addrOf;
	}
	else if(match(parser, TOKEN_STAR)) {
		Token op = parser->previous;
		Node* expr = parse_unary(parser);
		Node* deref = new_node(OP_DEREF, op);
		deref->lvalue = LVALUE_DEREF;
		deref->unary = expr;
		return deref;
	}

	return parse_member_access(parser);
}

Node* parse_as_cast(Parser* parser) {
	Node* left = parse_unary(parser);

	if(match(parser, TOKEN_AS)) {
		Token op = parser->previous;
		Type castTo = parse_type(parser);

		Node* cast = new_node(AST_CAST, op);
		cast->unary = left;
		cast->computedType = castTo;
		left = cast;
	}

	return left;
}

Node* parse_term(Parser* parser) {
	Node* left = parse_as_cast(parser);

	while(match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
		Token op = parser->previous;

		Node* right = parse_as_cast(parser);

		NodeType type;
		switch(op.type) {
			case TOKEN_STAR: type = OP_MUL; break;
			case TOKEN_SLASH: type = OP_DIV; break;
			case TOKEN_PERCENT: type = OP_MOD; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_arithmetic(Parser* parser) {
	Node* left = parse_term(parser);

	while(match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
		Token op = parser->previous;

		Node* right = parse_term(parser);

		NodeType type;
		switch(op.type) {
			case TOKEN_PLUS: type = OP_ADD; break;
			case TOKEN_MINUS: type = OP_SUB; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_shift(Parser* parser) {
	Node* left = parse_arithmetic(parser);

	while(match(parser, TOKEN_LSH) || match(parser, TOKEN_RSH)) {
		Token op = parser->previous;

		Node* right = parse_arithmetic(parser);

		NodeType type;
		switch(op.type) {
			case TOKEN_LSH: type = OP_LSH; break;
			case TOKEN_RSH: type = OP_RSH; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_comparison(Parser* parser) {
	Node* left = parse_shift(parser);

	while(match(parser, TOKEN_LESS) || match(parser, TOKEN_LEQ)
		|| match(parser, TOKEN_GREATER) || match(parser, TOKEN_GEQ)) {
		Token op = parser->previous;

		Node* right = parse_shift(parser);

		NodeType type;
		switch(op.type) {
			case TOKEN_LESS: type = OP_LESS; break;
			case TOKEN_LEQ: type = OP_LESS_EQ; break;
			case TOKEN_GREATER: type = OP_GREATER; break;
			case TOKEN_GEQ: type = OP_GREATER_EQ; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_equality(Parser* parser) {
	Node* left = parse_comparison(parser);

	while(match(parser, TOKEN_EQEQ) || match(parser, TOKEN_BANG_EQ)) {
		Token op = parser->previous;

		Node* right = parse_comparison(parser);

		NodeType type;
		switch(op.type) {
			case TOKEN_EQEQ: type = OP_EQUAL; break;
			case TOKEN_BANG_EQ: type = OP_NOT_EQUAL; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_bwand(Parser* parser) {
	Node* left = parse_equality(parser);

	while(match(parser, TOKEN_AMP)) {
		Token op = parser->previous;
		Node* right = parse_equality(parser);

		Node* binary = new_node(OP_BWAND, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_xor(Parser* parser) {
	Node* left = parse_bwand(parser);

	while(match(parser, TOKEN_XOR)) {
		Token op = parser->previous;
		Node* right = parse_bwand(parser);

		Node* binary = new_node(OP_XOR, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_bwor(Parser* parser) {
	Node* left = parse_xor(parser);

	while(match(parser, TOKEN_BAR)) {
		Token op = parser->previous;
		Node* right = parse_xor(parser);

		Node* binary = new_node(OP_BWOR, op);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_ternary_expression(Parser* parser) {
	Node* condition = parse_bwor(parser);

	if(match(parser, TOKEN_QUESTION)) {
		Token op = parser->previous;
		Node* doTrue = parse_expression(parser);

		consume(parser, TOKEN_COLON, "Expected ':' after true branch of ternary condition");

		Node* doFalse = parse_expression(parser);

		Node* ternary = new_node(OP_TERNARY, op);
		ternary->conditional.condition = condition;
		ternary->conditional.doTrue = doTrue;
		ternary->conditional.doFalse = doFalse;
		return ternary;
	}

	return condition;
}

Node* parse_assignment_expression(Parser* parser) {
	Node* left = parse_ternary_expression(parser);

	while(match(parser, TOKEN_EQ)) {
		if(left->lvalue == LVALUE_NONE) {
			error(parser, "Invalid lvalue for assignment");
			return NULL;
		}

		Token op = parser->previous;
		Node* right = parse_assignment_expression(parser);

		switch(left->lvalue) {
			case LVALUE_IDENTIFIER: {
				Token name = left->variable.name;
				Node* assign = new_node(AST_ASSIGN_VAR, name);
				assign->variable.name = name;
				assign->variable.value = right;
				left = assign;
				break;
			}
			case LVALUE_SUBSCRIPT: {
				Node* subscript = new_node(OP_ASSIGN_SUBSCRIPT, op);
				subscript->ternary.lhs = left->binary.lhs;
				subscript->ternary.mid = left->binary.rhs;
				subscript->ternary.rhs = right;
				left = subscript;
				break;
			}
			case LVALUE_DEREF: {
				Node* assign = new_node(OP_ASSIGN_DEREF, op);
				assign->binary.lhs = left->unary;
				assign->binary.rhs = right;
				left = assign;
				break;
			}
			case LVALUE_MEMBER: {
				Node* assign = new_node(OP_ASSIGN_MEMBER, op);
				assign->member.name = left->member.name;
				assign->member.parent = left->member.parent;
				assign->member.value = right;
				left = assign;
				break;
			}
			default: {
				assert(0 && "Unreachable - parse_assignment_expression lvalue");
			}
		}
	}

	return left;
}

Node* parse_expression(Parser* parser) {
	return parse_assignment_expression(parser);
}

Node* parse_if_statement(Parser* parser) {
	Node* ifStmt = new_node(AST_IF, parser->previous);

	consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after if");

	ifStmt->conditional.condition = parse_expression(parser);

	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after if condition");

	ifStmt->conditional.doTrue = parse_statement(parser);

	if(match(parser, TOKEN_ELSE)) {
		ifStmt->conditional.doFalse = parse_statement(parser);
	}

	return ifStmt;
}

Node* parse_return_statement(Parser* parser) {
	Node* returnStmt = new_node(AST_RETURN, parser->previous);

	if(!check(parser, TOKEN_SEMICOLON))
		returnStmt->unary = parse_expression(parser);

	consume(parser, TOKEN_SEMICOLON, "Expected ';' after return statement");

	return returnStmt;
}

Node* parse_var_declaration(Parser* parser) {
	Token name = consume(parser, TOKEN_IDENTIFIER, "Expected variable name");

	Type type = (Type) { .type = DATA_TYPE_VOID, .indirection = 0 };
	
	if(match(parser, TOKEN_COLON)) {
		type = parse_type(parser);
	}

	Node* value = NULL;
	if(match(parser, TOKEN_EQ)) {
		if(type.isArray && match(parser, TOKEN_LEFT_SQBR)) {
			Node* array = new_node(AST_ARRAY_INIT, parser->previous);

			if(!check(parser, TOKEN_RIGHT_SQBR)) {
				do {
					Node* item = parse_expression(parser);
					node_add_child(array, item);
				} while(match(parser, TOKEN_COMMA));
			}
			consume(parser, TOKEN_RIGHT_SQBR, "Expected ']' after array initialization");
			value = array;
		}
		else
			value = parse_expression(parser);
	}

	consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");

	Node* var = new_node(AST_DEFINE_VAR, name);
	var->variable.name = name;
	var->variable.type = type;
	var->variable.value = value;

	return var;
}

Node* parse_while_statement(Parser* parser) {
	Node* whileStmt = new_node(AST_WHILE, parser->previous);

	consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after while");

	whileStmt->conditional.condition = parse_expression(parser);

	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after while condition");

	whileStmt->conditional.doTrue = parse_statement(parser);

	return whileStmt;
}

Node* parse_for_statement(Parser* parser) {
	Node* forStmt = new_node(AST_FOR, parser->previous);

	consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after for");

	if(match(parser, TOKEN_SEMICOLON)) {
		// No initialiser clause
	}
	else if(match(parser, TOKEN_VAR)) {
		forStmt->loop.initial = parse_var_declaration(parser);
	}
	else if(check(parser, TOKEN_IDENTIFIER)) {
		forStmt->loop.initial = parse_statement(parser);
	}
	else {
		error_current(parser, "Expected for initializer clause");
	}

	if(!match(parser, TOKEN_SEMICOLON)) {
		forStmt->loop.condition = parse_expression(parser);
		consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition");
	}

	if(!match(parser, TOKEN_RIGHT_PAREN)) {
		forStmt->loop.iteration = parse_expression(parser);
		consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after for clauses");
	}

	forStmt->loop.body = parse_statement(parser);

	return forStmt;
}

Node* parse_statement(Parser* parser) {
	if(match(parser, TOKEN_IF)) {
		return parse_if_statement(parser);
	}
	else if(match(parser, TOKEN_FOR)) {
		return parse_for_statement(parser);
	}
	else if(match(parser, TOKEN_RETURN)) {
		return parse_return_statement(parser);
	}
	else if(match(parser, TOKEN_VAR)) {
		return parse_var_declaration(parser);
	}
	else if(match(parser, TOKEN_WHILE)) {
		return parse_while_statement(parser);
	}
	else if(match(parser, TOKEN_LEFT_BRACE)) {
		return parse_block(parser);
	}
	
	Node* expr = parse_expression(parser);

	if(expr == NULL) return NULL;

	if(!allow_expr_stmt(expr)) {
		error(parser, "Expected statement");
	}

	consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
		
	Node* exprStmt = new_node(AST_EXPR_STMT, expr->position);
	exprStmt->unary = expr;
	return exprStmt;
}

Node* parse_block(Parser* parser) {
	Node* block = new_node(AST_BLOCK, parser->previous);
	block->block.parent = parser->currentBlock;
	parser->currentBlock = block;

	while(!check(parser, TOKEN_RIGHT_BRACE)) {
		Node* stmt = parse_statement(parser);
		if(parser->error) break;
		node_add_child(block, stmt);
	}

	consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after block");

	parser->currentBlock = block->block.parent;

	return block;
}

Node* parse_function(Parser* parser) {
	Token name = consume(parser, TOKEN_IDENTIFIER, "Expected function name");

	Node* function = new_node(AST_FUNCTION, name);
	function->function.arguments = NULL;
	function->function.argumentCount = 0;

	if(match(parser, TOKEN_DOT)) {
		Type* parent = parser_lookup_type(parser, name);
		if(parent == NULL) return NULL;
		function->function.parentType = *parent;
		function->function.isMethod = true;
		Token methodName = consume(parser, TOKEN_IDENTIFIER, "Expected function name");

		Token argName = (Token) { .length=4, .start="this", .type=TOKEN_IDENTIFIER, .line=0 };
		Type type = (Type) { .type = DATA_TYPE_UNRESOLVED, .name = name, .indirection = 1 };

		Node* arg = new_node(AST_DEFINE_VAR, argName);
		arg->variable.name = argName;
		arg->variable.type = type;

		function->function.arguments = realloc(function->function.arguments, ++function->function.argumentCount * sizeof(Node*));
		function->function.arguments[function->function.argumentCount - 1] = arg;

		parent->methods = realloc(parent->methods, ++parent->methodCount * sizeof(Node*));
		parent->methods[parent->methodCount - 1] = function;

		name = methodName;
	}

	function->function.name = name;
	
	consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name");

	if(!check(parser, TOKEN_RIGHT_PAREN)) {
		do {
			Token argName = consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
			consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
			Type type = parse_type(parser);

			Node* arg = new_node(AST_DEFINE_VAR, argName);
			arg->variable.name = argName;
			arg->variable.type = type;

			function->function.arguments = realloc(function->function.arguments, ++function->function.argumentCount * sizeof(Node*));
			function->function.arguments[function->function.argumentCount - 1] = arg;
		} while(match(parser, TOKEN_COMMA));
	}

	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function parameters");

	Type type;
	if(match(parser, TOKEN_COLON)) {
		type = match(parser, TOKEN_VOID) ? (Type) { .type = DATA_TYPE_VOID, .indirection = 0 } : parse_type(parser);
	}
	else {
		// Functions return void by default
		type = (Type) {.type = DATA_TYPE_VOID };
	}

	function->function.returnType = type;

	consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before function body");


	parser->currentFunction = function;

	parser->functions = realloc(parser->functions, ++parser->functionCount * sizeof(Node*));
	parser->functions[parser->functionCount - 1] = function;

	Node* body = parse_block(parser);

	function->function.body = body;

	return function;
}

void parse_member_definition(Parser* parser, Type* type) {
	Token memName = consume(parser, TOKEN_IDENTIFIER, "Expected member name");

	consume(parser, TOKEN_COLON, "Expected ':' after member name");

	if(match(parser, TOKEN_UNION)) {
		Node* vnion = parse_union_definition(parser, true);

		consume(parser, TOKEN_SEMICOLON, "Expected ';' after member declaration");

		vnion->variable.name = memName;
		vnion->computedType.name = memName;

		Node* member = new_node(AST_MEMBER, memName);
		member->variable.name = memName;
		member->variable.type = vnion->computedType;
		member->variable.value = vnion;

		type->fields = realloc(type->fields, ++type->fieldCount * sizeof(Node*));
		type->fields[type->fieldCount - 1] = member;
	}
	else if(match(parser, TOKEN_STRUCT)) {
		Node* strukt = parse_struct_definition(parser, true);

		consume(parser, TOKEN_SEMICOLON, "Expected ';' after member declaration");

		strukt->variable.name = memName;
		strukt->computedType.name = memName;

		Node* member = new_node(AST_MEMBER, memName);
		member->variable.name = memName;
		member->variable.type = strukt->computedType;
		member->variable.value = strukt;

		type->fields = realloc(type->fields, ++type->fieldCount * sizeof(Node*));
		type->fields[type->fieldCount - 1] = member;
	}
	else {
		Type memType = parse_type(parser);

		consume(parser, TOKEN_SEMICOLON, "Expected ';' after member declaration");

		Node* member = new_node(AST_MEMBER, memName);
		member->variable.name = memName;
		member->variable.type = memType;

		type->fields = realloc(type->fields, ++type->fieldCount * sizeof(Node*));
		type->fields[type->fieldCount - 1] = member;
	}
}

Node* parse_struct_definition(Parser* parser, bool member) {
	Token name;

	if(!member) {
		name = consume(parser, TOKEN_IDENTIFIER, "Expected struct name");
	}

	consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before struct body");

	if(match(parser, TOKEN_RIGHT_BRACE)) error(parser, "Expected at least one struct member");

	Type structType = (Type) { .type = DATA_TYPE_STRUCT };

	structType.name = name;

	do {
		parse_member_definition(parser, &structType);
	} while(!check(parser, TOKEN_RIGHT_BRACE));

	consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after struct body");

	Node* strukt = new_node(AST_STRUCT, name);
	strukt->computedType = structType;
	strukt->variable.name = name;
	
	if(!member) {
		//TODO check for duplicate structure names
		parser->definedTypes = realloc(parser->definedTypes, ++parser->definedTypeCount * sizeof(Type));
		parser->definedTypes[parser->definedTypeCount - 1] = structType;
	}

	return strukt;
}

Node* parse_union_definition(Parser* parser, bool member) {
	Token name;

	if(!member) {
		name = consume(parser, TOKEN_IDENTIFIER, "Expected union name");
	}

	consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before union body");

	if(match(parser, TOKEN_RIGHT_BRACE)) error(parser, "Expected at least one union member");

	Type unionType = (Type) { .type = DATA_TYPE_UNION };

	unionType.name = name;

	do {
		parse_member_definition(parser, &unionType);
	} while(!check(parser, TOKEN_RIGHT_BRACE));

	consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after union body");

	Node* vnion = new_node(AST_UNION, name);
	vnion->computedType = unionType;
	vnion->variable.name = name;

	if(!member) {
		//TODO check for duplicate structure names
		parser->definedTypes = realloc(parser->definedTypes, ++parser->definedTypeCount * sizeof(Type));
		parser->definedTypes[parser->definedTypeCount - 1] = unionType;
	}

	return vnion;
}

Node* parse_program(Parser* parser) {
	advance(parser);
	Node* program = new_node(AST_PROGRAM, parser->current);

	int builtinCount;
	Node** builtins = parser_builtin_functions(&builtinCount);

	for(int i = 0; i < builtinCount; i++) {
		parser->functions = realloc(parser->functions, ++parser->functionCount * sizeof(Node*));
		parser->functions[parser->functionCount - 1] = builtins[i];
	}

	while(!match(parser, TOKEN_EOF)) {
		if(match(parser, TOKEN_FUNCTION)) {
			Node* function = parse_function(parser);
			node_add_child(program, function);
		}
		else if(match(parser, TOKEN_VAR)) {
			Node* var = parse_var_declaration(parser);
			var->type = AST_DEFINE_GLOBAL_VAR;
			node_add_child(program, var);
		}
		else if(match(parser, TOKEN_STRUCT)) {
			Node* strukt = parse_struct_definition(parser, false);
			node_add_child(program, strukt);
		}
		else if(match(parser, TOKEN_UNION)) {
			Node* vnion = parse_union_definition(parser, false);
			node_add_child(program, vnion);
		}
		else {
			error_current(parser, "Expected function definition");
			break;
		}
	}

	return program;
}
