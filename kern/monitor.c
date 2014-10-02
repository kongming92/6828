// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

// page table stuff
#include <inc/mmu.h>
#include <kern/pmap.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
  { "backtrace", "Show the backtrace", mon_backtrace },
  { "mappings", "Show virtual memory mappings", mon_mappings },
  { "changeperm", "Change permissions of mappings", mon_changeperm },
  { "dumpmem", "Dump contents of memory", mon_dumpmem }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  uint32_t *ebp = (uint32_t *)read_ebp();
  struct Eipdebuginfo eipinfo;
  cprintf("Stack backtrace:\n");
  while (ebp) {
    debuginfo_eip(ebp[1], &eipinfo);
    uintptr_t offset = (uintptr_t)ebp[1] - eipinfo.eip_fn_addr;

    cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", ebp, ebp[1], ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
    cprintf("       %s:%d: ", eipinfo.eip_file, eipinfo.eip_line);
    cprintf("%.*s", eipinfo.eip_fn_namelen, eipinfo.eip_fn_name);
    cprintf("+%u\n", offset);

    ebp = (uint32_t *)(ebp[0]);
  }
	return 0;
}

// Hex string to integer:
// http://codereview.stackexchange.com/questions/42976/hexadecimal-to-integer-conversion-function
static int htoi(const char *s, size_t *res)
{
  if ('0' == s[0] && ('x' == s[1] || 'X' == s[1]))
    s += 2;

  int c;
  size_t rc;
  for (rc = 0; '\0' != (c = *s); s++) {
    if ( c >= 'a' && c <= 'f') {
      c = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      c = c - 'A' + 10;
    } else if (c >= '0' && c <= '9') {
      c = c - '0';
    } else {
      return -1;
    }
    rc *= 16;
    rc += (size_t) c;
  }
  *res = rc;
  return 0;
}

// USAGE: mappings start_addr end_addr
// start and end addrs must be hex (0x...)
int mon_mappings(int argc, char **argv, struct Trapframe *tf) {
	if (!argv[1] || !argv[2]) {
		cprintf("usage: mappings start end\n");
		cprintf("start and end must be valid hex virtual addresses (e.g. 0xFFFF)\n");
		return 0;
	}

	size_t start;
	size_t end;
	htoi(argv[1], &start);
	htoi(argv[2], &end);

	if (start > end) {
		cprintf("invalid range\n");
		return 0;
	}

	cprintf("mappings\nstart: 0x%08x; end: 0x%08x\n", start, end);

	size_t i;
	pte_t *pte;
	for (i = start; i <= end; i += PGSIZE) {
		if ((pte = pgdir_walk(kern_pgdir, (void *)i, 0)) != NULL) {
			cprintf("virtual address 0x%08x: ", i);
			cprintf("physical address: 0x%08x; permissions: 0x%x\n", PTE_ADDR(*pte), *pte & 0xFFF);
		}
	}
	return 0;
}

// USAGE: changeperm virtual_addr perm
// virtual_addr and perm must be valid hex values (e.g. permission must be < 2^12)
int mon_changeperm(int argc, char **argv, struct Trapframe *tf) {
	if (!argv[1] || !argv[2]) {
		cprintf("usage: changeperm virtual_addr perm\n");
		return 0;
	}

	size_t va;
	size_t perm;
	htoi(argv[1], &va);
	htoi(argv[2], &perm);

	if (perm >= (1 << 12)) {
		cprintf("permissions must be only in lower 12 bits!\n");
		return 0;
	}

	pte_t *pte;
	if ((pte = pgdir_walk(kern_pgdir, (void *)va, 0)) != NULL) {
		cprintf("old permissions: 0x%x\n", *pte & 0xFFF);
		cprintf("new permissions: 0x%x\n", perm);
		*pte = PTE_ADDR(*pte) | perm;
	} else {
		cprintf("va 0x%08x not found!\n", va);
	}
	return 0;
}

// USAGE: dumpmem start end
int mon_dumpmem(int argc, char **argv, struct Trapframe *tf) {
	if (!argv[1] || !argv[2]) {
		cprintf("usage: dumpmem start end (must be valid hex addresses)\n");
		return 0;
	}

	size_t start;
	size_t end;
	htoi(argv[1], &start);
	htoi(argv[2], &end);

	if (start > end) {
		cprintf("invalid range\n");
		return 0;
	}

	size_t i;
	for (i = start; i <= end; i++) {
		cprintf("virtual address: 0x%08x; contents: 0x%08x\n", i, *(int *)i);
	}
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
