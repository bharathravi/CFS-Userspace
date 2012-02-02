#ifndef __CFS_RUNQUEUE_H
#define __CFS_RUNQUEUE_H

#include "red_black_tree.h"
#include "gt_uthread.h"

// Define a period of 200ms
#define CFS_PERIOD 200000

typedef struct __cfs_runqueue {
  // THe red black tree that stores the thread objects
  struct rb_red_blk_tree* tree;
  
  // A cache of the minimum thread in the tree, for quick access
  uthread_struct_t* min;

  // Count of number of threads in this runqueue
  unsigned int num_threads;
    
} cfs_runqueue_t;


//  Initializes the Red black tree and the runtime period for the CFS Runqueue
void inline initialize_cfs_runqueue(cfs_runqueue_t *runqueue);

// Adds a new User thread to the CFS runqeue. This is done by adding the 
// thread to the Red-Black tree
void inline add_to_cfs_runqueue(cfs_runqueue_t *runqueue, uthread_struct_t *uthread);

// Removes a user thread from the Runqueue
void inline remove_from_cfs_runqueue(cfs_runqueue_t *runqueue, uthread_struct_t *uthread);

// Determines which the best thread to schedule is. This is done by picking
// the thread with the lowest virtual runtime in the RB Tree: that is, the 
// leftmost element in the tree.
uthread_struct_t* get_best_thread(cfs_runqueue_t* runqueue);

// Determines the rightmost (maximum) thread in the runqueue
inline uthread_struct_t* get_max_uthread(cfs_runqueue_t* runqueue);
/*************** Private functions for using in the redblack tree ****************/
static int compare_virtual_runtimes(const void* a,const void* b);

static void destroy_node(void* a);

static void print_thread(const void* a);

static void info_print(void* a);

static void info_dest(void *a);

static inline uthread_struct_t* get_leftmost(rb_red_blk_tree* tree);

static inline uthread_struct_t* get_rightmost(rb_red_blk_tree* tree);

#endif
