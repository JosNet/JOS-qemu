// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/sched.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct PageInfo *user_page_lookup(void *va, pte_t **pte_store);
unsigned int pa2va(physaddr_t pa);

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
  {"backtrace", "Display a backtrace of the stack to this point", mon_backtrace},
  {"showmapps", "Show kernel page mappings from a to b", mon_showmappings},
  {"paperm", "Change the permissions of page at va", mon_changepage},
  {"memdump", "dump memory at pa/va", mon_memdump},
  {"sh", "start the shell and do work", mon_sh_start},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_sh_start(int argc, char **argv, struct Trapframe *tf)
{
  ENV_CREATE(user_icode,ENV_TYPE_USER);
  sched_yield();
  panic("mon_sh_start returned!!!");
  return 0;
}

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
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
  if (argc<3)
  {
    cprintf("usage: showmappings va1 va2\n");
    return 0;
  }
  cprintf("this function should show page mappings\n");
  struct PageInfo* page;
  pte_t* pte;
  unsigned int start=ROUNDDOWN(strtol(argv[1], NULL, 0), PGSIZE);
  unsigned int end=ROUNDDOWN(strtol(argv[2], NULL, 0), PGSIZE);
  cprintf("start:0x%#x, end:0x%x, %d pages\n", start, end, ((end-start)/PGSIZE)+1);
  cprintf("VA          PGNUM    PHYSADDR    PERMS\n-------------------\n");
  while (start<=end)
  {
    page=user_page_lookup((void*)start, &pte);
    if (page==NULL)
    {
      cprintf("0x%08x - no page\n", start);
    }
    else
    {
      cprintf("0x%08x   %d   0x%08x   0x%x\n",start,PGNUM(start),((*pte)>>12)<<12,(*pte)&0x00000fff);
    }
    start+=PGSIZE;
  }
  return 0;
}

int
mon_changepage(int argc, char **argv, struct Trapframe *tf)
{
  if (argc<3)
  {
    cprintf("usage: paperm va1 perms\n");
    return 0;
  }
  cprintf("this function should allow you to change permssions for pages in current address space\n");
  struct PageInfo* page;
  pte_t* pte;
  unsigned int start=ROUNDDOWN(strtol(argv[1], NULL, 0), PGSIZE);
  unsigned int permissions=strtol(argv[2], NULL, 0);
  page=user_page_lookup((void*)start, &pte);
  if (page==NULL)
  {
    cprintf("0x%08x - no page\n", start);
  }
  else
  {
    cprintf("old pte: %08x\n", *pte);
    (*pte)=(((*pte)>>12)<<12) | permissions;
    cprintf("new pte: %08x\t...you monster\n", *pte);
  }
  return 0;
}

int
mon_memdump(int argc, char **argv, struct Trapframe *tf)
{
  if (argc<3)
  {
    cprintf("this function allows you to dump amnt bytes of memory 4byte aligned\n");
    cprintf("usage: memdump addr amnt isPHYSADDR?\n");
    return 0;
  }
  unsigned int isphys=strtol(argv[3], NULL, 0);
  unsigned int addr=strtol(argv[1], NULL, 0);
  unsigned int count=ROUNDUP(strtol(argv[2], NULL, 0), 4);
  if (isphys>0)
  {
    if ((addr+KERNBASE)<addr)
      return 0;
    addr+=KERNBASE;
  }
  if ((addr+count)<addr)
    return 0;
  unsigned int* pt=(unsigned int*)addr;
  int i;
  for (i=0; i<count/4; ++i)
  {
    cprintf("0x%08x\n", pt[i]);
  }
  return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
  int stackframes=0;
  uint32_t args[]={0,0,0,0,0};
  struct Eipdebuginfo dbg;
	uint32_t ebp=read_ebp();
  uint32_t eip=ebp+4;
  args[0]=ebp+8;
  args[1]=ebp+12;
  args[2]=ebp+16;
  args[3]=ebp+20;
  args[4]=ebp+24;
  //keep doing this while ebp!=0. I think recursion won't really work here...
  cprintf("Stack backtrace:\n");
  while (ebp!=0)
  {
    //do the debug info
    if (debuginfo_eip(*((uint32_t*)eip), &dbg)==-1)
      cprintf("backtrace debug error\n");
    cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x \n", ebp, *((uint32_t*)eip), *((uint32_t*)args[0]), *((uint32_t*)args[1]), *((uint32_t*)args[2]), *((uint32_t*)args[3]), *((uint32_t*)args[4]));
    cprintf("\t%s:%d: %.*s+%d\n", dbg.eip_file, dbg.eip_line, dbg.eip_fn_namelen, dbg.eip_fn_name, *((uint32_t*)eip)-dbg.eip_fn_addr);
    ebp=*((uint32_t*)ebp); //cast ebp to pointer and dereference that thing it points at (the previous ebp)
    eip=ebp+4;
    args[0]=ebp+8;
    args[1]=ebp+12;
    args[2]=ebp+16;
    args[3]=ebp+20;
    args[4]=ebp+24;
    ++stackframes;
  }
  return stackframes;
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
		buf = readline("k: ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
