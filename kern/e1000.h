#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#define E1000E_VEND_ID 0x8086
#define E1000E_DEV_ID 0x100E
#include <kern/pci.h>

int e1000e_init(struct pci_func *f);

#endif	// JOS_KERN_E1000_H
