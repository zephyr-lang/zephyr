#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "typecheck.h"

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
static char* outFile = NULL;
static char* cli_source = NULL;
static bool dumpAst = false;

void usage_exit(FILE* out) {
	fprintf(out, "Possible Arguments:\n");
	fprintf(out, "zephyr <options> <source>\n");
	fprintf(out, "    -c => Provide a source string directly on the CLI\n");
	fprintf(out, "          Cannot specify with a source file\n");
	fprintf(out, "    -d => Display an AST dump\n");
	fprintf(out, "    -h => Print this help message\n");
	fprintf(out, "    -o => Specify the output file\n");

	exit(1);
}

const char** parse_cli_args(int argc, char const* argv[]) {
	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-c") == 0) {
			if(argc - i == 0) usage_exit(stderr);
			cli_source = (char*)argv[i + 1];
			i++;
		}
		else if(strcmp(argv[i], "-d") == 0) {
			dumpAst = true;
		}
		else if(strcmp(argv[i], "-h") == 0) {
			usage_exit(stdout);
		}
		else if(strcmp(argv[i], "-o") == 0) {
			if(argc - i == 0) usage_exit(stderr);
			outFile = (char*)argv[i + 1];
			i++;
		}
		else if(strcmp(argv[i], "--") == 0) {
			break;
		}
		else {
			if(filepath != NULL || cli_source != NULL)
				usage_exit(stderr);
			filepath = (char*)argv[i];
		}
	}
	return &argv[argc];
}

int run_command(char** argv) {
	pid_t process = fork();
	if(process < 0) {
		perror("fork");
		return 1;
	}

	if(process == 0) {
		execvp(argv[0], argv);
		perror("execv");
		return 1;
	}

	int status;

 	wait(&status);

	return status;
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

	if(outFile == NULL) {
		outFile = "./a.out";
	}

	size_t length;
	char* source = cli_source != NULL ? cli_source : read_file(filepath, &length);

	Lexer lexer = new_lexer(filepath == NULL ? "cli" : filepath, source);

	Parser parser = new_parser(&lexer);

	Node* ast = parse_program(&parser);

	if(parser.error)
		return 1;

	if(dumpAst)
		print_ast(ast);

	type_check(&parser, ast);

	char asmFile[1024];
	strcpy(asmFile, outFile);
	strcat(asmFile, ".yasm");

	FILE* out = fopen(asmFile, "w");
	generate_program(ast, out);
	fclose(out);

	// Source was read from a file - free it
	// If it's from the CLI do nothing
	if(cli_source == NULL)
		free(source);

	char objectFile[1024];
	strcpy(objectFile, outFile);
	strcat(objectFile, ".o");

	char* yasmArgs[] = { "yasm", "-felf64", "-o", objectFile, asmFile, NULL };

	int status = run_command(yasmArgs);

	if(status != 0) {
		fprintf(stderr, "yasm failed (code %d)\n", status);
		return 1;
	}

	char* ldArgs[] = { "ld", "-o", outFile, objectFile, NULL };

	status = run_command(ldArgs);
	if(status != 0) {
		fprintf(stderr, "ld failed (code %d)\n", status);
		return 1;
	}

	return 0;
}