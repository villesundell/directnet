#include <stdio.h>

int main()
{
    printf("key%c%cTestMan;", 1, 1);
    fflush(stdout);
    system("gpg --homedir ~/.dntest -a --export Testman --no-tty");
    putchar('\0');
}
