#include <stdio.h>

doRead() {
  int a;
  int b;
  int buffer;
  a = 1;
  b = 3;
  gets(&buffer);
  a = b * buffer;
}

main () {
  doRead();
}

