#include <stdio.h>

int main()
{
    printf("fnd%c%cBlah01;SuperMan;;", 1, 1);
    fflush(stdout);
    system("gpg --homedir ~/.dntest -a --export Testman --no-tty");
    putchar('\0');
}
