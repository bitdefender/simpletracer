#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Z3Handler.h"

#define MAX_CONTENT 4096

void readfile(char *content, const char *filename) {
	FILE *f = fopen(filename, "r");

	assert(f != NULL);

	unsigned read = fread(content, 1, MAX_CONTENT, f);
	assert(read != 0);

	fclose(f);
}

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Usage: %s logpath\n", argv[0]);
		exit(1);
	}
	Z3Handler zh;

	char content[MAX_CONTENT];
	readfile(content, argv[1]);
	Z3_ast res = zh.toAst(content, strlen(content + 1));
	zh.PrintAst(res);

	return 0;
}
