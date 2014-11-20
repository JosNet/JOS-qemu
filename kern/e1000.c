#include <inc/x86.h>
#include <inc/string.h>

#include <kern/e1000.h>
#include <kern/pmap.h>

volatile uint32_t *e1000e_bar0; //remember that e1000e_bar0[1]==*((char*)e1000e_bar0+4)

char tx_desc_buffers[TX_ARRAY_SIZE][TX_BUFFER_SIZE]; //each tx desc in the ring buffer needs a data buffer
struct tx_desc tx_ring_buffer[TX_ARRAY_SIZE]; //size must be multiple of 8 to be 128byte aligned
int txtail=0; //tail pointer. an index into the ring buffer


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

int
e1000e_tx_init()
{
  //first init tx ring buffer and tx desc buffers
  int i;
  for (i=0; i<TX_ARRAY_SIZE; ++i)
  {
    struct tx_desc template;
    template.addr=PADDR(tx_desc_buffers[i]);
    template.length=0;
    template.cso=0;
    template.cmd=E1000E_TXDESC_CMD_RS;//|E1000E_TXDESC_CMD_EOP; //for now the packet size is fixed
    template.status=E1000E_TXDESC_STATUS_DONE;
    template.css=0;
    template.special=0; //you're not special

    memset(tx_desc_buffers[i], 0, TX_BUFFER_SIZE);
    tx_ring_buffer[i]=template;
  }
  int zero=0;
  int one=1;
  //put addr of ring buffer on the nic
  int tx_ring_paddr=PADDR(tx_ring_buffer);
  bar_write(E1000E_TDBAL, &tx_ring_paddr, 4);
  bar_write(E1000E_TDBAH, &zero, 4);

  //put length of ring buffer on the nic
  int ringsize=TX_ARRAY_SIZE*sizeof(struct tx_desc);
  bar_write(E1000E_TDLEN, &ringsize, 4);

  //init head and tail to 0
  bar_write(E1000E_TDH, &zero, 4);
  bar_write(E1000E_TDT, &one, 4);

  //set up control reg
  int control=E1000E_TCTL_ENABLE|E1000E_PSP|E1000E_CTL|E1000E_COLD;
  bar_write(E1000E_TCTL, &control, 4);

  //set up TIPG reg
  control=E1000E_IPGT|E1000E_IPGR1|E1000E_IPGR2;
  bar_write(E1000E_TIPG, &control, 4);
  return 0;
}
/*
 *on success: returns bytes put into ring buffer
 *on failure: returns -1 (probably means txqueue is full)
 * */
int
e1000e_transmit(char* data, int size)
{
  if (size>=TX_BUFFER_SIZE)
  {
    //hahahaha feature not supported yet ;)
    //cprintf("not supported\n");
    return -1;
  }
  else if (!(tx_ring_buffer[(txtail+1)%TX_ARRAY_SIZE].status & E1000E_TXDESC_STATUS_DONE))
  {
    //queue is full hold off a bit
    //cprintf("queue full\n");
    return -1;
  }
  else
  {
    //good to go
    txtail=(txtail+1)%TX_ARRAY_SIZE; //increment txtail
    struct tx_desc *curdesc=&tx_ring_buffer[txtail]; //grab a ptr to the desc
    //memset(tx_desc_buffers[txtail], 0, TX_BUFFER_SIZE); //clear the buffer
    memcpy(tx_desc_buffers[txtail], data, size); //copy data into it
    //update fields
    curdesc->length=size;
    curdesc->cso=0;
    curdesc->cmd=E1000E_TXDESC_CMD_RS|E1000E_TXDESC_CMD_EOP; //for now the packet size is fixed
    curdesc->status=0;
    curdesc->css=0;
    curdesc->special=0;

    //now update tail on the nic
    bar_write(E1000E_TDT, &txtail, sizeof(int));
    //cprintf("sent %d\n", size);
    return size;
  }
  panic("e1000e transmit unexpected state");
}

