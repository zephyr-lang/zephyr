#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "codegen.h"
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

static char* filepath = NULL;
static char* cli_source = NULL;

void usage_exit(FILE* out) {
	fprintf(out, "Possible Arguments:\n");
	fprintf(out, "zephyr <options> <source>\n");
	fprintf(out, "    -c => Provide a source string directly on the CLI\n");
	fprintf(out, "          Cannot specify with a source file\n");
	fprintf(out, "    -h => Print this help message\n");

	exit(1);
}

const char** parse_cli_args(int argc, char const* argv[]) {
	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-c") == 0) {
			if(argc - i == 0) usage_exit(stderr);
			cli_source = (char*)argv[i + 1];
			i++;
		}
		else if(strcmp(argv[i], "-h") == 0) {
			usage_exit(stdout);
		}
		else {
			if(filepath != NULL || cli_source != NULL)
				usage_exit(stderr);
			filepath = (char*)argv[i];
		}
	}
	return &argv[argc];
}

int main(int argc, char const *argv[]) {

	if(argc == 1) {
		usage_exit(stderr);
	}

	parse_cli_args(argc - 1, &argv[1]);

	if(filepath != NULL && cli_source != NULL) {
		fprintf(stderr, "Cannot specify source file and source string\n");
		usage_exit(stderr);
	}

	size_t length;
	char* source = cli_source != NULL ? cli_source : read_file(filepath, &length);

	Lexer lexer = new_lexer(filepath == NULL ? "cli" : filepath, source);

	Parser parser = new_parser(&lexer);

	Node* ast = parse_program(&parser);

	if(parser.error)
		return 1;

	print_ast(ast);

	FILE* out = fopen("./out.yasm", "w");
	generate_program(ast, out);
	fclose(out);

	// Source was read from a file - free it
	// If it's from the CLI do nothing
	if(cli_source == NULL)
		free(source);

	//TODO This isn't very nice
	int status = system("yasm -felf64 out.yasm");

	if(status != 0) {
		fprintf(stderr, "yasm failed (code %d)\n", status);
		return 1;
	}

	status = system("ld out.o");
	if(status != 0) {
		fprintf(stderr, "ld failed (code %d)\n", status);
		return 1;
	}

	return 0;
}