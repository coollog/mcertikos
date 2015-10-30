#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>

#include "import.h"

// CUSTOM
static spinlock_t pthread_lk;

static spinlock_t pthread_lk;
static spinlock_t pthread_init_lk;
static spinlock_t pthread_sched_lk[NUM_CPUS];
static int milliElapsed[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
	tqueue_init(mbi_addr);
	set_curid(0);

	tcb_set_state(0, TSTATE_RUN);

	// CUSTOM
	// Initialize scheduler elapsed time.
	KERN_DEBUG("Initializing milliElapsed...");
	int i;
	for (i = 0; i < NUM_CPUS; i ++) {
		milliElapsed[i] = 0;
	}
}

/**
 * Allocates new child thread context, set the state of the new child thread
 * as ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
	// CUSTOM
	spinlock_acquire(&pthread_lk);

	unsigned int pid;

	pid = kctx_new(entry, id, quota);
	tcb_set_state(pid, TSTATE_READY);

	tqueue_enqueue(NUM_IDS, pid);

	// CUSTOM
	spinlock_release(&pthread_lk);

	return pid;
}

/**
 * Yield to the next thread in the ready queue.
 * You should set the currently running thread state as ready,
 * and push it back to the ready queue.
 * And set the state of the poped thread as running, set the
 * current thread id, then switches to the new kernel context.
 * Hint: if you are the only thread that is ready to run,
 * do you need to switch to yourself?
 */
void thread_yield(void)
{
	// CUSTOM
	spinlock_acquire(&pthread_lk);

	unsigned int old_cur_pid;
	unsigned int new_cur_pid;

	old_cur_pid = get_curid();
	tcb_set_state(old_cur_pid, TSTATE_READY);
	tqueue_enqueue(NUM_IDS, old_cur_pid);

	new_cur_pid = tqueue_dequeue(NUM_IDS);
	tcb_set_state(new_cur_pid, TSTATE_RUN);
	set_curid(new_cur_pid);

	// CUSTOM
	spinlock_release(&pthread_lk);

	if (old_cur_pid != new_cur_pid){
		kctx_switch(old_cur_pid, new_cur_pid);
	}
}

// CUSTOM
/**
 * Updates elapsed time on CPU and yields if too long.
 */
void sched_update() {
	int curCPU = get_pcpu_idx();
	spinlock_acquire(&pthread_sched_lk[curCPU]);

	int elapsedTime = 1000 / LAPIC_TIMER_INTR_FREQ;
	milliElapsed[curCPU] += elapsedTime;
	// KERN_DEBUG("milliElapsed for cpu %d = %d\n", curCPU, milliElapsed[curCPU]);
	if (milliElapsed[curCPU] >= SCHED_SLICE) {
		milliElapsed[curCPU] = 0;
		// KERN_DEBUG("milliElapsed exceeded %d for cpu %d, new=%d\n", SCHED_SLICE, curCPU, milliElapsed[curCPU]);

		spinlock_release(&pthread_sched_lk[curCPU]);
		intr_eoi();

		thread_yield();
	} else {
		spinlock_release(&pthread_sched_lk[curCPU]);
	}
}
