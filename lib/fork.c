// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t pte = uvpt[PGNUM(addr)];
	if (!((err & FEC_WR) && (pte & PTE_COW))) {
		panic("fork pgfault: not write or not COW\n");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if (sys_page_alloc(0, (void *)PFTEMP, PTE_U | PTE_W | PTE_P) < 0) {
		panic("fork pgfault: cannot allocate page\n");
	}

	void *pgalign = (void *)ROUNDDOWN(addr, PGSIZE);
	memmove((void *)PFTEMP, pgalign, PGSIZE);

	if (sys_page_map(0, (void *)PFTEMP, 0, pgalign, PTE_U | PTE_W | PTE_P) < 0) {
		panic("fork pgfault: cannot map page\n");
	}
	if (sys_page_unmap(0, (void *)PFTEMP) < 0) {
		panic("fork pgfault: cannot unmap temp page\n");
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	int perm;

	void *va = (void *)(pn * PGSIZE);
	pte_t pte = uvpt[pn];

	if (!(pte & PTE_P) || !(pte & PTE_U)) {
		panic("duppage: why is the page not present or not user?\n");
	}

	if (!(pte & PTE_SHARE) && ((pte & PTE_W) || (pte & PTE_COW))) {
		perm = PTE_COW | PTE_U | PTE_P;
		if ((r = sys_page_map(0, va, envid, va, perm)) < 0) {
			return r;
		}
		if ((r = sys_page_map(0, va, 0, va, perm)) < 0) {
			return r;
		}
	} else {
		perm = PGOFF(pte) & PTE_SYSCALL;
		if ((r = sys_page_map(0, va, envid, va, perm)) < 0) {
			return r;
		}
	}
	return 0;
}

static int duppage_share(envid_t envid, unsigned pn) {
	void *va = (void *)(pn * PGSIZE);
	pte_t pte = uvpt[pn];
	int perm;

	if (!(pte & PTE_P) || !(pte & PTE_U)) {
		panic("duppage_share: why is the page not present or not user?\n");
	}

	perm = PGOFF(pte) & (PTE_P | PTE_U | PTE_SYSCALL);
	if (sys_page_map(0, va, envid, va, perm) < 0) {
		panic("duppage_share: cannot map page\n");
	}
	return 0;
}

envid_t real_fork(bool share) {
	set_pgfault_handler(pgfault);

	int r;
	envid_t envid;
	if ((envid = sys_exofork()) < 0) {
		return envid;
	}
	if (envid == 0) {		// child
		// thisenv = &envs[ENVX(sys_getenvid())]; // comment out for challenge problem
		return 0;
	}

	// parent here
	size_t pn;
	pde_t pde;
	pte_t pte;

	for (pn = UTEXT / PGSIZE; pn * PGSIZE < USTACKTOP; pn++) {
		pde = uvpd[PDX(pn * PGSIZE)];
		if ((pde & PTE_P) && (pde & PTE_U)) {
			pte = uvpt[pn];
			if ((pte & PTE_P) && (pte & PTE_U)) {
				if (share && ((pn * PGSIZE) < (USTACKTOP - PGSIZE))) {
					duppage_share(envid, pn);
				} else {
					if ((r = duppage(envid, pn)) < 0) {
						return r;
					}
				}
			}
		}
	}

	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P)) < 0) {
		return r;
	}
	if ((r = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall)) < 0) {
		return r;
	}
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
		return r;
	}

	return envid;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	return real_fork(0);
}

// Challenge!
int
sfork(void)
{
	return (int)real_fork(1);
}
