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

int main() {
//  uthread_arg_t *args;
  uthread_fn();
  return 0;
}
