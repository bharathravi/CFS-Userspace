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
/**********************************************************************/
/** DECLARATIONS **/
/**********************************************************************/


/**********************************************************************/
/* kthread runqueue and env */

/* XXX: should be the apic-id */
#define KTHREAD_CUR_ID	0

/**********************************************************************/
/* uthread scheduling */
static void uthread_context_func(int);
static int uthread_init(uthread_struct_t *u_new);

/********* CFS Accounting ********/
static inline void update_vruntime(uthread_struct_t* uthread);
static inline void update_vruntime_to_max(uthread_struct_t* uthread, cfs_runqueue_t *runqueue);
static inline long get_fair_slice(uthread_struct_t* uthread, cfs_runqueue_t * runqueue);
static inline long get_fair_period(cfs_runqueue_t* runqueue);


/**********************************************************************/
/* uthread creation */
extern int uthread_create(uthread_t *u_tid, int (*u_func)(void *), void *u_arg, uthread_group_t u_gid);

/**********************************************************************/
/** DEFNITIONS **/
/**********************************************************************/

/**********************************************************************/
/* uthread scheduling */

/* Assumes that the caller has disabled vtalrm and sigusr1 signals */
/* uthread_init will be using */
static int uthread_init(uthread_struct_t *u_new)
{
	stack_t oldstack;
	sigset_t set, oldset;
	struct sigaction act, oldact;

	gt_spin_lock(&(ksched_shared_info.uthread_init_lock));

	/* Register a signal(SIGUSR2) for alternate stack */
	act.sa_handler = uthread_context_func;
	act.sa_flags = (SA_ONSTACK | SA_RESTART);
	if(sigaction(SIGUSR2,&act,&oldact))
	{
		fprintf(stderr, "uthread sigusr2 install failed !!");
		return -1;
	}

	/* Install alternate signal stack (for SIGUSR2) */
	if(sigaltstack(&(u_new->uthread_stack), &oldstack))
	{
		fprintf(stderr, "uthread sigaltstack install failed.");
		return -1;
	}

	/* Unblock the signal(SIGUSR2) */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_UNBLOCK, &set, &oldset);


	/* SIGUSR2 handler expects kthread_runq->cur_uthread
	 * to point to the newly created thread. We will temporarily
	 * change cur_uthread, before entering the synchronous call
	 * to SIGUSR2. */

	/* kthread_runq is made to point to this new thread
	 * in the caller. Raise the signal(SIGUSR2) synchronously */
#if 0
	raise(SIGUSR2);
#endif
	syscall(__NR_tkill, kthread_cpu_map[kthread_apic_id()]->tid, SIGUSR2);

	/* Block the signal(SIGUSR2) */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	if(sigaction(SIGUSR2,&oldact,NULL))
	{
		fprintf(stderr, "uthread sigusr2 revert failed !!");
		return -1;
	}

	/* Disable the stack for signal(SIGUSR2) handling */
	u_new->uthread_stack.ss_flags = SS_DISABLE;

	/* Restore the old stack/signal handling */
	if(sigaltstack(&oldstack, NULL))
	{
		fprintf(stderr, "uthread sigaltstack revert failed.");
		return -1;
	}

	gt_spin_unlock(&(ksched_shared_info.uthread_init_lock));
	return 0;
}

