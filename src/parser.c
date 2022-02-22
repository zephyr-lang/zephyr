#include "parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node* parse_expression(Parser* parser);

Parser new_parser(Lexer* lexer) {
	Parser parser = {0};
	parser.lexer = lexer;
	parser.currentFunction = NULL;
	parser.functions = NULL;
	return parser;
}

static Node* new_node(NodeType type) {
	Node* node = calloc(1, sizeof(Node));
	node->type = type;
	return node;
}

static void node_add_child(Node* parent, Node* child) {
	parent->block.children = realloc(parent->block.children, ++parent->block.size);
	parent->block.children[parent->block.size - 1] = child;
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

Type parse_type(Parser* parser) {
	advance(parser);

	switch (parser->previous.type) {
		case TOKEN_INT: return (Type) { .type = DATA_TYPE_INT };
		default: 
			error(parser, "Expected type");
			return (Type) { .type = DATA_TYPE_VOID };
	}
}

Node* lookup_variable(Parser* parser, Token name) {
	for(int i = 0; i < parser->currentFunction->function.argumentCount; i++) {
		Token argName = parser->currentFunction->function.arguments[i]->variable.name;
		if(name.length == argName.length && memcmp(name.start, argName.start, name.length) == 0) {
			return parser->currentFunction->function.arguments[i];
		}
	}

	for(int i = 0; i < parser->currentFunction->function.variableCount; i++) {
		Token varName = parser->currentFunction->function.variables[i]->variable.name;
		if(name.length == varName.length && memcmp(name.start, varName.start, name.length) == 0) {
			return parser->currentFunction->function.variables[i];
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

Node* parse_identifier(Parser* parser) {
	Token name = parser->previous;
	Node* variable = lookup_variable(parser, name);

	if(variable == NULL) {
		error(parser, "Unknown variable in current scope");
		return NULL;
	}

	if(match(parser, TOKEN_EQ)) {
		if(variable->type != AST_DEFINE_VAR) {
			error(parser, "Can only assign to variables");
			return NULL;
		}

		Node* value = parse_expression(parser);

		Node* assign = new_node(AST_ASSIGN_VAR);
		assign->variable.name = name;
		assign->variable.stackOffset = variable->variable.stackOffset;
		assign->variable.type = variable->variable.type;
		assign->variable.value = value;
		return assign;
	}
	else if(match(parser, TOKEN_LEFT_PAREN)) {
		Node* call = new_node(AST_CALL);
		call->function.name = name;
		if(!check(parser, TOKEN_RIGHT_PAREN)) {
			do {
				Node* arg = parse_expression(parser);

				call->function.arguments = realloc(call->function.arguments, ++call->function.argumentCount);
				call->function.arguments[call->function.argumentCount - 1] = arg;
			} while(match(parser, TOKEN_COMMA));
		}

		consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function arguments");

		if(call->function.argumentCount != variable->function.argumentCount) {
			//TODO: move this to typechecking
			error(parser, "Argument count mismatch");
		}
		
		return call;
	}

	if(variable->type != AST_DEFINE_VAR) {
		error(parser, "Can only access variables");
		return NULL;
	}

	Node* access = new_node(AST_ACCESS_VAR);
	access->variable.name = name;
	access->variable.stackOffset = variable->variable.stackOffset;
	access->variable.type = variable->variable.type;
	return access;
}

Node* parse_value(Parser* parser) {
	if(match(parser, TOKEN_INT_LITERAL)) {
		Token literal = parser->previous;
		if(parser->error) return NULL;

		Node* literalNode = new_node(AST_INT_LITERAL);
		literalNode->literal.type = (Type) { .type = DATA_TYPE_INT };
		literalNode->literal.as.integer = (int)strtol(literal.start, NULL, 10);
		
		return literalNode;
	}
	else if(match(parser, TOKEN_IDENTIFIER)) {
		return parse_identifier(parser);
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

Node* parse_unary(Parser* parser) {
	if(match(parser, TOKEN_TILDE)) {
		Node* expr = parse_unary(parser);
		Node* bwnot = new_node(OP_BWNOT);
		bwnot->unary = expr;
		return bwnot;
	}
	else if(match(parser, TOKEN_MINUS)) {
		Node* expr = parse_unary(parser);
		Node* neg = new_node(OP_NEG);
		neg->unary = expr;
		return neg;
	}
	else if(match(parser, TOKEN_BANG)) {
		Node* expr = parse_unary(parser);
		Node* not = new_node(OP_NOT);
		not->unary = expr;
		return not;
	}

	return parse_value(parser);
}

Node* parse_term(Parser* parser) {
	Node* left = parse_unary(parser);

	while(match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
		TokenType op = parser->previous.type;

		Node* right = parse_unary(parser);

		NodeType type;
		switch(op) {
			case TOKEN_STAR: type = OP_MUL; break;
			case TOKEN_SLASH: type = OP_DIV; break;
			case TOKEN_PERCENT: type = OP_MOD; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_arithmetic(Parser* parser) {
	Node* left = parse_term(parser);

	while(match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
		TokenType op = parser->previous.type;

		Node* right = parse_term(parser);

		NodeType type;
		switch(op) {
			case TOKEN_PLUS: type = OP_ADD; break;
			case TOKEN_MINUS: type = OP_SUB; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_shift(Parser* parser) {
	Node* left = parse_arithmetic(parser);

	while(match(parser, TOKEN_LSH) || match(parser, TOKEN_RSH)) {
		TokenType op = parser->previous.type;

		Node* right = parse_arithmetic(parser);

		NodeType type;
		switch(op) {
			case TOKEN_LSH: type = OP_LSH; break;
			case TOKEN_RSH: type = OP_RSH; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type);
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
		TokenType op = parser->previous.type;

		Node* right = parse_shift(parser);

		NodeType type;
		switch(op) {
			case TOKEN_LESS: type = OP_LESS; break;
			case TOKEN_LEQ: type = OP_LESS_EQ; break;
			case TOKEN_GREATER: type = OP_GREATER; break;
			case TOKEN_GEQ: type = OP_GREATER_EQ; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_equality(Parser* parser) {
	Node* left = parse_comparison(parser);

	while(match(parser, TOKEN_EQEQ) || match(parser, TOKEN_BANG_EQ)) {
		TokenType op = parser->previous.type;

		Node* right = parse_comparison(parser);

		NodeType type;
		switch(op) {
			case TOKEN_EQEQ: type = OP_EQUAL; break;
			case TOKEN_BANG_EQ: type = OP_NOT_EQUAL; break;
			default: break; // Unreachable
		}

		Node* binary = new_node(type);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_bwand(Parser* parser) {
	Node* left = parse_equality(parser);

	while(match(parser, TOKEN_AMP)) {
		Node* right = parse_equality(parser);

		Node* binary = new_node(OP_BWAND);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_xor(Parser* parser) {
	Node* left = parse_bwand(parser);

	while(match(parser, TOKEN_XOR)) {
		Node* right = parse_bwand(parser);

		Node* binary = new_node(OP_XOR);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_bwor(Parser* parser) {
	Node* left = parse_xor(parser);

	while(match(parser, TOKEN_BAR)) {
		Node* right = parse_xor(parser);

		Node* binary = new_node(OP_BWOR);
		binary->binary.lhs = left;
		binary->binary.rhs = right;
		left = binary;
	}

	return left;
}

Node* parse_expression(Parser* parser) {
	return parse_bwor(parser);
}

Node* parse_return_statement(Parser* parser) {
	Node* returnStmt = new_node(AST_RETURN);
	returnStmt->unary = parse_expression(parser);

	consume(parser, TOKEN_SEMICOLON, "Expected ';' after return statement");

	return returnStmt;
}

Node* parse_var_declaration(Parser* parser) {
	Token name = consume(parser, TOKEN_IDENTIFIER, "Expected variable name");

	// TODO: Type inference
	consume(parser, TOKEN_COLON, "Expected ':' after variable name");

	Type type = parse_type(parser);

	Node* value = NULL;
	if(match(parser, TOKEN_EQ)) {
		value = parse_expression(parser);
	}

	consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");

	Node* var = new_node(AST_DEFINE_VAR);
	var->variable.name = name;
	// TODO: Get type size
	var->variable.stackOffset = parser->currentFunction->function.currentStackOffset += 8;
	var->variable.type = type;
	var->variable.value = value;

	parser->currentFunction->function.variables = realloc(parser->currentFunction->function.variables, ++parser->currentFunction->function.variableCount);
	parser->currentFunction->function.variables[parser->currentFunction->function.variableCount - 1] = var;

	return var;
}

Node* parse_statement(Parser* parser) {
	if(match(parser, TOKEN_RETURN)) {
		return parse_return_statement(parser);
	}
	else if(match(parser, TOKEN_VAR)) {
		return parse_var_declaration(parser);
	}
	else if(match(parser, TOKEN_IDENTIFIER)) {
		Node* expr = parse_identifier(parser);
		
		if(expr == NULL) return NULL;

		// Disallow just having a variable name as a statement
		if(expr->type == AST_ACCESS_VAR) {
			error_current(parser, "Expected statement");
		}

		consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
		
		Node* exprStmt = new_node(AST_EXPR_STMT);
		exprStmt->unary = expr;
		return exprStmt;
	}

	error_current(parser, "Expected statement");

	return NULL;
}

Node* parse_block(Parser* parser) {
	Node* block = new_node(AST_BLOCK);

	while(!check(parser, TOKEN_RIGHT_BRACE)) {
		Node* stmt = parse_statement(parser);
		if(parser->error) break;
		node_add_child(block, stmt);
	}

	consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after block");

	return block;
}

Node* parse_function(Parser* parser) {
	Token name = consume(parser, TOKEN_IDENTIFIER, "Expected function name");

	Node* function = new_node(AST_FUNCTION);
	function->function.name = name;
	function->function.currentStackOffset = 0;
	function->function.variables = NULL;
	function->function.variableCount = 0;
	function->function.arguments = NULL;
	function->function.argumentCount = 0;

	consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name");

	if(!check(parser, TOKEN_RIGHT_PAREN)) {
		do {
			Token argName = consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
			consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
			Type type = parse_type(parser);

			Node* arg = new_node(AST_DEFINE_VAR);
			arg->variable.name = argName;
			arg->variable.type = type;
			//TODO Get size from type
			arg->variable.stackOffset = function->function.currentStackOffset += 8;

			function->function.arguments = realloc(function->function.arguments, ++function->function.argumentCount);
			function->function.arguments[function->function.argumentCount - 1] = arg;
		} while(match(parser, TOKEN_COMMA));
	}

	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function parameters");

	Type type;
	if(match(parser, TOKEN_COLON)) {
		type = match(parser, TOKEN_VOID) ? (Type) { .type = DATA_TYPE_VOID } : parse_type(parser);
	}
	else {
		// Functions return void by default
		type = (Type) {.type = DATA_TYPE_VOID };
	}

	function->function.returnType = type;

	consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before function body");


	parser->currentFunction = function;

	parser->functions = realloc(parser->functions, ++parser->functionCount);
	parser->functions[parser->functionCount - 1] = function;

	Node* body = parse_block(parser);

	function->function.body = body;

	return function;
}

Node* parse_program(Parser* parser) {
	Node* program = new_node(AST_PROGRAM);

	advance(parser);

	while(!match(parser, TOKEN_EOF)) {
		if(match(parser, TOKEN_FUNCTION)) {
			Node* function = parse_function(parser);
			node_add_child(program, function);
		}
		else {
			error_current(parser, "Expected function definition");
			break;
		}
	}

	return program;
}
