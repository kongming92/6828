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
TRAPHANDLER_NOEC(int0, 0)
TRAPHANDLER_NOEC(int1, 1)
TRAPHANDLER_NOEC(int2, 2)
TRAPHANDLER_NOEC(int3, 3)
TRAPHANDLER_NOEC(int4, 4)
TRAPHANDLER_NOEC(int5, 5)
TRAPHANDLER_NOEC(int6, 6)
TRAPHANDLER_NOEC(int7, 7)
TRAPHANDLER(int8, 8)
TRAPHANDLER_NOEC(int9, 9)
TRAPHANDLER(int10, 10)
TRAPHANDLER(int11, 11)
TRAPHANDLER(int12, 12)
TRAPHANDLER(int13, 13)
TRAPHANDLER(int14, 14)
TRAPHANDLER_NOEC(int15, 15)
TRAPHANDLER_NOEC(int16, 16)
TRAPHANDLER(int17, 17)
TRAPHANDLER_NOEC(int18, 18)
TRAPHANDLER_NOEC(int19, 19)
TRAPHANDLER(int20, 20)
TRAPHANDLER(int21, 21)
TRAPHANDLER(int22, 22)
TRAPHANDLER(int23, 23)
TRAPHANDLER(int24, 24)
TRAPHANDLER(int25, 25)
TRAPHANDLER(int26, 26)
TRAPHANDLER(int27, 27)
TRAPHANDLER(int28, 28)
TRAPHANDLER(int29, 29)
TRAPHANDLER(int30, 30)
TRAPHANDLER(int31, 31)

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	pushl %ds
	pushl %es
	pushal
	movw $(GD_KD), %ax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call trap
	addl $4, %esp
