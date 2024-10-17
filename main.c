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

  // Token* cur = token;
  // while (cur){
  //   fprintf(stderr, "str: %s\n", cur->str);
  //   fprintf(stderr, "%u\n", cur->len);
  //   cur = cur->next;
  // }
  // fprintf(stderr, "done token");
  Program *prog = program();
  
  // それぞれの関数を呼び出す時に必要になるスタックのサイズを確認.
  // local変数の分だけ必要.
  int offset = 0;
  for (LVar *var = prog->locals; var; var = var->next){
    offset += 8;
    var->offset = offset;
  }
  prog->stack_size = offset;
  codegen(prog);
  return 0;
}