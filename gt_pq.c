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

/**********************************************************************/
/* Exported runqueue operations */
extern void init_runqueue(cfs_runqueue_t *runqueue)  {
  initialize_cfs_runqueue(runqueue);
}

extern void add_to_runqueue(cfs_runqueue_t *runqueue, gt_spinlock_t *runq_lock, uthread_struct_t *uthread) {
  gt_spin_lock(runq_lock);
  runq_lock->holder = 0x02;
  add_to_cfs_runqueue(runqueue, uthread);
  gt_spin_unlock(runq_lock);
  return;
}

extern void rem_from_runqueue(cfs_runqueue_t *runqueue, gt_spinlock_t *runq_lock, uthread_struct_t *uthread)
{
  gt_spin_lock(runq_lock);
  runq_lock->holder = 0x03;
  remove_from_cfs_runqueue(runqueue, uthread);
  gt_spin_unlock(runq_lock);
  return;
}


/**********************************************************************/

extern void kthread_init_runqueue(kthread_runqueue_t *kthread_runq)
{

  kthread_runq->runqueue = malloc(sizeof(kthread_runqueue_t));
  gt_spinlock_init(&(kthread_runq->kthread_runqlock));
  init_runqueue(kthread_runq->runqueue);

        //TODO: maybe...
  //TAILQ_INIT(&(kthread_runq->zombie_uthreads));
  return;
}

#if 0
static void print_runq_stats(runqueue_t *runq, char *runq_str)
{
  int inx;
  printf("******************************************************\n");
  printf("Run queue(%s) state : \n", runq_str);
  printf("******************************************************\n");
  printf("uthreads details - (tot:%d , mask:%x)\n", runq->uthread_tot, runq->uthread_mask);
  printf("******************************************************\n");
  printf("uthread priority details : \n");
  for(inx=0; inx<MAX_UTHREAD_PRIORITY; inx++)
    printf("uthread priority (%d) - (tot:%d)\n", inx, runq->uthread_prio_tot[inx]);
  return;
  printf("******************************************************\n");
  printf("uthread group details : \n");
  for(inx=0; inx<MAX_UTHREAD_GROUPS; inx++)
    printf("uthread group (%d) - (tot:%d , mask:%x)\n", inx, runq->uthread_group_tot[inx], runq->uthread_group_mask[inx]);
  printf("******************************************************\n");
  return;
}
#endif

extern uthread_struct_t *sched_find_best_uthread(kthread_runqueue_t *kthread_runq)
{
  /* [1] Tries to find the highest priority RUNNABLE uthread in active-runq.
   * [2] Found - Jump to [FOUND]
   * [3] Switches runqueues (active/expires)
   * [4] Repeat [1] through [2]
   * [NOT FOUND] Return NULL(no more jobs)
   * [FOUND] Remove uthread from pq and return it. */

  cfs_runqueue_t *runq;
  uthread_struct_t *best_uthread;
  unsigned int uprio, ugroup;
  gt_spin_lock(&(kthread_runq->kthread_runqlock));
  runq = kthread_runq->runqueue;
  kthread_runq->kthread_runqlock.holder = 0x04;

  best_uthread = get_best_thread(runq); 
  remove_from_cfs_runqueue(runq, best_uthread);
  gt_spin_unlock(&(kthread_runq->kthread_runqlock));
  return(best_uthread);
}



/* XXX: More work to be done !!! */
extern gt_spinlock_t uthread_group_penalty_lock;
extern unsigned int uthread_group_penalty;

// XXX(CFS): No group scheduling for this project, so commented out.
//extern uthread_struct_t *sched_find_best_uthread_group(kthread_runqueue_t *kthread_runq)
//{
  /* [1] Tries to find a RUNNABLE uthread in active-runq from u_gid.
   * [2] Found - Jump to [FOUND]
   * [3] Tries to find a thread from a group with least threads in runq (XXX: NOT DONE)
   * - [Tries to find the highest priority RUNNABLE thread (XXX: DONE)]
   * [4] Found - Jump to [FOUND]
   * [5] Switches runqueues (active/expires)
   * [6] Repeat [1] through [4]
   * [NOT FOUND] Return NULL(no more jobs)
   * [FOUND] Remove uthread from pq and return it. */
