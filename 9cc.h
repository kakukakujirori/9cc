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
	TK_IDENT,    // identifier
	TK_RETURN,   // return symbol
	TK_IF,       // if symbol
	TK_ELSE,     // else symbol
	TK_WHILE,    // while symbol
	TK_FOR,      // for symbol
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
	int len;        // length of token
};

// kind of nodes of abstract syntax tree
typedef enum {
	ND_ADD,    // +
	ND_SUB,    // -
	ND_MUL,    // *
	ND_DIV,    // /
	ND_ASSIGN, // =
	ND_EQ,     // ==
	ND_NE,     // !=
	ND_LT,     // <
	ND_LE,     // <=
	ND_LVAR,   // local variable
	ND_NUM,    // integer
	ND_RETURN, // return symbol
	ND_IF,     // if symbol
	ND_ELSE,   // else symbol
	ND_WHILE,  // while symbol
	ND_FOR,    // for symbol
	ND_BLOCK,  // block symbol
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind; // node type
	Node *lhs;     // left hand side
	Node *rhs;     // right hand side
	Node *cond;    // condition clause (used for if/while/for sentence)
	Node *then;    // resulting process (used only for if/while/for sentence)
	Node *els;     // else clause (used only for if ... else ... sentence)
	Node *init;    // initialization clause (used only for "for" sentence)
	Node *inc;     // increment clause (used only for "for" sentence)
	Node *next;    // next stmt()
	Node *body;    // Block or statement expression
	int val;       // used only when kind == ND_NUM
	int offset;    // used only when kind == ND_LVAR
};

typedef struct LVar LVar;

struct LVar {
	LVar *next;  // next variable or NULL
	char *name;  // name of variable
	int len;     // length of variable name
	int offset;  // offset from RBP
};

////////////////////////////////////////////////////////////////

/* prototype declaration */
void error(char *fmt, ...) ;
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
bool consume_kind(TokenKind tk);
Token *consume_ident(void);
void expect(char *op);
int expect_number(void);
bool at_eof(void);
bool startswith(char *p, char *q);
LVar *find_lvar(Token *tok);
int is_alnum(char c);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(void);

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
void *program(void);
Node *stmt(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);
void gen_lval(Node *node);
void gen(Node *node);

////////////////////////////////////////////////////////////////

/* global constants */
Token *token; // current token
char *user_input; // input program
Node *code[100];
LVar *locals;
int go_to_number;