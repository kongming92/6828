// test step/continue
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
  cprintf("hello, world\n");
  asm volatile("int $3");
  asm volatile("mov $1, %ebx");
  asm volatile("mov $2, %ecx");
  cprintf("i am environment %08x\n", thisenv->env_id);
}
