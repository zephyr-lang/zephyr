#include "parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

Node* parse_expression(Parser* parser);

Parser new_parser(Lexer* lexer) {
	Parser parser = {0};
	parser.lexer = lexer;
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
		case TOKEN_VOID: return (Type) { .type = DATA_TYPE_VOID };
		default: 
			error(parser, "Expected type");
			return (Type) { .type = DATA_TYPE_VOID };
	}
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

Node* parse_expression(Parser* parser) {
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

Node* parse_return_statement(Parser* parser) {
	Node* returnStmt = new_node(AST_RETURN);
	returnStmt->unary = parse_expression(parser);

	consume(parser, TOKEN_SEMICOLON, "Expected ';' after return statement");

	return returnStmt;
}

Node* parse_statement(Parser* parser) {
	if(match(parser, TOKEN_RETURN)) {
		return parse_return_statement(parser);
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

	consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name");
	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after function parameters");

	Type type;
	if(match(parser, TOKEN_COLON)) {
		type = parse_type(parser);
	}
	else {
		// Functions return void by default
		type = (Type) {.type = DATA_TYPE_VOID };
	}

	consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before function body");

	Node* body = parse_block(parser);

	Node* function = new_node(AST_FUNCTION);
	function->function.name = name;
	function->function.returnType = type;
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
			error(parser, "Expected function definition");
			break;
		}
	}

	return program;
}
