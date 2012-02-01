#include<stdio.h>
#include<unistd.h>
#include "gt_include.h"

int uthread_fn(void * args) {
  long i,j,k;
  for (i=0;i<100000; ++i) {
    for(j=0; j < 1000 ; ++j) {
   //   for (k=0;k <100000;++k) {
   //    ;
   //   }
    }
  }
  printf("Hello world.\n");
}

int uthread_fn2(void * args) {
  long i,j,k;
  for (i=0;i<100000; ++i) {
    for(j=0; j < 1000 ; ++j) {
   //   for (k=0;k <100000;++k) {
   //    ;
   //   }
    }
  }
  printf("Hello all.\n");
}

int main() {
//  uthread_arg_t *args;
  uthread_t utid[10] = {0,1,2,3,4,5,6,7,8,9,10};
  int i = 0;
  gtthread_app_init();
  for (i = 0; i < 20; ++i) {
    uthread_create(&utid[0], uthread_fn, NULL, 0);
  }
  uthread_create(&utid[1], uthread_fn2, NULL, 0);
  gtthread_app_exit();
  return 0;
}
