#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
// tokenize.c
//
typedef enum {
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // 識別子
  TK_NUM,      // Integer literals
  TK_EOF,      // End-of-file markers
} TokenKind;
// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token length
};
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token* consume_ident();
bool consume_kind(int token_kind);
void expect(char *op);
int expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();
extern char *user_input;
extern Token *token;
//
// parse.c
//
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_ASSIGN, // =
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // Integer
  ND_LVAR,   // ローカル変数
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_FUNCALL, // Function call
} NodeKind;
// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *next;
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side

  // "if" , "while"  or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // Block
  Node* body;

  // Function call
  char *funcname;
  Node *args;

  int val;       // Used if kind == ND_NUM
  char name;
  int offset;
};

// ローカル変数の型
typedef struct LVar LVar;
struct LVar {
  struct LVar *next; // 次の変数かNULL
  char *name; // 変数の名前
  int len;    // 名前の長さ
  int offset; // RBPからのオフセット
};

struct LVar *find_lvar(Token *tok);

typedef struct Function Function;
struct Function{
  Function *next;
  char *name;
  Node *node;
  LVar *locals;
  int stack_size;
};

Function *program();

// codegen.c
//
void codegen(Function *prog);

char *strndup(const char *s, size_t n);