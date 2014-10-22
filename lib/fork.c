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
  pte_t* pte=&uvpt[PGNUM(addr)]; //grab the pte
  //if not a write OR not a COW page
  if (!(err & 0x2) || !(*pte & PTE_COW))
  {
    panic("fork COW invalid err");
  }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	sys_page_alloc(sys_getenvid(), PFTEMP, PTE_U); //make new page at PFTEMP
  memcpy(PFTEMP,addr,PGSIZE); //copy COW data into it
  sys_page_map(sys_getenvid(), PFTEMP, sys_getenvid(), addr, PTE_W); //map temp page at addr
  sys_page_unmap(sys_getenvid(), PFTEMP); //unmap temp page
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
	int address=pn*PGSIZE;
  pte_t* pte=&uvpt[pn];
  if (*pte & (PTE_COW|PTE_W))
    r=sys_page_map(sys_getenvid(), (void*)address, envid, (void*)address, PTE_COW);
  else
    r=sys_page_map(sys_getenvid(), (void*)address, envid, (void*)address, PTE_U);
	if (r<0)
    panic("duppage messed up");
  return 0;
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
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
  int child=sys_exofork();
  if (child<0)
    panic("fork: exofork failed");
  int i;
  for (i=0; i<PGNUM(UTOP); i)
  {
    //if page is writable or COW and not UXSTACK
    if ((uvpt[PGNUM(i)] & (PTE_W|PTE_COW)) && i!=PGNUM(UXSTACKTOP-PGSIZE))
    {
      //duppage
      duppage(child, PGNUM(i));
    }
  }
  sys_env_set_pgfault_upcall(child, pgfault);
  sys_env_set_status(child, ENV_RUNNABLE);
  //set child tf eax to 0
  envs[child].env_tf.tf_regs.reg_eax=0;
  return child;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
