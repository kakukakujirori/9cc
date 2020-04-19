#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// sort of token
typedef enum {
	TK_RESERVED, // symbol
	TK_NUM,      // number token
	TK_EOF,      // end of input token
} TokenKind;

typedef struct Token Token;

// token structure
struct Token {
	TokenKind kind; // type of token
	Token *next;    // next input token
	int val;        // when kind==TK_NUM, its number
	char *str;      // token string
};

// current token
Token *token;

// error reporting function
// it takes the same input as printf
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// If the next token is the expected one, this function returns true
// and advances the token one ahead. Otherwise it returns false.
bool consume(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op) {
		return false;
	}
	token = token->next;
	return true;
}

// If the next token is the expected one, this function advances the token one ahead.
// Otherwise it reports an error. 
void expect(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op) {
		error("It is not '%c'\n", op);
	}
	token = token->next;
}

// If the next token is a number, this function returns the number and
// advances the token one ahead. Otherwise it reports an error.
int expect_number() {
	if (token->kind != TK_NUM) {
		error("It is not a number\n");
	}
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// Create a new token and configure it next to cur.
Token *new_token(TokenKind kind, Token *cur, char *str) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}

// Tokenize the input string and returns it.
Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// skip whitespaces
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p == '+' || *p == '-') {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error("Cannot tokenize");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		error("the number of input is incorrect");
		return 1;
	}

	// tokenize
	token = tokenize(argv[1]);

	// output the first half of the assembly
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// Since the first character must be a number, check it
	// and output the first mov command.
	printf("    mov rax, %d\n", expect_number());

	// Consuming the array of token such as '+<number>' and '-<number>',
	// output the assembly
	while (!at_eof()) {
		if (consume('+')) {
			printf("    add rax, %d\n", expect_number());
			continue;
		}

		expect('-');
		printf("    sub rax, %d\n", expect_number());
	}

	printf("    ret\n");
	return 0;
}

			
