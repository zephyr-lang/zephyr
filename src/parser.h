#pragma once
#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

typedef struct Parser {
	Lexer* lexer;
	Token current;
	Token previous;

	Node* currentFunction;
	Node* currentBlock;

	Node** functions;
	size_t functionCount;

	Node** globalVars;
	size_t globalVarCount;

	Node** strings;
	size_t stringCount;

	bool error;
	bool panic;
} Parser;

Parser new_parser(Lexer* lexer);
Node* parse_program(Parser* parser);
Node* new_node(NodeType type, Token position);
void node_add_child(Node* parent, Node* child);