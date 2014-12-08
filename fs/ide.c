/*
 * Minimal PIO-based (non-interrupt-driven) IDE driver code.
 * For information about what all this IDE/ATA magic means,
 * see the materials available on the class references page.
 */

#include "fs.h"
#include <inc/x86.h>
#include <inc/memlayout.h>

#define IDE_BSY		0x80
#define IDE_DRDY	0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01

static int diskno = 1;

#ifdef INMEM_FS
extern char _inmem_fs_begin[];
extern char _inmem_fs_end[];
#endif

static int
ide_wait_ready(bool check_error)
{
#ifndef INMEM_FS
	int r;

	while (((r = inb(0x1F7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
		/* do nothing */;

	if (check_error && (r & (IDE_DF|IDE_ERR)) != 0)
		return -1;
#endif
	return 0;
}

bool
ide_probe_disk1(void)
{
#ifndef INMEM_FS
	int r, x;

	// wait for Device 0 to be ready
	ide_wait_ready(0);

	// switch to Device 1
	outb(0x1F6, 0xE0 | (1<<4));

	// check for Device 1 to be ready for a while
	for (x = 0;
	     x < 1000 && ((r = inb(0x1F7)) & (IDE_BSY|IDE_DF|IDE_ERR)) != 0;
	     x++)
		/* do nothing */;

	// switch back to Device 0
	outb(0x1F6, 0xE0 | (0<<4));

	cprintf("Device 1 presence: %d\n", (x < 1000));
	return (x < 1000);
#else
    return true;
#endif
}

bool
ide_probe_diskn(int n)
{
#ifndef INMEM_FS
	int r, x;

	// wait for Device 0 to be ready
	ide_wait_ready(0);

	// switch to Device n
	outb(0x1F6, 0xE0 | (n<<4));

	// check for Device n to be ready for a while
	for (x = 0;
	     x < 1000 && ((r = inb(0x1F7)) & (IDE_BSY|IDE_DF|IDE_ERR)) != 0;
	     x++)
		/* do nothing */;

	// switch back to Device 0
	outb(0x1F6, 0xE0 | (0<<4));

	cprintf("Device %d presence: %d\n", n, (x < 1000));
	return (x < 1000);
#else
    return true;
#endif
}

void
ide_set_disk(int d)
{
	diskno = d;
}


int
ide_read(uint32_t secno, void *dst, size_t nsecs)
{
#ifndef INMEM_FS
	int r;

	assert(nsecs <= 256);

	ide_wait_ready(0);

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, 0x20);	// CMD 0x20 means read sector

	for (; nsecs > 0; nsecs--, dst += SECTSIZE) {
		if ((r = ide_wait_ready(1)) < 0)
			return r;
		insl(0x1F0, dst, SECTSIZE/4);
	}
#else

    int i = 0;
    for(; i < nsecs; i++) {
        void* cur_dst = dst + i*SECTSIZE;
        void* addr = (void*)_inmem_fs_begin + (i+secno)*SECTSIZE;
        if(addr < (void*)_inmem_fs_end) {
            memcpy(cur_dst, addr, SECTSIZE);
        } else {
            cprintf("SHITSHITSHIT\n");
            memset(cur_dst, 0, SECTSIZE);
        }
    }

#endif
	return 0;
}

int
ide_write(uint32_t secno, const void *src, size_t nsecs)
{
#ifndef INMEM_FS
	int r;

	assert(nsecs <= 256);

	ide_wait_ready(0);

	outb(0x1F2, nsecs);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, 0x30);	// CMD 0x30 means write sector

	for (; nsecs > 0; nsecs--, src += SECTSIZE) {
		if ((r = ide_wait_ready(1)) < 0)
			return r;
		outsl(0x1F0, src, SECTSIZE/4);
	}
#else

    int i = 0;
    for(; i < nsecs; i++) {
        void* cur_src = (void*)src + i*SECTSIZE;
        void* addr = (void*)_inmem_fs_begin + (secno+i)*SECTSIZE;
        if(addr < (void*)_inmem_fs_end) {
            memcpy(addr, cur_src, SECTSIZE);
        } else {
            return -1;
        }
    }

#endif
	return 0;
}

