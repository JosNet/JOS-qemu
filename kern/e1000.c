#include <kern/e1000.h>
#include <kern/pmap.h>

volatile uint32_t *e1000e_bar0; //remember that e1000e_bar0[1]==*((char*)e1000e_bar0+4)

int
e1000e_init(struct pci_func *f)
{
  e1000e_bar0=mmio_map_region(f->reg_base[0], f->reg_size[0]);
  cprintf("e1000e nic bar 0 lives at 0x%8x\n", e1000e_bar0);
  cprintf("e1000e device status: 0x%x\n", e1000e_bar0[2]);
  return 0;
}
