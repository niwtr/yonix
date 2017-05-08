// 内存swap演示程序

#include "types.h"
#include "user.h"

int main(int argc, char *argv[])
{
    printf(1, "memtest\n");

    char *m1 = (char *)malloc(1024 * 200);
    for (int i = 0; i < 1024 * 100; i++)
        m1[i] = 0;

    char *m2 = (char *)malloc(1024 * 1024 * 30);

    char *m3 = (char *)malloc(1024 * 40);
    for (int i = 0; i < 1024 * 10; i++)
        m1[i] = 0;

    free(m1);
    free(m3);
    free(m2);

    printf(1, "mem ok\n");
    exit();
}