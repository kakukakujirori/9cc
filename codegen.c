#include "9cc.h"

LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
			return var;
		}
	}
	return NULL;
}

Function *find_func(Token *tok) {
	for (Function *func = functions; func; func = func->next) {
		if (func->len == tok->len && !memcmp(tok->str, func->name, func->len)) {
			return func;
		}
	}
	return NULL;
}

/* Recursive descent parsing */
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	node->cond = NULL;
	node->then = NULL;
	node->els = NULL;
	node->init = NULL;
	node->inc = NULL;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	node->els = NULL;
	return node;
}

Node *new_node_ident(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;

	// determine the offset of the variable
	LVar *lvar = find_lvar(tok);
	if (lvar) {
		node->offset = lvar->offset;
	} else {
		lvar = calloc(1, sizeof(LVar));
		lvar->next = locals;
		lvar->name = tok->str;
		lvar->len = tok->len;
		lvar->offset = locals->offset + 8;
		node->offset = lvar->offset;
		locals = lvar;
	}
	return node;
}

Node *new_node_func(Token *tok) {
	Node *node = new_node(ND_FUNC, NULL, NULL);
	Function *func = find_func(tok);
	if (func) {
		node->funcname = strndup(tok->str, tok->len);
	} else {
		func = calloc(1, sizeof(Function));
		func->next = functions;
		func->name = tok->str;
		func->len = tok->len;
		node->funcname = strndup(tok->str, tok->len);
		functions = func;
	}
	return node;
}

void *program(void) {
	int i = 0;
	while (!at_eof()) {
		code[i++] = stmt();
	}
	code[i] = NULL;
}

Node *stmt(void) {
	Node *node;
	if (consume_kind(TK_RETURN)) {
		node = new_node(ND_RETURN, expr(), NULL);
		expect(";");
		return node;
	} else if (consume_kind(TK_IF)) {
		expect("(");
		node = new_node(ND_IF, NULL, NULL);
		node->cond = expr();
		expect(")");
		node->then = stmt();
		if (consume_kind(TK_ELSE)) {
			node->els = stmt();
		}
		return node;
	} else if (consume_kind(TK_WHILE)) {
		expect("(");
		node = new_node(ND_WHILE, NULL, NULL);
		node->cond = expr();
		expect(")");
		node->then = stmt();
		return node;
	} else if (consume_kind(TK_FOR)) {
		expect("(");
		node = new_node(ND_FOR, NULL, NULL);
		if (!consume(";")) {
			node->init = expr();
			expect(";");
		}
		if (!consume(";")) {
			node->cond = expr();
			expect(";");
		}
		if (!consume(")")) {
			node->inc = expr();
			expect(")");
		}
		node->then = stmt();
		return node;
	} else if (consume("{")) {
		/* reference: chibicc/codegen.c/l.1266-1280 */
		Node head = {};
		Node *cur = &head;
		while (!consume("}")) {
			cur->next = stmt();
			cur = cur->next;
		}
		Node *node = new_node(ND_BLOCK, NULL, NULL);
		node->body = head.next;
		return node;
	} else {
		node = expr();
		expect(";");
		return node;
	}
}

Node *expr(void) {
	return assign();
}

Node *assign(void) {
	Node *node = equality();
	if (consume("=")) {
		node = new_node(ND_ASSIGN, node, assign());
	}
	return node;
}

Node *equality(void) {
	Node *node = relational();

	for (;;) { // infinity loop
		if (consume("==")) {
			node = new_node(ND_EQ, node, relational());
		} else if (consume("!=")) {
			node = new_node(ND_NE, node, relational());
		} else {
			return node;
		}
	}
}

Node *relational(void) {
	Node *node = add();

	for (;;) {
		if (consume("<")) {
			node = new_node(ND_LT, node, add());
		} else if (consume("<=")) {
			node = new_node(ND_LE, node, add());
		} else if (consume(">")) {
			node = new_node(ND_LT, add(), node);
		} else if (consume(">=")) {
			node = new_node(ND_LE, add(), node);
		} else {
			return node;
		}
	}
}

Node *add(void) {
	Node *node = mul();

	for (;;) {
		if (consume("+")) {
			node = new_node(ND_ADD, node, mul());
		} else if (consume("-")) {
			node = new_node(ND_SUB, node, mul());
		} else {
			return node;
		}
	}
}

Node *mul(void) {
	Node *node = unary();

	for (;;) {
		if (consume("*")) {
			node = new_node(ND_MUL, node, unary());
		} else if (consume("/")) {
			node = new_node(ND_DIV, node, unary());
		} else {
			return node;
		}
	}
}

Node *unary(void) {
	if (consume("+")) {
		return unary(); // in order to admit the expressions such as (+ + 10)
	}
	if (consume("-")) {
		return new_node(ND_SUB, new_node_num(0), unary());
	}
	return primary();
}