//XXX(CFS): No group scheduling in this project, so this does the same as above
//  cfs_runqueue_t *runq;
//  prio_struct_t *prioq;
//  uthread_head_t *u_head;
//  uthread_struct_t *u_obj;
//  unsigned int uprio, ugroup, mask;
//  uthread_group_t u_gid;

//  return sched_find_best_uthread(kthread_runq);

//}

#if 0
/*****************************************************************************************/
/* Main Test Function */

runqueue_t active_runqueue, expires_runqueue;

#define MAX_UTHREADS 1000
uthread_struct_t u_objs[MAX_UTHREADS];

static void fill_runq(runqueue_t *runq)
{
  uthread_struct_t *u_obj;
  int inx;
  /* create and insert */
  for(inx=0; inx<MAX_UTHREADS; inx++)
  {
    u_obj = &u_objs[inx];
    u_obj->uthread_tid = inx;
    u_obj->uthread_gid = (inx % MAX_UTHREAD_GROUPS);
    u_obj->uthread_priority = (inx % MAX_UTHREAD_PRIORITY);
    __add_to_runqueue(runq, u_obj);
    printf("Uthread (id:%d , prio:%d) inserted\n", u_obj->uthread_tid, u_obj->uthread_priority);
  }

  return;
}

static void print_runq_stats(runqueue_t *runq, char *runq_str)
{
  int inx;
  printf("******************************************************\n");
  printf("Run queue(%s) state : \n", runq_str);
  printf("******************************************************\n");
  printf("uthreads details - (tot:%d , mask:%x)\n", runq->uthread_tot, runq->uthread_mask);
  printf("******************************************************\n");
  printf("uthread priority details : \n");
  for(inx=0; inx<MAX_UTHREAD_PRIORITY; inx++)
    printf("uthread priority (%d) - (tot:%d)\n", inx, runq->uthread_prio_tot[inx]);
  printf("******************************************************\n");
  printf("uthread group details : \n");
  for(inx=0; inx<MAX_UTHREAD_GROUPS; inx++)
    printf("uthread group (%d) - (tot:%d , mask:%x)\n", inx, runq->uthread_group_tot[inx], runq->uthread_group_mask[inx]);
  printf("******************************************************\n");
  return;
}

static void change_runq(runqueue_t *from_runq, runqueue_t *to_runq)
{
  uthread_struct_t *u_obj;
  int inx;
  /* Remove and delete */
  for(inx=0; inx<MAX_UTHREADS; inx++)
  {
    u_obj = &u_objs[inx];
    switch_runqueue(from_runq, to_runq, u_obj);
    printf("Uthread (id:%d , prio:%d) moved\n", u_obj->uthread_tid, u_obj->uthread_priority);
  }

  return;
}


static void empty_runq(runqueue_t *runq)
{
  uthread_struct_t *u_obj;
  int inx;
  /* Remove and delete */
  for(inx=0; inx<MAX_UTHREADS; inx++)
  {
    u_obj = &u_objs[inx];
    __rem_from_runqueue(runq, u_obj);
    printf("Uthread (id:%d , prio:%d) removed\n", u_obj->uthread_tid, u_obj->uthread_priority);
  }

  return;
}

int main()
{
  runqueue_t *active_runq, *expires_runq;
  uthread_struct_t *u_obj;
  int inx;

  active_runq = &active_runqueue;
  expires_runq = &expires_runqueue;

  init_runqueue(active_runq);
  init_runqueue(expires_runq);

  fill_runq(active_runq);
  print_runq_stats(active_runq, "ACTIVE");
  print_runq_stats(expires_runq, "EXPIRES");
  change_runq(active_runq, expires_runq);
  print_runq_stats(active_runq, "ACTIVE");
  print_runq_stats(expires_runq, "EXPIRES");
  empty_runq(expires_runq);
  print_runq_stats(active_runq, "ACTIVE");
  print_runq_stats(expires_runq, "EXPIRES");
  
  return 0;
}

#endif
