#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

//#define LOTTERY_SCHEDULER
void sched_halt(void);
extern int priority_sums;

//code I got on the internet to approximate a RNG
uint32_t xor128(void) {
  static uint32_t x = 0xdeadbeef;
  static uint32_t y = 0xcafebffe;
  static uint32_t z = 0x698D;
  static uint32_t w = 0xABCD69;
  uint32_t t;

  t = x ^ (x << 11);
  x = y; y = z; z = w;
  return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}




// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
  idle = thiscpu->cpu_env;
  int startenvid=0;
  if (idle)
  {
    startenvid=ENVX(idle->env_id);
  }
  int i = startenvid+1;
  if (startenvid==NENV-1)
  {
    i=0;
  }
#ifndef LOTTERY_SCHEDULER
  int count=0;
  for ( ; i != startenvid; i = (i+1) % NENV)
  {
    //cprintf("env id: %d status: %d\n", i, envs[i].env_status);
    if (!&envs[i])
      break;
    if (envs[i].env_status == ENV_RUNNABLE){
 //     cprintf("running env %d\n", i);
      cprintf("\t\tcpunum:%d count:%d\n", cpunum(), count);
      env_run(&envs[i]);
      return;
    }
    ++count;
  }
  if (startenvid==0 && &envs[0] && envs[0].env_status==ENV_RUNNABLE)
  {
    env_run(&envs[0]);
    return;
  }
  if (idle && (idle->env_status == ENV_RUNNING || idle->env_status==ENV_RUNNABLE)){
    env_run(idle);
    return;
  }
#else
  //lottery scheduler
//  cprintf("lottery scheduler, %d\n", priority_sums);
  if (priority_sums<=0)
  {
    //there are no envs
  //  cprintf("no envs left\n");
    sched_halt();
  }
  int ticket=(xor128() % priority_sums)+1; //ding ding!
  //cprintf("ticket is %d\n", ticket);
  for (; i!=startenvid; i=(i+1)%NENV)
  {
    if (&envs[i] && (envs[i].env_status==ENV_RUNNABLE))
    {
      ticket-=envs[i].priority;
      if (ticket<=0)
      {
    //    cprintf("running env %d\n", id);
        env_run(&envs[i]);
        return;
      }
    }
  }
  if (startenvid==0 && &envs[0] && envs[0].env_status==ENV_RUNNABLE)
  {
    env_run(&envs[0]);
    return;
  }
  if (idle && (idle->env_status == ENV_RUNNING || idle->env_status==ENV_RUNNABLE)){
    env_run(idle);
    return;
  }
#endif
	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
    {
     // cprintf("env %d can still run\n", i);
      break;
    }
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

