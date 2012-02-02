#all : gt_include.h gt_kthread.c gt_kthread.h gt_uthread.c gt_uthread.h gt_pq.c gt_pq.h gt_signal.h gt_signal.c gt_spinlock.h gt_spinlock.c gt_matrix.c
#all : cfs_runqueue.c cfs_runqueue.h gt_pq.c gt_pq.h gt_signal.h gt_signal.c gt_spinlock.h gt_spinlock.c gt_matrix.c
all: rb/misc.c  rb/red_black_tree.c  rb/stack.c
	@echo Building...
	@gcc -g -o matrix test_creation.c cfs_runqueue.c  gt_kthread.c gt_pq.c gt_signal.c gt_spinlock.c gt_uthread.c redblack.c
	@echo Done!
	@echo Now run './matrix'

.PHONY : clean

clean :
	@rm matrix
	@echo Cleaned!
