#ifndef __GT_SIGNAL_H
#define __GT_SIGNAL_H

/**********************************************************************/
/* kthread signal handling */
extern void kthread_install_sighandler(int signo, void (*handler)(int));
extern void kthread_block_signal(int signo);
extern void kthread_unblock_signal(int signo);

#define SCHEDULER_MIN_GRANULARITY_USEC 1000 // 4 milliseconds
#define SCHEDULER_MIN_LATENCY_USEC 5000  // 20 milliseconds

// Initializes the VTALRM signal when the kthread is created.
extern void kthread_init_vtalrm();

// Sets a VTALRM timer. This is used to make the scheduler wake up
// when the current process' timeslice has expired.
extern void kthread_set_vtalrm(long usecs);

#endif
