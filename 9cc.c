#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    char *p = argv[1];

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");
    // strtolはlong型に変換する. pは変換対象の文字列ポインタ
    // &pは次に解析する位置. 10は10新数.
    // 数値を読み込んだ後、第二引数のポインタを
    // updateとして読み込んだ文字の次の文字を指すように更新する.
    printf("  mov rax, %ld\n", strtol(p, &p, 10));

    while (*p)
    {
        if (*p == '+')
        {
            p++;
            printf("  add rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        if (*p == '-')
        {
            p++;
            printf("  sub rax, %ld\n", strtol(p, &p, 10));
            continue;
        }
        fprintf(stderr, "予期しない文字です: '%c'\n", *p);
        return 1;
    }
    printf("  ret\n");
    return 0;
}