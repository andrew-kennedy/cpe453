#include<string.h>
#include<stdlib.h>
#include<stdio.h>
int main(int argc, char *argv[]) {

    int i, j, k;
    i = j = k = 0;
    while (i++ < 1000000) {
      void *ptr = malloc(127);
      realloc(ptr, 0);
    }


}
