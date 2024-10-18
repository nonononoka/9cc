#include "9cc.h"
#include <stdio.h>

char *user_input;
Token *token;
typedef struct LVar LVar;

char *strndup(const char *s, size_t n){
    char *p;
    size_t n1;

    for (n1 = 0; n1 < n && s[n1] != '\0'; n1++)
        continue;
    p = malloc(n + 1);
    if (p != NULL) {
        memcpy(p, s, n1);
        p[n1] = '\0';
    }
    return p;
}

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}
// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}
// Consumes the current token if it matches `op`.
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待している種類のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_kind(int token_kind)
{
  if (token->kind != token_kind){
    return false;
  }
  token = token->next;
  return true;
}

// Consumes the current token if it is an identifier.
Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

char *expect_ident(){
  if(token->kind != TK_IDENT){
    error_at(token->str, "expected an identifier");
  }
  char *s = strndup(token->str, token->len);
  token = token->next;
  return s;
}
// Ensure that the current token is `op`.
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "expected \"%s\"", op);
  token = token->next;
}
// Ensure that the current token is TK_NUM.
int expect_number() {
  if (token->kind != TK_NUM){
    // fprintf(stderr, "expected_number: %s\n", token->str);
    error_at(token->str, "expected a number");
  }
  int val = token->val;
  token = token->next;
  return val;
}
bool at_eof() {
  return token->kind == TK_EOF;
}
// Create a new token and add it as the next token of `cur`.
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}
bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

char *starts_with_reserved(char *p){
  static char *kw[] = {"return", "if", "else", "while", "for", "int"};

  for (int i = 0; i < sizeof(kw)/sizeof(*kw); i++){
    int len = strlen(kw[i]);
    if(startswith(p, kw[i]) && !is_alnum(p[len])){
      // fprintf(stderr, "yoyakugo: %s\n", kw[i]);
      return kw[i];
    }
  }

  static char *ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops)/sizeof(*ops); i++){
    if(startswith(p, ops[i])){
      return ops[i];
    }
  }

  return NULL;
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize() {
  char *p = user_input;
  // fprintf(stderr, "user_input: %s", p);
  Token head;
  head.next = NULL;
  Token *cur = &head;
  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }
    
    // Keyword or multi-letter punctuator
    char *kw = starts_with_reserved(p);
    if (kw){
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    // Single-letter punctuator
    if (strchr("+-*/()<>;={},&", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
    // Integer literal
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }
    if (is_alpha(*p)){
      char *q = p++;
      while(is_alnum(*p)){
        p++;
      }
      cur = new_token(TK_IDENT, cur, q, p-q);
      continue;
    }
    error_at(p, "invalid token");
  }
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}