extern void uthread_schedule(uthread_struct_t * (*kthread_best_sched_uthread)(kthread_runqueue_t *))
{
	kthread_context_t *k_ctx;
	kthread_runqueue_t *kthread_runq;
	uthread_struct_t *u_obj;
	long fair_slice;
        struct timeval now;
        int i=0;
	kthread_context_t *debug;

	/* Signals used for cpu_thread scheduling */
	// kthread_block_signal(SIGVTALRM);
	// kthread_block_signal(SIGUSR1);
        kthread_set_vtalrm(0);
#if 0
	fprintf(stderr, "uthread_schedule invoked !!\n");
#endif

	k_ctx = kthread_cpu_map[kthread_apic_id()];
	kthread_runq = &(k_ctx->krunqueue);

//        printf("%d entering\n", k_ctx->tid);
	if((u_obj = kthread_runq->cur_uthread))
	{  
		/*Go through the runq and schedule the next thread to run */
		kthread_runq->cur_uthread = NULL;
		
		if(u_obj->uthread_state & (UTHREAD_DONE | UTHREAD_CANCELLED))
		{   
			/* XXX: Inserting uthread into zombie queue is causing improper
			 * cleanup/exit of uthread (core dump) */
//			uthread_head_t * kthread_zhead = &(kthread_runq->zombie_uthreads);
//			gt_spin_lock(&(kthread_runq->kthread_runqlock));
//			kthread_runq->kthread_runqlock.holder = 0x01;
//			TAILQ_INSERT_TAIL(kthread_zhead, u_obj, uthread_runq);
//			gt_spin_unlock(&(kthread_runq->kthread_runqlock));
                        if(u_obj->uthread_tid == 0) {
                          printf("Mster finished\n");
                        }
			{
				ksched_shared_info_t *ksched_info = &ksched_shared_info;	
				gt_spin_lock(&ksched_info->ksched_lock);
				ksched_info->kthread_cur_uthreads--;
				gt_spin_unlock(&ksched_info->ksched_lock);
			}
		} else if (u_obj->uthread_state & UTHREAD_YIELD) {
			update_vruntime_to_max(u_obj, kthread_runq->runqueue);
			u_obj->uthread_state = UTHREAD_RUNNABLE;
			add_to_runqueue(kthread_runq->runqueue, &(kthread_runq->kthread_runqlock), u_obj);
			if(sigsetjmp(u_obj->uthread_env, 0))
				return;

                } else {
                        update_vruntime(u_obj);
			u_obj->uthread_state = UTHREAD_RUNNABLE;
			add_to_runqueue(kthread_runq->runqueue, &(kthread_runq->kthread_runqlock), u_obj);
			/* XXX: Save the context (signal mask not saved) */
			if(sigsetjmp(u_obj->uthread_env, 0))
				return;
		}
	}
//        printf("%d %d still here\n", k_ctx->tid, kthread_runq);
	/* kthread_best_sched_uthread acquires kthread_runqlock. Dont lock it up when calling the function. */
	if(!(u_obj = kthread_best_sched_uthread(kthread_runq)))
	{
		/* Done executing all uthreads. Return to main */
		/* XXX: We can actually get rid of KTHREAD_DONE flag */
		if(ksched_shared_info.app_exit)
		{
			fprintf(stderr, "Quitting kthread (%d) with globals:%d\n", k_ctx->cpuid, ksched_shared_info.kthread_cur_uthreads);
			for (i=0; i <GT_MAX_CORES; ++i) {
			    if(kthread_cpu_map[i]) {
				debug = kthread_cpu_map[i];
                                printf("%d Cur: %d\n", debug->tid, debug->krunqueue.cur_uthread);
                                if (debug->krunqueue.cur_uthread) {
                                  printf("%d Cur: %d\n", debug->tid, debug->krunqueue.cur_uthread->uthread_tid);
                                }
				print_all(debug->krunqueue.runqueue->tree->root->left,debug->krunqueue.runqueue->tree);
                            }
                        }
                        
			k_ctx->kthread_flags |= KTHREAD_DONE;
		}
                 
		siglongjmp(k_ctx->kthread_env, 1);
		return;
	}
//        printf("%d chose %d\n", k_ctx->tid, u_obj->uthread_tid);
       
	kthread_runq->cur_uthread = u_obj;
	if((u_obj->uthread_state == UTHREAD_INIT) && (uthread_init(u_obj)))
	{
		fprintf(stderr, "uthread_init failed on kthread(%d)\n", k_ctx->cpuid);
		exit(0);
	}

	u_obj->uthread_state = UTHREAD_RUNNING;
	
	/* Re-install the scheduling signal handlers */
        //XXX(CFS): At this point, we need to schedule the next thread for as long
        // as its fair slice allows. We calculate the fair slice and set the scheduler
        // to wake up after the thread has run for <fair_slice> amount of time, by
        // setting the VTALRM appropriately
        fair_slice = get_fair_slice(u_obj, kthread_runq->runqueue);

	// Record time of entry to CPU
	gettimeofday(&now, 0);
	u_obj->entry_to_cpu = now;

	// Jump to the selected uthread context
	kthread_set_vtalrm(fair_slice);
	kthread_install_sighandler(SIGVTALRM, k_ctx->kthread_sched_timer);
//        printf("%d jumpting to %d\n", k_ctx->tid, u_obj->uthread_tid);
	siglongjmp(u_obj->uthread_env, 1);

	return;
}


