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
  //pte_t* pte=&uvpt[PGNUM(addr)]; //grab the pte
  //if not a write OR not a COW page
  if (!(err & 0x2) || !(uvpt[PGNUM(addr)] & PTE_COW))
  {
    panic("fork pgfault handler: COW invalid err");
  }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

  addr=ROUNDDOWN(addr, PGSIZE);
	sys_page_alloc(sys_getenvid(), PFTEMP, PTE_U|PTE_W|PTE_P); //make new page at PFTEMP
  memcpy(PFTEMP,addr,PGSIZE); //copy COW data into it
  sys_page_map(sys_getenvid(), PFTEMP, sys_getenvid(), addr, PTE_W|PTE_P|PTE_U); //map temp page at addr
  sys_page_unmap(sys_getenvid(), PFTEMP); //unmap temp page
  //???
  //PROFIT
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
  //pte_t* pte=&uvpt[pn];
  if (uvpt[pn] & (PTE_COW|PTE_W))
  {
    r=sys_page_map(sys_getenvid(), (void*)address, envid, (void*)address, PTE_COW|PTE_U|PTE_P);
    r=sys_page_map(sys_getenvid(), (void*)address, sys_getenvid(), (void*)address, PTE_COW|PTE_U|PTE_P);
  }
  else
    r=sys_page_map(sys_getenvid(), (void*)address, envid, (void*)address, PTE_U|PTE_P);
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
  int newenvid=sys_exofork();
  if (newenvid<0)
    panic("fork: exofork failed");
  if (newenvid==0)
  {
    //this is the child
    //cprintf("exofork child\n");
    thisenv=&envs[ENVX(sys_getenvid())];
    return 0;
  }
  int i;
  for (i=0; i<PGNUM(UTOP); ++i)
  {
    //if page is writable or COW and not UXSTACK
   // cprintf("i: %d, UTOP: %d\n", i, PGNUM(UTOP));
    if (!(uvpd[PDX(i<<PGSHIFT)] & PTE_P))
    {
      //the whole directory isn't present
      //cprintf("pgdir not present\n");
      i+=0x400-1;
      continue;
    }
    else if (!(uvpt[i] & PTE_P))
    {
      //page not marked
      continue;
    }
    /*
    else if (i==PGNUM(USTACKTOP))
    {
      cprintf("skipping USTACK buffer\n");
      continue;
    }*/
    else if (i==PGNUM(UXSTACKTOP-PGSIZE))
    {
      //cprintf("skipping UXSTACK\n");
      continue;
    }
  //  else if ((uvpt[PGNUM(i)] & (PTE_W|PTE_COW)) && i!=PGNUM(UXSTACKTOP-PGSIZE))
    else
    {
      //duppage
      //cprintf("duppage\n");
      duppage(newenvid, i);
    }
  }
  //cprintf("done iterating pages\n");
  if (sys_page_alloc(newenvid, (void*)(UXSTACKTOP-PGSIZE), PTE_W|PTE_U|PTE_P)<0)
    panic("couldn't allocate user exception stack");
  if (sys_env_set_pgfault_upcall(newenvid, thisenv->env_pgfault_upcall)<0)
    panic("fork:setting pgfault upcall failed");
  if (sys_env_set_status(newenvid, ENV_RUNNABLE)<0)
    panic("fork:couldn't set child status to runnable");
  return newenvid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
