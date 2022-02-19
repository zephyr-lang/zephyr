#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lexer.h"
#include "parser.h"

char* read_file(const char* filename, size_t* size) {
	FILE* file;
	file = fopen(filename, "rb");

	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\"\n", filename);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);

	size_t fileSize = ftell(file);
	rewind(file);

	char* charBuffer = (char*)malloc(fileSize + 1);
	if (charBuffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\"\n", filename);
		exit(1);
	}
	size_t bytesRead = fread(charBuffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\"\n", filename);
		exit(1);
	}

	charBuffer[bytesRead] = '\0';

	fclose(file);

	if(size != NULL)
		*size = fileSize;

	return charBuffer;
}

int main(int argc, char const *argv[]) {
	size_t length;
	char* source = read_file("./examples/return-0.zpr", &length);

	Lexer lexer = new_lexer("./examples/return-0.zpr", source);

	/*Token token;
	while((token = lexer_next(&lexer)).type != TOKEN_EOF) {
		print_token(&token);
		printf("\n");
	}*/

	Parser parser = new_parser(&lexer);

	Node* ast = parse_program(&parser);

	if(!parser.error)
		print_ast(ast);

	free(source);

	return 0;
}