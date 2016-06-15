#include <stdio.h>

foo(int a, int b, int c, int d) {
  int i;
  int j;

  gets(&i);
  i = i + a + b + 1;
  j = c + d +2;
  printf("%d %d %d %d %d %d", i, j, a, b, c, d);
}

main() {
  foo(1, 2, 3, 4);
}
