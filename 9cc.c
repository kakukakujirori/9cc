#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////

/* structures */
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

// kind of nodes of abstract syntax tree
typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // integer
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind; // node type
	Node *lhs;     // left hand side
	Node *rhs;     // right hand side
	int val;       // used only when kind == ND_NUM
};

////////////////////////////////////////////////////////////////

/* prototype declaration */
void error(char *fmt, ...) ;
void error_at(char *loc, char *fmt, ...);
bool consume(char op);
void expect(char op);
int expect_number(void);
bool at_eof(void);
Token *new_token(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *p);

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *expr(void);
Node *mul(void);
Node *primary(void);
void gen(Node *node);

////////////////////////////////////////////////////////////////

/* global constants */
Token *token; // current token
char *user_input; // input program

////////////////////////////////////////////////////////////////

/* functions */

// error reporting function
// it takes the same input as printf
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, ""); // output pos-times of empty space
	fprintf(stderr, "^ ");
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
		error_at(token->str, "It is not '%c'", op);
	}
	token = token->next;
}

// If the next token is a number, this function returns the number and
// advances the token one ahead. Otherwise it reports an error.
int expect_number(void) {
	if (token->kind != TK_NUM) {
		error_at(token->str, "It is not a number");
	}
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof(void) {
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

		if (strchr("+-*/()", *p)) {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error_at(cur->str+1, "Cannot tokenize");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}



/* Recursive descent parsing */
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

Node *expr(void) {
	Node *node = mul();

	for (;;) { // infinity loop
		if (consume('+')) {
			node = new_node(ND_ADD, node, mul());
		} else if (consume('-')) {
			node = new_node(ND_SUB, node, mul());
		} else {
			return node;
		}
	}
}

Node *mul(void) {
	Node *node = primary();

	for (;;) {
		if (consume('*')) {
			node = new_node(ND_MUL, node, primary());
		} else if (consume('/')) {
			node = new_node(ND_DIV, node, primary());
		} else {
			return node;
		}
	}
}

Node *primary(void) {
	if (consume('(')) {
		Node *node = expr();
		expect(')');
		return node;
	} 
	return new_node_num(expect_number());
}

void gen(Node *node) {
	if (node->kind == ND_NUM) {
		printf("    push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("    pop rdi\n");
	printf("    pop rax\n");

	switch (node->kind) {
		case ND_ADD:
	        printf("    add rax, rdi\n");
		    break;
		case ND_SUB:
            printf("    sub rax, rdi\n");
			break;
		case ND_MUL:
		    printf("    imul rax, rdi\n");
			break;
		case ND_DIV:
		    printf("    cqo\n");
			printf("    idiv rdi\n");
			break;
	}
	printf("    push rax\n");
}

////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
	if (argc != 2) {
		error("the number of input is incorrect");
		return 1;
	}

	// tokenize and parse
	user_input = argv[1];
	token = tokenize(user_input);
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

			
