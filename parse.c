#include "9cc.h"

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
bool consume(char *op) {
	if (token->kind != TK_RESERVED ||
	    strlen(op) != token->len ||
	    memcmp(token->str, op, token->len)) {
		return false;
	}
	token = token->next;
	return true;
}

// If the next token is a variable, this returns true and advances
// the token one ahead. Otherwise it returns false
Token *consume_ident(void) {
	if (token->kind != TK_IDENT) {
		return NULL;
	}
	Token *c = token;
	token = token->next;
	return c;
}

// If the next token is the expected one, this function advances the token one ahead.
// Otherwise it reports an error. 
void expect(char *op) {
	if (token->kind != TK_RESERVED ||
	    strlen(op) != token->len ||
	    memcmp(token->str, op, token->len)) {
		error_at(token->str, "It is not \"%s\"", op);
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

bool startswith(char *p, char *q) {
	return memcmp(p, q, strlen(q)) == 0;
}

LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
			return var;
		}
	}
	return NULL;
}

// Create a new token and configure it next to cur.
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

// Tokenize the input string and returns it.
Token *tokenize(void) {
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// skip whitespaces
		if (isspace(*p)) {
			p++;
			continue;
		}

		// semicolon
		if (startswith(p, ";")) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		// multi letter punctuator
		if (startswith(p, "==") || startswith(p, "!=") ||
		    startswith(p, "<=") || startswith(p, ">=")) {
				cur = new_token(TK_RESERVED, cur, p, 2);
				p += 2;
				continue;
		}

		// single letter punctuator
		if (strchr("+-*/()<>=", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		// variables
		if ('a' <= *p && *p <= 'z') {

			cur = new_token(TK_IDENT, cur, p++, 1);
			continue;
		}

		// numbers
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}

		error_at(cur->str+1, "Cannot tokenize");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}