Node *primary(void) {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	} 
	Token *tok = consume_ident();
	if (tok && consume("(")) {
		Node *node = new_node_func(tok);
		if (consume(")")) {
			return node;
		} else {
			// refered to https://github.com/kokosabu/9cc/blob/master/parse.c
			node->rhs = calloc(1, sizeof(Node));
			Node *n = node->rhs;
			while (1) {
				n->lhs = expr();
				if (consume(",")) {
					n->rhs = calloc(1, sizeof(Node));
					n->rhs->lhs = NULL;
					n->rhs->rhs = NULL;
					n = n->rhs;
				} else {
					n->rhs = NULL;
					break;
				}
			}
			expect(")");
			return node;
		}
	}
	if (tok) {
		return new_node_ident(tok);
	}
	return new_node_num(expect_number());
}

void gen_lval(Node *node) {
	if (node->kind !=ND_LVAR) {
		error("left value is not a local variable");
	}
	printf("    mov rax, rbp\n");
	printf("    sub rax, %d\n", node->offset);
	printf("    push rax\n");
}

void gen_args(Node *node, int argnum) {
	gen(node->lhs);
	if (node->rhs) gen_args(node->rhs, argnum + 1);

	if (argnum == 1) {
		printf("    pop rax\n");
		printf("    mov rdi, rax\n");
	} else if (argnum == 2) {
		printf("    pop rax\n");
		printf("    mov rsi, rax\n");
	} else if (argnum == 3) {
		printf("    pop rax\n");
		printf("    mov rdx, rax\n");
	} else if (argnum == 4) {
		printf("    pop rax\n");
		printf("    mov rcx, rax\n");
	} else if (argnum == 5) {
		printf("    pop rax\n");
		printf("    mov r8, rax\n");
	} else if (argnum == 6) {
		printf("    pop rax\n");
		printf("    mov r9, rax\n");
	} else {
		/* arguments over 7 remains in the stack */
	}
}

void gen(Node *node) {
	switch (node->kind) {
		case ND_RETURN:
			gen(node->lhs);
			printf("    pop rax\n");
			printf("    mov rsp, rbp\n");
			printf("    pop rbp\n");
			printf("    ret\n");
			return;
		case ND_IF: {
			int current_num = go_to_number++;
			gen(node->cond);
			printf("    pop rax\n");
			printf("    cmp rax, 0\n");
			if (node->els) {
				printf("    je  .L.else.%d\n", current_num);
				gen(node->then);
				printf("    jmp .L.end.%d\n", current_num);
				printf(".L.else.%d:\n", current_num);
				gen(node->els);
			} else {
				printf("    je  .L.end.%d\n", current_num);
				gen(node->then);
			}
			printf(".L.end.%d:\n", current_num);
			return;
		}
		case ND_WHILE: {
			int current_num = go_to_number++;
			printf(".L.begin.%d:\n", current_num);
			gen(node->cond);
			printf("    pop rax\n");
			printf("    cmp rax, 0\n");
			printf("    je  .L.end.%d\n", current_num);
			gen(node->then);
			printf("    jmp .L.begin.%d\n", current_num);
			printf(".L.end.%d:\n", current_num);
			return;
		}
		case ND_FOR: {
			int current_num = go_to_number++;
			if (node->init) gen(node->init);
			printf(".L.begin.%d:\n", current_num);
			if (node->cond) gen(node->cond);
			printf("    pop rax\n");
			printf("    cmp rax, 0\n");
			printf("    je  .L.end.%d\n", current_num);
			gen(node->then);
			if (node->inc) gen(node->inc);
			printf("    jmp .L.begin.%d\n", current_num);
			printf(".L.end.%d:\n", current_num);
			return;
		}
		case ND_BLOCK:
			for (Node *n = node->body; n; n = n->next) gen(n);
			return;
		case ND_FUNC:
			if (node->rhs != NULL) gen_args(node->rhs, 1);
			printf("    call %s\n", node->funcname);
			printf("    push rax\n");
			return;
	    case ND_NUM:
		    printf("    push %d\n", node->val);
		    return;
		case ND_LVAR:
		    gen_lval(node);
			printf("    pop rax\n");
			printf("    mov rax, [rax]\n");
			printf("    push rax\n");
			return;
		case ND_ASSIGN:
		    gen_lval(node->lhs);
			gen(node->rhs);

			printf("    pop rdi\n");
			printf("    pop rax\n");
			printf("    mov [rax], rdi\n");
			printf("    push rdi\n");
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
		case ND_EQ:
            printf("    cmp rax, rdi\n");
			printf("    sete al\n");
			printf("    movzb rax, al\n");
			break;
		case ND_NE:
		    printf("    cmp rax, rdi\n");
			printf("    setne al\n");
			printf("    movzb rax, al\n");
			break;
		case ND_LT:
		    printf("    cmp rax, rdi\n");
			printf("    setl al\n");
			printf("    movzb rax, al\n");
			break;
		case ND_LE:
		    printf("    cmp rax, rdi\n");
			printf("    setle al\n");
			printf("    movzb rax, al\n");
			break;
	}
	printf("    push rax\n");
}
