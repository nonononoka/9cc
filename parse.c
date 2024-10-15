#include "9cc.h"
#include <stdio.h>

// 文脈自由文法に沿ってASTを生成する
Node *code[100];
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

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

void program()
{
  int i = 0;
  while (!at_eof())
  {
    code[i++] = stmt();
  }
  code[i] = NULL;
}

Node *stmt()
{
  Node *node;
  if (consume_kind(TK_RETURN))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
  }
  else
  {
    node = expr();
  }
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
// primary = "(" expr ")" | num
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
    // ローカル変数のベースポインタからのオフセット
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    struct LVar *lvar = find_lvar(tok);
    if (lvar)
    {
      node->offset = lvar->offset;
    }
    else
    {
      // 新たな変数の場合、新しいLVarを作って、新たなオフセットをセットして、そのオフセットを使う.
      lvar = calloc(1, sizeof(struct LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      if (locals)
      {
        lvar->offset = locals->offset + 8;
      }
      else
      {
        lvar->offset = 8;
      }
      node->offset = lvar->offset;
      locals = lvar;
    }
    return node;
  }
  return new_num(expect_number());
}
