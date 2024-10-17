#include "9cc.h"

int labelseq = 0;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
char *funcname;

void gen_lval(Node *node){
  if(node->kind != ND_LVAR){
     error("代入の左辺値が変数ではありません");
  }
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN: // 代入式だけはちょっと特別に扱う
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  case ND_IF:{
    int seq = labelseq++;
    if(node->els){
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lelse%d\n", seq);
      gen(node->then);
      printf("  jmp .Lend%d\n", seq);
      printf(".Lelse%d:\n", seq);
      gen(node->els);
      printf(".Lend%d:\n", seq);
    } else{
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
      gen(node->then);
      printf(".Lend%d:\n", seq);
    }
    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;
    printf(".Lbegin%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", seq);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    if(node->init){
      gen(node->init); // ここで変数の宣言
    }
    printf(".Lbegin%d:\n", seq);
    if(node->cond){
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
    }
    gen(node->then);
    if(node->inc){
      gen(node->inc);
    }
    printf("  jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case ND_BLOCK: {
    for (Node *n = node->body; n ; n = n->next){
      gen(n);
    }
    return;
  }
  case ND_FUNCALL:{
    int nargs = 0;
    for(Node* arg = node->args; arg; arg = arg->next){
      gen(arg);
      nargs++;
    }

    for (int i = nargs -1; i >= 0; i--){
      printf("  pop %s\n", argreg[i]);
    }
    int seq = labelseq++;
    printf("  mov rax, rsp\n"); // rax = rsp
    printf("  and rax, 15\n"); // raxの下位4bitのみ残す
    printf("  jnz .Lcall%d\n", seq); // raxが0じゃないなら、.Lcallラベルにジャンプ
    printf("  mov rax, 0\n"); // アラインメントがあってるなら、rax を0にセットし関数を呼び出す
    printf("  call %s\n", node->funcname);
    printf("  jmp .Lend%d\n", seq); // 関数呼び出し後の操作に移行
    printf(".Lcall%d:\n", seq); // raxが0じゃないなら
    printf("  sub rsp, 8\n"); //rspを8バイト減少させてスタックを下に伸ばす. rspは常に8の倍数だから8を引けば16の倍数になる
    printf("  mov rax, 0\n"); // raxを0にセットして
    printf("  call %s\n", node->funcname); // 関数呼び出し
    printf("  add rsp, 8\n"); // スタックポインタを元に戻す
    printf(".Lend%d:\n", seq);
    printf("  push rax\n");
    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }
  gen(node->lhs);
  gen(node->rhs);
  printf("  pop rdi\n");
  printf("  pop rax\n");
  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }
  printf("  push rax\n");
}

void codegen(Function *prog){
  printf(".intel_syntax noprefix\n");

  for (Function *fn = prog; fn; fn = fn->next){
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);

    funcname = fn->name;
    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);
    // Emit code
    for (Node *node = fn->node; node; node = node->next)
      gen(node);
    // Epilogue
    printf(".Lreturn.%s:\n", funcname);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}
