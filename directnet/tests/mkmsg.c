#include <stdio.h>

int main()
{
    printf("msg%c%cTestMan\nTestt\n;TestMan;", 1, 1);
    fflush(stdout);
    system("echo 'This is a test' | gpg --homedir ~/.dntest -aes -r TestMan --trust-model always --no-tty");
    putchar('\0');
}
