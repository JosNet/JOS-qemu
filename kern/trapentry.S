/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

#define better_TRAPHANDLER(name, num)						\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

better_TRAPHANDLER(vector0,0)
better_TRAPHANDLER(vector1,1)
better_TRAPHANDLER(vector2,2)
better_TRAPHANDLER(vector3,3)
better_TRAPHANDLER(vector4,4)
better_TRAPHANDLER(vector5,5)
better_TRAPHANDLER(vector6,6)
better_TRAPHANDLER(vector7,7)
better_TRAPHANDLER(vector8,8)
better_TRAPHANDLER(vector9,9)
better_TRAPHANDLER(vector10,10)
better_TRAPHANDLER(vector11,11)
better_TRAPHANDLER(vector12,12)
better_TRAPHANDLER(vector13,13)
better_TRAPHANDLER(vector14,14)
better_TRAPHANDLER(vector15,15)
better_TRAPHANDLER(vector16,16)
better_TRAPHANDLER(vector17,17)
better_TRAPHANDLER(vector18,18)
better_TRAPHANDLER(vector19,19)
better_TRAPHANDLER(vector20,20)
better_TRAPHANDLER(vector21,21)
better_TRAPHANDLER(vector22,22)
better_TRAPHANDLER(vector23,23)
better_TRAPHANDLER(vector24,24)
better_TRAPHANDLER(vector25,25)
better_TRAPHANDLER(vector26,26)
better_TRAPHANDLER(vector27,27)
better_TRAPHANDLER(vector28,28)
better_TRAPHANDLER(vector29,29)
better_TRAPHANDLER(vector30,30)
better_TRAPHANDLER(vector31,31)


/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
//;set up the trapframe structure
  pusha       //;push whole bunch of useless things
  pushl %es   //;es reg
  pushl $0x69 //;padding1
  pushl %ds   //;ds reg
  pushl $0x69 //;padding2
  pushl 36(%esp) //tf number

.data
.globl vectors
vectors:
  .long vector0
  .long vector1
  .long vector2
  .long vector3
  .long vector4
  .long vector5
  .long vector6
  .long vector7
  .long vector8
  .long vector9
  .long vector10
  .long vector11
  .long vector12
  .long vector13
  .long vector14
  .long vector15
  .long vector16
  .long vector17
  .long vector18
  .long vector19
  .long vector20
  .long vector21
  .long vector22
  .long vector23
  .long vector24
  .long vector25
  .long vector26
  .long vector27
  .long vector28
  .long vector29
  .long vector30
  .long vector31
