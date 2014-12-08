// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>
#include <inc/x86.h>
#include <inc/mmu.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

void _pgfault_upcall();

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
    // cprintf("pgfault at %x va %x\n", utf->utf_eip, utf->utf_fault_va);
	void *addr = (void *) ROUNDDOWN(utf->utf_fault_va, PGSIZE);
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

    pte_t addr_entry = uvpt[PGNUM(addr)];

    if(0 == (err & 2))
        panic("Page fault on read access");

    if(0 == (addr_entry & PTE_COW))
        panic("Page fault on access to non-COW page directory");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

    void* new_page = (void*) PFTEMP;
    if(sys_page_alloc(0, new_page, PTE_P | PTE_U | PTE_W))
        panic("Failed to allocate new page to replace COW");

    memcpy(new_page, addr, PGSIZE);
    if(0 != (r = sys_page_map(0, new_page, 0, addr, PTE_P | PTE_U | PTE_W)))
        panic("Failed to sys_page_map! %x", r);

    sys_page_unmap(0, new_page);
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
duppage(envid_t envid, unsigned pn, int use_cow)
{
	int r;
    void* addr = (void*)(pn * PGSIZE);
    int perms = PTE_P | PTE_U;

    pte_t addr_entry = uvpt[PGNUM(addr)];

    if(!((addr_entry & perms) == perms))
        panic("called duppage on non-accessible page");

    if((addr_entry & PTE_W) != 0) {
        if(use_cow)
            perms |= PTE_COW;
        else
            perms |= PTE_W;
    }
        
    if((addr_entry & PTE_COW) != 0)
        perms |= PTE_COW;

    if((addr_entry & PTE_SHARE) != 0)
        perms = addr_entry & PTE_SYSCALL;

    if(sys_page_map(0, addr, envid, addr, perms))
        panic("Couldn't remap page!");

    if(sys_page_map(0, addr, 0, addr, perms))
        panic("Couldn't remap page!");

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
gen_fork(int use_cow)
{
    uint8_t *addr;
	int r;

    // Just to make sure... it'll get cowed over
	set_pgfault_handler(pgfault);

    envid_t new_envid = sys_exofork();

    if (new_envid < 0) panic("fork: %e", new_envid);
	if (new_envid == 0) {
        thisenv = &envs[sys_getenvid() & 0xfff];
        cprintf("I'm the child!\n");
        return 0;
	}
    cprintf("I'm the parent!\n");

    if(sys_env_set_pgfault_upcall(new_envid, thisenv->env_pgfault_upcall))
        panic("Couldn't set pgfault upcall for new env");

    // Copy the address space COW
	for (addr = (uint8_t*) 0; addr < (uint8_t*)USTACKTOP; addr += PGSIZE) {
        if(0 == (uvpd[PDX(addr)] & PTE_P)) continue;

        pte_t cur_entry = uvpt[PGNUM(addr)];
        if((PTE_P | PTE_U) == (cur_entry & (PTE_P | PTE_U)))
            duppage(new_envid, (unsigned)addr / PGSIZE, use_cow);
    }

    // Allocate a new exception stack
    if(sys_page_alloc(new_envid, (void*)(UXSTACKTOP-PGSIZE), PTE_P | PTE_U | PTE_W))
        panic("Failed to allocate new exception stack");

    // Start the child environment running
	if ((r = sys_env_set_status(new_envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return new_envid;
}

int
fork(void)
{
    return gen_fork(1);
}

int
sfork(void)
{
    return gen_fork(0);
}
