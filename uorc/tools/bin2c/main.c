#include <stdio.h>

int main(int argc, char *argv[])
{
    int c;
    int cnt = 0;
    const char *name = (argc > 1) ? argv[1] : "DATA";

    printf("const char %s[] = {\n", name);

    while ((c = getchar()) != EOF) {

        if ((cnt & 15) == 0)
            printf("\t");

        printf("0x%02x,", c);

        if ((cnt & 15) == 15)
            printf("\n");

        cnt++;
    }
    printf("\n");
    printf("};\n\n");
    printf("const int %s_SIZE = %d;\n", name, cnt);

    return 0;
}
