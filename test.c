#include<stdio.h>
#include<unistd.h>
#include "gt_include.h"

void uthread_fn() {
  long i,j,k;
  for (i=0;i<100000; ++i) {
    for(j=0; j < 5000; ++j) {
//      for (k=0;k <100000;++k) {
//       ;
//      }
    }
  }
  printf("Hello world.\n");
}

static char* itoa(unsigned int n) {
  int size=0, i, j;
  char* c;
  i = n;
  //calculate size of the string
  do {
    size++;
    i = i/10;
  } while(i > 0);

  printf("%d ", size);
  c = malloc(sizeof(char)*(size+4));

  i = n;
  j = 0;
  do {
    c[size - j -1] = i%10 + 48;
    i = i/10;
    j++;
  } while(j < size);

  c[size] = '.';
  c[size + 1] = 't';
  c[size + 2] = 'x';
  c[size + 3] = 't';

  return c;
}


int main() {
  char *c;
  c = itoa(22);
  printf("%s\n", c);
  c = itoa(200);
  printf("%s\n", c);
  c = itoa(1912382);
  printf("%s\n", c);
  c = itoa(0);
  printf("%s\n", c);
  return 0;
}