// XXX(CFS): The yield function voluntarily gives up CPU to the thread library.
// This is done by setting the thread state to "UTHREAD_YIELD" and simply
// calling the scheduler, which will take care of the rest.
extern void gt_yield() {
  uthread_struct_t *cur_uthread;
  kthread_runqueue_t *kthread_runq;

  // Get the kthread runqueue on which this thread is running.
  kthread_runq = &(kthread_cpu_map[kthread_apic_id()]->krunqueue);

  // Get the thread object of the thread that is calling this function.
  cur_uthread = kthread_runq->cur_uthread;
  assert(cur_uthread->uthread_state == UTHREAD_RUNNING);

  // Set state to UTHREAD_YIELD
  cur_uthread->uthread_state = UTHREAD_YIELD;
  printf("Yielding\n");
  uthread_schedule(&sched_find_best_uthread);
}


/* For uthreads, we obtain a seperate stack by registering an alternate
 * stack for SIGUSR2 signal. Once the context is saved, we turn this 
 * into a regular stack for uthread (by using SS_DISABLE). */
static void uthread_context_func(int signo)
{
	uthread_struct_t *cur_uthread;
	kthread_runqueue_t *kthread_runq;

	kthread_runq = &(kthread_cpu_map[kthread_apic_id()]->krunqueue);

	printf("..... uthread_context_func .....\n");
	/* kthread->cur_uthread points to newly created uthread */
	if(!sigsetjmp(kthread_runq->cur_uthread->uthread_env,0))
	{
		/* In UTHREAD_INIT : saves the context and returns.
		 * Otherwise, continues execution. */
		/* DONT USE any locks here !! */
		assert(kthread_runq->cur_uthread->uthread_state == UTHREAD_INIT);
		kthread_runq->cur_uthread->uthread_state = UTHREAD_RUNNABLE;
		return;
	}

	/* UTHREAD_RUNNING : siglongjmp was executed. */
	cur_uthread = kthread_runq->cur_uthread;
	assert(cur_uthread->uthread_state == UTHREAD_RUNNING);
	/* Execute the uthread task */
	cur_uthread->uthread_func(cur_uthread->uthread_arg);
	cur_uthread->uthread_state = UTHREAD_DONE;

        printf("%d is done on %d\n", cur_uthread->uthread_tid, kthread_runq);
	uthread_schedule(&sched_find_best_uthread);
	return;
}

/**********************************************************************/
/* uthread creation */

extern kthread_runqueue_t *ksched_find_target(uthread_struct_t *);

extern int uthread_create(uthread_t *u_tid, int (*u_func)(void *), void *u_arg, uthread_group_t u_gid)
{
	kthread_runqueue_t *kthread_runq;
	uthread_struct_t *u_new;

	/* Signals used for cpu_thread scheduling */
	// kthread_block_signal(SIGVTALRM);
	// kthread_block_signal(SIGUSR1);

	/* create a new uthread structure and fill it */
	if(!(u_new = (uthread_struct_t *)MALLOCZ_SAFE(sizeof(uthread_struct_t))))
	{
		fprintf(stderr, "uthread mem alloc failure !!");
		exit(0);
	}

	u_new->uthread_state = UTHREAD_INIT;
	u_new->uthread_gid = u_gid;
	u_new->uthread_func = u_func;
	u_new->uthread_arg = u_arg;
        u_new->vruntime = 0;
        u_new->nice = DEFAULT_NICE_VALUE;

	/* Allocate new stack for uthread */
	u_new->uthread_stack.ss_flags = 0; /* Stack enabled for signal handling */
	if(!(u_new->uthread_stack.ss_sp = (void *)MALLOC_SAFE(UTHREAD_DEFAULT_SSIZE)))
	{
		fprintf(stderr, "uthread stack mem alloc failure !!");
		return -1;
	}
	u_new->uthread_stack.ss_size = UTHREAD_DEFAULT_SSIZE;


	{
		ksched_shared_info_t *ksched_info = &ksched_shared_info;

		gt_spin_lock(&ksched_info->ksched_lock);
		u_new->uthread_tid = ksched_info->kthread_tot_uthreads++;
		ksched_info->kthread_cur_uthreads++;
		gt_spin_unlock(&ksched_info->ksched_lock);
	}

	/* XXX: ksched_find_target should be a function pointer */
	kthread_runq = ksched_find_target(u_new);

	*u_tid = u_new->uthread_tid;
        printf("Adding %d to %d\n", u_new->uthread_tid, kthread_runq);
	/* Queue the uthread for target-cpu. Let target-cpu take care of initialization. */
	add_to_runqueue(kthread_runq->runqueue, &(kthread_runq->kthread_runqlock), u_new);


	/* WARNING : DONOT USE u_new WITHOUT A LOCK, ONCE IT IS ENQUEUED. */

	/* Resume with the old thread (with all signals enabled) */
	// kthread_unblock_signal(SIGVTALRM);
	// kthread_unblock_signal(SIGUSR1);
	return 0;
}


