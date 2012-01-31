#include<stdio.h>
#include<unistd.h>
#include "gt_include.h"

int uthread_fn(void * args) {
  long i,j,k;
  gt_yield();
  for (i=0;i<100000; ++i) {
    for(j=0; j < 5000 ; ++j) {
   //   for (k=0;k <100000;++k) {
   //    ;
   //   }
    }
  }
  printf("Hello world. Yielding\n");
}

int main() {
//  uthread_arg_t *args;
  uthread_t utid[100];
  int i = 0;
  gtthread_app_init();
  for (i = 0; i <2; ++i) {
    uthread_create(&utid[i], uthread_fn, NULL, 0);
  }
  gtthread_app_exit();
  return 0;
}
