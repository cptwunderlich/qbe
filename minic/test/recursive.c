#include <stdio.h>

foo(int x) {
  int j;

  if (x <= 0)
    return 0;

  for (j=0; j < x; j++) {
    printf("*");
  }

  printf("\n");
  foo(x-1);
}

main() {
  int i;
  i = 8;
  foo(i);
}
