#ifndef __GT_UTHREAD_H
#define __GT_UTHREAD_H
#include <setjmp.h>
#include <signal.h>

#include "rb/red_black_tree.h"

/* User-level thread implementation (using alternate signal stacks) */

typedef unsigned int uthread_t;
typedef unsigned int uthread_group_t;

/* uthread states */
#define UTHREAD_INIT 0x01
#define UTHREAD_RUNNABLE 0x02
#define UTHREAD_RUNNING 0x04
#define UTHREAD_CANCELLED 0x08
#define UTHREAD_DONE 0x10
#define UTHREAD_YIELD 0x20

#define UTHREAD_DEFAULT_SSIZE (16 * 1024)
/* uthread struct : has all the uthread context info */
typedef struct uthread_struct
{
	
	int uthread_state; /* UTHREAD_INIT, UTHREAD_RUNNABLE, UTHREAD_RUNNING, UTHREAD_CANCELLED, UTHREAD_DONE, UTHREAD_YIELD */
        int nice; //XXX(CFS): In the O(1) scheduler, this was the thread priority. Instead, in CFS, this is the "Nice" value.
	int cpu_id; /* cpu it is currently executing on */
	int last_cpu_id; /* last cpu it was executing on */
	
	uthread_t uthread_tid; /* thread id */
	uthread_group_t uthread_gid; /* thread group id  */
	int (*uthread_func)(void*);
	void *uthread_arg;

        // CFS Scheduling statistics
        struct timeval entry_to_cpu; // Records the time this thread last entered the CPU to run.
        unsigned long vruntime;  // Current vruntime of this thread in microseconds
        rb_red_blk_node* node;  // This thread's position in the RB Tree runqueue

	void *exit_status; /* exit status */
	int reserved1;
	int reserved2;
	int reserved3;
	
	sigjmp_buf uthread_env; /* 156 bytes : save user-level thread context*/
	stack_t uthread_stack; /* 12 bytes : user-level thread stack */
	// TAILQ_ENTRY(uthread_struct) uthread_runq;
} uthread_struct_t;

struct __kthread_runqueue;
extern void uthread_schedule(uthread_struct_t * (*kthread_best_sched_uthread)(struct __kthread_runqueue *));

// XXX(CFS): A user level "yield" function, that voluntarily gives up the 
// CPU to the underlying thread library.
extern void gt_yield();

/******* Private functions *********/
//static inline  long update_vruntime(uthread_struct_t* uthread);
//static inline long get_fair_slice(uthread_struct_t* uthread, cfs_runqueue_t * runqueue);
//static inline long get_fair_period(cfs_runqueue_t* runqueue);
#endif
