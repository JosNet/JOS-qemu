#include <inc/x86.h>
#include <inc/string.h>

#include <kern/e1000.h>
#include <kern/pmap.h>

volatile uint32_t *e1000e_bar0; //remember that e1000e_bar0[1]==*((char*)e1000e_bar0+4)

char tx_desc_buffers[TX_ARRAY_SIZE][TX_BUFFER_SIZE]; //each tx desc in the ring buffer needs a data buffer
struct tx_desc tx_ring_buffer[TX_ARRAY_SIZE]; //size must be multiple of 8 to be 128byte aligned

static void
bar_write(unsigned addr, void* data, int size)
{
  memcpy((char*)e1000e_bar0+addr, data, size);
}

int
e1000e_init(struct pci_func *f)
{
  e1000e_bar0=mmio_map_region(f->reg_base[0], f->reg_size[0]);
  cprintf("e1000e nic bar 0 lives at 0x%8x\n", e1000e_bar0);
  cprintf("e1000e device status: 0x%x\n", e1000e_bar0[2]);
  return 0;
}

int e1000e_tx_init()
{
  //first init tx ring buffer and tx desc buffers
  int i;
  for (i=0; i<TX_ARRAY_SIZE; ++i)
  {
    struct tx_desc template;
    template.addr=PADDR(tx_desc_buffers[i]);
    template.length=0;
  }
  int zero=0;
  //put addr of ring buffer on the nic
  int tx_ring_paddr=PADDR(tx_ring_buffer);
  bar_write(E1000E_TDBAL, &tx_ring_paddr, sizeof(int));
  bar_write(E1000E_TDBAH, &zero, sizeof(int));

  //put length of ring buffer on the nic
  int ringsize=TX_ARRAY_SIZE*sizeof(struct tx_desc);
  bar_write(E1000E_TDLEN, &ringsize, sizeof(int));

  //init head and tail to 0
  bar_write(E1000E_TDH, &zero, sizeof(int));
  bar_write(E1000E_TDT, &zero, sizeof(int));

  //set up control reg
  int control=E1000E_TCTL_ENABLE|E1000E_PSP|E1000E_CTL|E1000E_COLD;
  bar_write(E1000E_TCTL, &control, sizeof(int));

  //set up TIPG reg
  control=E1000E_IPGT|E1000E_IPGR1|E1000E_IPGR2;
  bar_write(E1000E_TIPG, &control, sizeof(int));
  return 0;
}
