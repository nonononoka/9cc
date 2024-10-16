#include "9cc.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  // Tokenize and parse.
  user_input = argv[1];
  // fprintf(stderr, "user_input: %s", user_input);
  token = tokenize();

  Token* cur = token;
  while (cur){
    cur = cur->next;
  }
  // fprintf(stderr, "done token");
  program();
  // fprintf(stderr, "done program");

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // プロローグ
  // 変数26個分
  printf("  push rbp\n");     // 今のrbpの値をpushする
  printf("  mov rbp, rsp\n"); // rbp = rsp
  if(locals){
  printf("  sub rsp, %d\n", locals->offset);
  }
  for (int i = 0; code[i]; i++)
  {
    gen(code[i]);

    // 式の評価結果としてスタックに一つの値が残っている
    // はずなので、スタックが溢れないようにポップしておく
    printf("  pop rax\n");
  }
  // エピローグ
  printf("  mov rsp, rbp\n"); // rsp = rbp
  printf("  pop rbp\n"); // 値をpopしてrbpに代入.
  printf("  ret\n");
  return 0;
}