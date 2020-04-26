#include "9cc.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		error("the number of input is incorrect");
		return 1;
	}

	// tokenize and parse
	user_input = argv[1];
	token = tokenize();
	Node *node = expr();

	// output the first half of the assembly
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// generate code by descending the abstract syntax tree
	gen(node);

	// load the value in the top of the stack into rax
	printf("    pop rax\n");
	printf("    ret\n");
	return 0;
}
