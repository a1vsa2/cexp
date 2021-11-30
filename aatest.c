#include <stdio.h>
#include <stdlib.h>


typedef struct OPT {
    char c;
} S_OPT;

typedef struct INFO {
    S_OPT *opts;
    int n;
} S_INFO;

int main() {
    long long c = 0x123456L;
    printf("%p\n", &c);
    printf("%p\n", (char*)(&c));
    printf("%d\n", *((char*)(&c)));
    printf("%d\n", *((short*)(&c)));
    printf("num: %I64d\n", sizeof(long long));
    S_INFO *inf = (S_INFO*)malloc(sizeof(S_INFO));
    S_OPT myopts[] = {{1}, {2}};
    inf->opts = myopts;
    S_OPT *ops = inf->opts;
    printf("op: %d", ops[1].c);

}