/******* Private functions for CFS accounting **************/
//XXX(CFS) Calculates the amount of time a thread spent of CPU, by taking the difference
// between current time and the time it entered the CPU.
static inline void update_vruntime(uthread_struct_t* uthread) {
  int secs, usecs;
  struct timeval now;

  gettimeofday(&now, 0);

  secs = now.tv_sec - uthread->entry_to_cpu.tv_sec;
  usecs = now.tv_usec - uthread->entry_to_cpu.tv_usec;

  uthread->vruntime += secs*1000000 + usecs;
}


//XXX(CFS): This function updates the vruntime of the thread to the
// (maximum of all threads) + 1, so that it will be inserted at the
// rightmost end of the RB Tree.
static inline void update_vruntime_to_max(uthread_struct_t* uthread, cfs_runqueue_t *runqueue) {
  rb_red_blk_tree *tree = runqueue->tree;
  uthread_struct_t *rightmost = get_max_uthread(runqueue);

  uthread->vruntime = rightmost->vruntime + 1;
}


// XXX(CFS): This function calculates the length of the ideal period.
// By default, it is 20ms (defined in gt_signal.h). But if the number of
// threads is too large, then we dont want the timeslices to get too squished.
// Hence, for a large number of threads, the period is extended as:
//               period = num_threads * Min_granularity
static inline long get_fair_period(cfs_runqueue_t* runqueue) {
  int num_threads = runqueue->num_threads + 1;
  long ideal_period = num_threads * SCHEDULER_MIN_GRANULARITY_USEC;
  if (ideal_period < SCHEDULER_MIN_LATENCY_USEC) {
    return SCHEDULER_MIN_LATENCY_USEC;
  }

  return ideal_period;
}



//XXX(CFS): This function calculates the fair slice of time that
// a thread should get out of the current period. The calculation is typically:
// as follows:
//         fair_slice = period * Nice value/ (number of threads in runqueue)
static inline long get_fair_slice(uthread_struct_t* uthread, cfs_runqueue_t * runqueue) {
  long period = get_fair_period(runqueue);
  int num_threads = runqueue->num_threads + 1;
  int nice_value = uthread->nice;
  return period * nice_value / num_threads;
}



#if 0
/**********************************************************************/
kthread_runqueue_t kthread_runqueue;
kthread_runqueue_t *kthread_runq = &kthread_runqueue;
sigjmp_buf kthread_env;

/* Main Test */
typedef struct uthread_arg
{
	int num1;
	int num2;
	int num3;
	int num4;	
} uthread_arg_t;

#define NUM_THREADS 10
static int func(void *arg);

int main()
{
	uthread_struct_t *uthread;
	uthread_t u_tid;
	uthread_arg_t *uarg;

	int inx;

	/* XXX: Put this lock in kthread_shared_info_t */
	gt_spinlock_init(&uthread_group_penalty_lock);

	/* spin locks are initialized internally */
	kthread_init_runqueue(kthread_runq);

	for(inx=0; inx<NUM_THREADS; inx++)
	{
		uarg = (uthread_arg_t *)MALLOC_SAFE(sizeof(uthread_arg_t));
		uarg->num1 = inx;
		uarg->num2 = 0x33;
		uarg->num3 = 0x55;
		uarg->num4 = 0x77;
		uthread_create(&u_tid, func, uarg, (inx % MAX_UTHREAD_GROUPS));
	}

	kthread_init_vtalrm_timeslice();
	kthread_install_sighandler(SIGVTALRM, kthread_sched_vtalrm_handler);
	if(sigsetjmp(kthread_env, 0) > 0)
	{
		/* XXX: (TODO) : uthread cleanup */
		exit(0);
	}
	
	uthread_schedule(&ksched_priority);
	return(0);
}

static int func(void *arg)
{
	unsigned int count;
#define u_info ((uthread_arg_t *)arg)
	printf("Thread %d created\n", u_info->num1);
	count = 0;
	while(count <= 0xffffff)
	{
		if(!(count % 5000000))
			printf("uthread(%d) => count : %d\n", u_info->num1, count);
		count++;
	}
#undef u_info
	return 0;
}
#endif
