#include <lib/trap.h>
#include <lib/debug.h>
#include <dev/intr.h>
#include "import.h"

int inited = FALSE;

void
trap_init_array(void)
{
  KERN_ASSERT(inited == FALSE);
  memzero(&(TRAP_HANDLER), sizeof(trap_cb_t) * 8 * 256);
  inited = TRUE;
}

void
trap_handler_register(int cpu_idx, int trapno, trap_cb_t cb)
{
  KERN_ASSERT(0 <= cpu_idx && cpu_idx < 8);
  KERN_ASSERT(0 <= trapno && trapno < 256);
  KERN_ASSERT(cb != NULL);

  TRAP_HANDLER[cpu_idx][trapno] = cb;
}

void
trap_init(unsigned int cpu_idx){

	if (cpu_idx == 0){
		trap_init_array();
	}

	if (cpu_idx == 0){
		KERN_INFO("[BSP KERN] Register trap handlers ... \n");
	} else {
		KERN_INFO("[AP%d KERN] Register trap handlers ... \n", cpu_idx);
	}

  // TODO: for CPU # [cpu_idx], register appropriate trap handler for each trap number,
  // with trap_handler_register function defined above.
  int i;
  for (i = T_DIVIDE; i <= T_SECEV; i ++) {
    trap_handler_register(cpu_idx, i, exception_handler);
  }
  for (i = IRQ_TIMER; i <= IRQ_IDE2; i ++) {
    trap_handler_register(cpu_idx, T_IRQ0 + i, interrupt_handler);
  }
  trap_handler_register(cpu_idx, T_SYSCALL, trap);

	if (cpu_idx == 0){
		KERN_INFO("[BSP KERN] Done.\n");
	} else {
		KERN_INFO("[AP%d KERN] Done.\n", cpu_idx);
	}

	if (cpu_idx == 0){
		KERN_INFO("[BSP KERN] Enabling interrupts ... \n");
	} else {
		KERN_INFO("[AP%d KERN] Enabling interrupts ... \n", cpu_idx);
	}

	/* enable interrupts */
  intr_enable (IRQ_TIMER, cpu_idx);
  intr_enable (IRQ_KBD, cpu_idx);
 	intr_enable (IRQ_SERIAL13, cpu_idx);

	if (cpu_idx == 0){
		KERN_INFO("[BSP KERN] Done.\n");
	} else {
		KERN_INFO("[AP%d KERN] Done.\n", cpu_idx);
	}

}

