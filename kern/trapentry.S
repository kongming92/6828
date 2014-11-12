/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


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
TRAPHANDLER_NOEC(int20, 20)
TRAPHANDLER_NOEC(int21, 21)
TRAPHANDLER_NOEC(int22, 22)
TRAPHANDLER_NOEC(int23, 23)
TRAPHANDLER_NOEC(int24, 24)
TRAPHANDLER_NOEC(int25, 25)
TRAPHANDLER_NOEC(int26, 26)
TRAPHANDLER_NOEC(int27, 27)
TRAPHANDLER_NOEC(int28, 28)
TRAPHANDLER_NOEC(int29, 29)
TRAPHANDLER(int30, 30)
TRAPHANDLER_NOEC(int31, 31)
TRAPHANDLER_NOEC(int32, 32)
TRAPHANDLER_NOEC(int33, 33)
TRAPHANDLER_NOEC(int34, 34)
TRAPHANDLER_NOEC(int35, 35)
TRAPHANDLER_NOEC(int36, 36)
TRAPHANDLER_NOEC(int37, 37)
TRAPHANDLER_NOEC(int38, 38)
TRAPHANDLER_NOEC(int39, 39)
TRAPHANDLER_NOEC(int40, 40)
TRAPHANDLER_NOEC(int41, 41)
TRAPHANDLER_NOEC(int42, 42)
TRAPHANDLER_NOEC(int43, 43)
TRAPHANDLER_NOEC(int44, 44)
TRAPHANDLER_NOEC(int45, 45)
TRAPHANDLER_NOEC(int46, 46)
TRAPHANDLER_NOEC(int47, 47)

TRAPHANDLER_NOEC(int_syscall, T_SYSCALL);

.globl ints
ints:
	.long int0
	.long int1
	.long int2
	.long int3
	.long int4
	.long int5
	.long int6
	.long int7
	.long int8
	.long int9
	.long int10
	.long int11
	.long int12
	.long int13
	.long int14
	.long int15
	.long int16
	.long int17
	.long int18
	.long int19
	.long int20
	.long int21
	.long int22
	.long int23
	.long int24
	.long int25
	.long int26
	.long int27
	.long int28
	.long int29
	.long int30
	.long int31
	.long int32
	.long int33
	.long int34
	.long int35
	.long int36
	.long int37
	.long int38
	.long int39
	.long int40
	.long int41
	.long int42
	.long int43
	.long int44
	.long int45
	.long int46
	.long int47


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
