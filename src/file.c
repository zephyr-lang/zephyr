#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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