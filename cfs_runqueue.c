#include "cfs_runqueue.h"
#include "gt_kthread.h"


void print_all(rb_red_blk_node* node, rb_red_blk_tree* tree);

//  Initializes the Red black tree and the runtime period for the CFS Runqueue
void inline initialize_cfs_runqueue(cfs_runqueue_t *runqueue) {
  runqueue->tree = RBTreeCreate(compare_virtual_runtimes,
                          destroy_node,
                          info_dest,
                          print_thread,
                          info_print);
  runqueue->num_threads = 0;
  runqueue->min = NULL;
}

// Adds a new User thread to the CFS runqeue. This is done by adding the 
// thread to the Red-Black tree
void inline add_to_cfs_runqueue(cfs_runqueue_t *runqueue, uthread_struct_t *uthread) {
  uthread->node = RBTreeInsert(runqueue->tree, uthread, &(uthread->uthread_tid));
  runqueue->num_threads++;
  //TODO: All of this should be moved out of this function, and into uthread_schedule 
   //Shortcut, update min
  if (runqueue->min) {
    if (runqueue->min->vruntime > uthread->vruntime) {
      runqueue->min = uthread;
    }
  } else {
    runqueue->min = uthread;
  }
}

// Removes a user thread from the Runqueue
void inline remove_from_cfs_runqueue(cfs_runqueue_t *runqueue, uthread_struct_t *uthread) {
  rb_red_blk_node* n1, n2;

  if (!uthread) {
    return;
  }

  // If the thread we are removing is the min thread, we need to update
  // to the new min, since we're extracting this one.
  if (runqueue->min == uthread) {
    // Update min, since we're going to extract this one.
    n1 = runqueue->min->node;

    if (n1->parent != runqueue->tree->root) {
      runqueue->min = (uthread_struct_t *)n1->parent->key;      
    } else { 
      runqueue->min = (uthread_struct_t *)n1->right->key;      
    }
  }
  RBDelete(runqueue->tree , uthread->node);
  uthread->node = runqueue->tree->nil;
  runqueue->num_threads--;
  return;
}

// Determines which the best thread to schedule is. This is done by picking
// the thread with the lowest virtual runtime in the RB Tree: that is, the 
// leftmost element in the tree.
uthread_struct_t* get_best_thread(cfs_runqueue_t* runqueue) {
   uthread_struct_t* best_thread = NULL;
   if (!runqueue->min) {
     runqueue->min = get_leftmost(runqueue->tree);
   }

   if (!runqueue->min) {
     return NULL;
   } 
   best_thread = runqueue->min;
  
   return best_thread;
}

inline uthread_struct_t* get_max_uthread(cfs_runqueue_t* runqueue) {
  return get_rightmost(runqueue->tree);
}


/*************** Private functions for using in the redblack tree ****************/
int compare_virtual_runtimes(const void* a,const void* b) {
  // TODO(bharath): Get rid of ugly pointer casting by hardcoding RB Tree to use
  // only uthread_struct_t
  uthread_struct_t* threada = (uthread_struct_t *)a;
  uthread_struct_t* threadb = (uthread_struct_t *)b;

  if(threada->vruntime > threadb->vruntime) return(1);
  if(threada->vruntime < threadb->vruntime) return(-1);

  return(0);
}


void destroy_node(void* a) {
  uthread_struct_t *uthread = (uthread_struct_t *)a;
  uthread->node = NULL;  
}


void print_thread(const void* a) {
  //Do nothing
}

void info_print(void* a) {
  //Do nothing
}

void info_dest(void *a) {
  //Do nothing
}

static inline uthread_struct_t* get_leftmost(rb_red_blk_tree* tree) {
  rb_red_blk_node* node;
  rb_red_blk_node* parent = NULL;
  uthread_struct_t *debug;

  if (!tree || !tree->root || !tree->root->left) {
    // Check for bad trees
    return NULL;
  }

  node = tree->root->left;

  do {
    parent = node;
    node = node->left;
  } while(node!=tree->nil);
  
  if (parent == tree->nil) {
    return NULL;
  }
  return (uthread_struct_t *)parent->key;
}

static inline uthread_struct_t* get_rightmost(rb_red_blk_tree* tree) {
  rb_red_blk_node* node;
  rb_red_blk_node* parent = NULL;
  uthread_struct_t *debug;

  if (!tree || !tree->root || !tree->root->left) {
    // Check for bad trees
    return NULL;
  }

  node = tree->root->left;

  do {
    parent = node;
    node = node->right;
  } while(node!=tree->nil);

  if (parent == tree->nil) {
    return NULL;
  }
  return (uthread_struct_t *)parent->key;
}


void print_all(rb_red_blk_node* node, rb_red_blk_tree* tree) {
  uthread_struct_t* t;
  if (node == tree->nil) {
    return;
  }
  printf("L");print_all(node->left, tree);
  t = (uthread_struct_t *) node->key;
  printf("T%d ", t->uthread_tid);
  printf("R");print_all(node->right, tree);
}

// XXX(CFS): Function to test the cfs_runqueue
/*
int main() {
  cfs_runqueue_t* runqueue;
  uthread_struct_t* a,*b,*c, *d;
  int i,j,k;

  runqueue = (cfs_runqueue_t *)malloc(sizeof(cfs_runqueue_t));

  a = (uthread_struct_t *)malloc(sizeof(uthread_struct_t));
  b = (uthread_struct_t *)malloc(sizeof(uthread_struct_t));
  c = (uthread_struct_t *)malloc(sizeof(uthread_struct_t));
  a->vruntime = 1;
  b->vruntime = 2;
  c->vruntime = 3;
  
  // TEST INIT
  initialize_cfs_runqueue(runqueue);
//  printf("Initialize done\n");

  // TEST COMPARE
  i = compare_virtual_runtimes(&a, &b);
  j = compare_virtual_runtimes(&c, &b);
  k = compare_virtual_runtimes(&a, &a);
//  printf("%d %d %d\n", i, j, k);

  add_to_cfs_runqueue(runqueue, a);
  add_to_cfs_runqueue(runqueue, b);
  add_to_cfs_runqueue(runqueue, c);
  printf("A %d %d\n", a,a->node);
  printf("B %d %d\n", b,b->node);
  printf("C %d %d\n", c,c->node);
  printf("Min %d\n", runqueue->min);

 
  remove_from_cfs_runqueue(runqueue, a);
  remove_from_cfs_runqueue(runqueue, a);
  remove_from_cfs_runqueue(runqueue, a);
  return 1;
}

*/
