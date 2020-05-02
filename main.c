#include "9cc.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		error("the number of input is incorrect");
		return 1;
	}

	// tokenize and parse, and its output is recorded in code
	user_input = argv[1];
	token = tokenize();
	program();

	// output the first half of the assembly
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// prologue: allocate the area for 26 variables
	printf("    push rbp\n");
	printf("    mov rbp, rsp\n");
	printf("    sub rsp, 208\n");

	// generate code by descending the abstract syntax tree
	for (int i = 0; code[i]; i++) {
		gen(code[i]);

		// pop the resulting value from the stack
		printf("    pop rax\n");
	}

	// epilogue
	printf("    mov rsp, rbp\n");
	printf("    pop rbp\n");
	printf("    ret\n");
	return 0;
}
