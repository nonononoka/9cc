#include "9cc.h"
VarList *locals;

LVar *find_lvar(Token *tok){
  for (VarList *vl = locals; vl; vl = vl->next){
    LVar *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  }
  return NULL;
}

// 文脈自由文法に沿ってASTを生成する
Node *new_node(NodeKind kind)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_unary(NodeKind kind, Node *expr)
{
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

Node *new_num(int val)
{
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

Node *new_var(LVar* var){
  Node *node = new_node(ND_LVAR);
  node->var = var;
  return node;
}

LVar *push_var(char *name){
  LVar* lvar = calloc(1, sizeof(struct LVar));
  lvar->name = name;

  VarList *vl = calloc(1, sizeof(VarList)); // これはどっちかっていうとVarListNodeかも
  vl->var = lvar;
  vl->next = locals; // すでにあるlocalsに繋げる
  locals = vl;
  return lvar;
}

VarList *read_func_params(){
  if (consume(")")){
    return NULL;
  }

  VarList* head = calloc(1, sizeof(VarList));
  head->var = push_var(expect_ident()); // 関数の引数もlocal変数と同じように扱う.
  VarList *cur = head;

  while(!consume(")")){
    expect(",");
    cur->next = calloc(1, sizeof(VarList));
    cur->next->var = push_var(expect_ident());
    cur = cur->next;
  }

  return head;
}

Function *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program = function*
Function* program()
{
  Function head;
  head.next = NULL;
  Function *cur = &head;

  while(!at_eof()){
    cur->next = function();
    cur = cur->next;
  }
  return head.next;
}

// function = ident "(" ")" "{" stmt* "}"
Function *function(){
  locals = NULL; // 一旦NULLに戻す.

  Function *fn = calloc(1, sizeof(Function));
  fn->name = expect_ident();
  expect("(");
  fn->params = read_func_params(); // 引数を読み取る
  expect("{");

  Node head;
  head.next = NULL;
  Node *cur = &head;
  while(!consume("}")){
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

// ;で終わるかたまり
Node *stmt()
{
  Node *node;
  if (consume("return"))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    expect(";");
    return node;
  }
  if (consume("if")){
    Node * node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if(consume("else")){
      node->els = stmt();
    }
    return node;
  }
  if(consume("while")){
    Node * node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }
  if(consume("for")){
    Node *node = new_node(ND_FOR);
    expect("(");
    if(!consume(";")){
      node->init = expr();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if(!consume(")")){
      node->inc = expr();
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  if(consume("{")){
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while(!consume("}")){
      cur->next = stmt();
      cur = cur->next;
    }
    Node *node = new_node(ND_BLOCK);
    node->body = head.next;
    return node;
  }

  node = expr();
  if (!consume(";")){
    error_at(token->str, "';'ではないトークンです");
  }
  return node;
}

Node *expr()
{
  return assign();
}

Node *assign()
{
  Node *node = equality();
  if (consume("="))
  {
    // 左に代入される値で右に代入する値
    node = new_binary(ND_ASSIGN, node, assign());
  }
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality()
{
  Node *node = relational();
  for (;;)
  {
    if (consume("=="))
      node = new_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational()
{
  Node *node = add();
  for (;;)
  {
    if (consume("<"))
      node = new_binary(ND_LT, node, add());
    else if (consume("<="))
      node = new_binary(ND_LE, node, add());
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else
      return node;
  }
}
// add = mul ("+" mul | "-" mul)*
Node *add()
{
  Node *node = mul();
  for (;;)
  {
    if (consume("+"))
      node = new_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}
// mul = unary ("*" unary | "/" unary)*
Node *mul()
{
  Node *node = unary();
  for (;;)
  {
    if (consume("*"))
      node = new_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}
// unary = ("+" | "-")? unary
//       | primary
Node *unary()
{
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}

// func-args = "(" (assign ("," assign)*)? ")"
Node *func_args(){
  if(consume(")")){
    return NULL;
  }
  Node *head = assign();
  Node *cur = head;
  while(consume(",")){
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}

// primary = "(" expr ")" | ident func-args? | num
Node *primary()
{
  if (consume("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }
  Token *tok = consume_ident();
  if (tok)
  {
    // 関数の引数呼び出し
    if(consume("(")){
      Node *node = new_node(ND_FUNCALL);
      node->funcname = strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }
    // ただのidentifier
    struct LVar *lvar = find_lvar(tok);
    if (!lvar)
      lvar = push_var(strndup(tok->str, tok->len));
    return new_var(lvar);
  }
  return new_num(expect_number());
}
