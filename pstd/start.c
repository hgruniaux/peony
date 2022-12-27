#include <stdlib.h>
#include <stdio.h>

extern int main();

int fib_(int n) {
  if (n == 0) {
    return 0;
  } else if(n == 1) {
    return 1;
  } else {
    return fib_(n - 1) + fib_(n - 2);
  }
}

void
_start()
{
  printf("%d", fib_(15));
  exit(0);
}
