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

static Node* new_node(NodeType type, Token position) {
	Node* node = calloc(1, sizeof(Node));
	node->type = type;
	node->position = position;
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

Node* parse_identifier(Parser* parser) {
	Token name = parser->previous;

	if(match(parser, TOKEN_EQ)) {
		Node* value = parse_expression(parser);

		Node* assign = new_node(AST_ASSIGN_VAR, name);
		assign->variable.name = name;
		assign->variable.value = value;
		return assign;
	}
	else if(match(parser, TOKEN_LEFT_PAREN)) {
		Node* call = new_node(AST_CALL, name);
		call->function.name = name;
		if(!check(parser, TOKEN_RIGHT_PAREN)) {
			do {
				Node* arg = parse_expression(parser);

				call->function.arguments = realloc(call->function.arguments, ++call->function.argumentCount);
				call->function.arguments[call->function.argumentCount - 1] = arg;
			} while(match(parser, TOKEN_COMMA));
		}

		consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function arguments");
		
		return call;
	}

	Node* access = new_node(AST_ACCESS_VAR, name);
	access->variable.name = name;
	return access;
}

Node* parse_value(Parser* parser) {
	if(match(parser, TOKEN_INT_LITERAL)) {
		Token literal = parser->previous;
		if(parser->error) return NULL;

		Node* literalNode = new_node(AST_INT_LITERAL, literal);
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

	return parse_value(parser);
}

Node* parse_term(Parser* parser) {
	Node* left = parse_unary(parser);

	while(match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
		Token op = parser->previous;

		Node* right = parse_unary(parser);

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

Node* parse_expression(Parser* parser) {
	return parse_bwor(parser);
}

Node* parse_return_statement(Parser* parser) {
	Node* returnStmt = new_node(AST_RETURN, parser->previous);
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

	Node* var = new_node(AST_DEFINE_VAR, name);
	var->variable.name = name;
	var->variable.type = type;
	var->variable.value = value;

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
		
		Node* exprStmt = new_node(AST_EXPR_STMT, expr->position);
		exprStmt->unary = expr;
		return exprStmt;
	}

	error_current(parser, "Expected statement");

	return NULL;
}

Node* parse_block(Parser* parser) {
	Node* block = new_node(AST_BLOCK, parser->previous);

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

	Node* function = new_node(AST_FUNCTION, name);
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

			Node* arg = new_node(AST_DEFINE_VAR, argName);
			arg->variable.name = argName;
			arg->variable.type = type;

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
	advance(parser);
	Node* program = new_node(AST_PROGRAM, parser->current);

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
