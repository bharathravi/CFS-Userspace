#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "gt_include.h"


//#define ROWS 512
//#define COLS ROWS
//#define SIZE COLS

//#define NUM_CPUS 10
//#define NUM_GROUPS NUM_CPUS
//#define PER_GROUP_COLS (SIZE/NUM_GROUPS)

#define NUM_THREADS 32 // Number of threads per matrix
//#define PER_THREAD_ROWS (SIZE/NUM_THREADS)
#define MAXSIZE 512

/* A[SIZE][SIZE] X B[SIZE][SIZE] = C[SIZE][SIZE]
 * Let T(g, t) be thread 't' in group 'g'. 
 * T(g, t) is responsible for multiplication : 
 * A(rows)[(t-1)*SIZE -> (t*SIZE - 1)] X B(cols)[(g-1)*SIZE -> (g*SIZE - 1)] */

typedef struct matrix
{
	int m[MAXSIZE][MAXSIZE];

	int rows;
	int cols;
	unsigned int reserved[2];
} matrix_t;


typedef struct __uthread_arg
{
	matrix_t *_A, *_B, *_C;
	unsigned int reserved0;

	unsigned int tid;
	unsigned int gid;
	int start_row; /* start_row -> (start_row + PER_THREAD_ROWS) */
	int start_col; /* start_col -> (start_col + PER_GROUP_COLS) */
	int matrix_size;	
}uthread_arg_t;
	
struct timeval tv1;

static void generate_matrix(matrix_t *mat, int val, int size)
{
    
	int i,j;
	mat->rows = size;
	mat->cols = size;
	for(i = 0; i < mat->rows;i++)
		for( j = 0; j < mat->cols; j++ )
		{	//printf("%d %d\n", i, j);
			mat->m[i][j] = val;
		}
	return;
}

static void print_matrix(matrix_t *mat, int size)
{
	int i, j;

	for(i=0;i<size;i++)
	{
		for(j=0;j<size;j++)
			printf(" %d ",mat->m[i][j]);
		printf("\n");
	}

	return;
}

static void * uthread_mulmat(void *p)
{
	int i, j, k;
	int start_row, end_row;
	int start_col, end_col;
	unsigned int cpuid;
	struct timeval tv2;

#define ptr ((uthread_arg_t *)p)

	i=0; j= 0; k=0;

	start_row = ptr->start_row;
	end_row = (ptr->start_row + ptr->matrix_size/NUM_THREADS);

#ifdef GT_GROUP_SPLIT
	start_col = ptr->start_col;
	end_col = (ptr->start_col + ptr->matrix_size/NUM_THREADS);
#else
	start_col = 0;
	end_col = ptr->matrix_size;
#endif

#ifdef GT_THREADS
	cpuid = kthread_cpu_map[kthread_apic_id()]->cpuid;
#else
#endif

	for(i = start_row; i < end_row; i++)
		for(j = start_col; j < end_col; j++)
			for(k = 0; k < ptr->matrix_size; k++) {
				ptr->_C->m[i][j] += ptr->_A->m[i][k] * ptr->_B->m[k][j];
                        }

#ifdef GT_THREADS
#else
	gettimeofday(&tv2,NULL);
#endif

#undef ptr
	return 0;
}


// Four matrices, of sizes 32, 64, 128, and 256
matrix_t A[4], B[4], C[4];
int sizes[4] = {32, 64, 128, 256};


static void init_matrices() {
  int i = 0;
  for (i=0; i<4; ++i) {
	generate_matrix(&A[i], 1, sizes[i]);
	generate_matrix(&B[i], 1, sizes[i]);
	generate_matrix(&C[i], 0, sizes[i]);
  }
	return;
}


uthread_arg_t uargs[NUM_THREADS*4];
uthread_t utids[NUM_THREADS*4];

int main()
{
	uthread_arg_t *uarg;
	int inx, i ,j;


	gtthread_app_init();

	init_matrices();
//	print_matrix(&A);
//	print_matrix(&B);
//	print_matrix(&C);
	gettimeofday(&tv1,NULL);
	j = 0;
	for (i=0; i<4; ++i) 
	{
		for(inx=0; inx<NUM_THREADS; inx++)
		{
			uarg = &uargs[inx];
			uarg->_A = &A[i];
			uarg->_B = &B[i];
			uarg->_C = &C[i];

			uarg->tid = inx;

			uarg->gid = i;  // Every matrix in a different group
                        uarg->matrix_size = sizes[i];

			uarg->start_row = (inx * sizes[i]/NUM_THREADS);
#ifdef GT_GROUP_SPLIT
		/* Wanted to split the columns by groups !!! */
			uarg->start_col = (uarg->gid * PER_GROUP_COLS);
#endif

			uthread_create(&utids[j], uthread_mulmat, uarg, uarg->gid);
			j++;
		}
	}
//	for(i=0; i < 100000; ++i) {
//		for(j=0; j<9000; ++j) {
//
//              }
//        }

	gtthread_app_exit();

//	print_matrix(&A);
//	print_matrix(&B);
//	print_matrix(&C);
	// fprintf(stderr, "********************************");
	return(0);
}
