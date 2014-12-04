#include <inc/x86.h>
#include <inc/string.h>

#include <kern/e1000.h>
#include <kern/pmap.h>
#include <kern/sched.h>

volatile uint32_t *e1000e_bar0; //remember that e1000e_bar0[1]==*((char*)e1000e_bar0+4)

char tx_desc_buffers[TX_ARRAY_SIZE][TX_BUFFER_SIZE]; //each tx desc in the ring buffer needs a data buffer
struct tx_desc tx_ring_buffer[TX_ARRAY_SIZE]; //size must be multiple of 8 to be 128byte aligned

char rx_desc_buffers[RX_ARRAY_SIZE][RX_BUFFER_SIZE]; //each rx desc in the ring buffer needs a data buffer
struct rx_desc rx_ring_buffer[RX_ARRAY_SIZE]; //size must be multiple of 8 to be 128byte aligned
int txtail=1; //tail pointer. an index into the ring buffer
int rxtail=RX_ARRAY_SIZE-1; //tail pointer. an index into the ring buffer


static void
bar_write(unsigned addr, void* data, int size)
{
  memcpy((char*)e1000e_bar0+addr, data, size);
}

static void
bar_read(unsigned addr, void* data, int size)
{
  memcpy(data, (char*)e1000e_bar0+addr, size);
}

int
eeprom_read(unsigned addr)
{
  int control=E1000E_EEPROM_START|(addr<<E1000E_EEPROM_ADDR_SHIFT);
  bar_write(E1000E_EERD, &control, 4);
  int ready=0;
  while (!(ready&E1000E_EEPROM_DONE))
  {
    bar_read(E1000E_EERD, &ready, 4);
  }
  control=0;
  bar_write(E1000E_EERD, &control, 4);
  return (ready&E1000E_EEPROM_RDMASK)>>16;
}

int
e1000e_init(struct pci_func *f)
{
  int mac_addr[3]={0,0,0};
  e1000e_bar0=mmio_map_region(f->reg_base[0], f->reg_size[0]);
  cprintf("e1000e nic bar 0 lives at 0x%8x\n", e1000e_bar0);
  cprintf("e1000e device status: 0x%x\n", e1000e_bar0[2]);
  mac_addr[0]=eeprom_read(E1000E_EEPROM_ETH_ADDR_0);
  mac_addr[1]=eeprom_read(E1000E_EEPROM_ETH_ADDR_1);
  mac_addr[2]=eeprom_read(E1000E_EEPROM_ETH_ADDR_2);
  cprintf("read MAC as :\t0x%8x\n\t0x%8x\n\t0x%8x\n", mac_addr[0], mac_addr[1], mac_addr[2]);
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
  int control=E1000E_TCTL_ENABLE|E1000E_PSP|E1000E_CTL|E1000E_COLD|E1000E_CTL|E1000E_RTLC;
  bar_write(E1000E_TCTL, &control, 4);

  //set up TIPG reg
  control=E1000E_IPGT|E1000E_IPGR1|E1000E_IPGR2;
  bar_write(E1000E_TIPG, &control, 4);
  return 0;
}

int
e1000e_rx_init()
{
  //first init rx ring buffer and rx desc buffers
  int i;
  for (i=0; i<RX_ARRAY_SIZE; ++i)
  {
    struct rx_desc template;
    template.addr=PADDR(rx_desc_buffers[i]);
    template.length=0;
    template.checksum=0;
    template.status=0;
    template.errors=0;
    template.special=0; //you're not special

    memset(rx_desc_buffers[i], 0, RX_BUFFER_SIZE);
    rx_ring_buffer[i]=template;
  }
  int zero=0;
  int one=1;

  //put MAC addr in RAL0 and RAH0
  uint32_t mac_h=MAC_HIGH_BITS|(0x80000000);
  uint32_t mac_l=MAC_LOW_BITS;
  bar_write(E1000E_RAL0, &mac_l, 4);
  bar_write(E1000E_RAH0, &mac_h, 4);

  //init MTA to 0
  for (i=E1000E_MTA_START; i<=E1000E_MTA_END; ++i)
  {
    bar_write(i, &zero, 4);
  }

  //disable interrupts
  bar_write(E1000E_IMS, &zero, 4);

  //put addr of ring buffer on the nic
  int rx_ring_paddr=PADDR(rx_ring_buffer);
  bar_write(E1000E_RDBAL, &rx_ring_paddr, 4);
  bar_write(E1000E_RDBAH, &zero, 4);

  //put length of ring buffer on the nic
  int ringsize=RX_ARRAY_SIZE*sizeof(struct rx_desc);
  if (ringsize % 128)
    panic("\n\nringsize not 128byte aligned\n\n");
  bar_write(E1000E_RDLEN, &ringsize, 4);

  //init head and tail
  int rdt=RX_ARRAY_SIZE-1;
  bar_write(E1000E_RDH, &zero, 4);
  bar_write(E1000E_RDT, &rdt, 4);

  //set up control reg
  int control=E1000E_RCTL_EN|E1000E_RCTL_BAM|E1000E_RCTL_BSIZE|E1000E_RCTL_BSEX|E1000E_RCTL_SECRC;//|E1000E_RCTL_UPE;//|E1000E_RCTL_MPE;
  bar_write(E1000E_RCTL, &control, 4);

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
    cprintf("not supported\n");
    return -1;
  }
  else if (!(tx_ring_buffer[txtail].status & E1000E_TXDESC_STATUS_DONE))
  {
    //queue is full hold off a bit
    cprintf("tx queue full\n");
    return -1;
  }
  else
  {
    //good to go
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

//    unsigned int oldtail=txtail;
    txtail=(txtail+1)%TX_ARRAY_SIZE; //increment txtail
    //now update tail on the nic
    bar_write(E1000E_TDT, &txtail, 4);
    /*
    unsigned int curtail;
    bar_read(E1000E_TDT, &curtail, 4);
    if (curtail!=txtail)
      panic("tails don't match");
      */
    return size;
  }
  panic("e1000e transmit unexpected state");
}

/*
 *put len bytes into buf
 *if not EOP recurse until it is EOP
 *on success: returns length of bytes transferred
 *on failure: returns -1
 * */
int
e1000e_recv(char* buf, int len)
{
  int newtail=(rxtail+1) % RX_ARRAY_SIZE;
  if (!(rx_ring_buffer[newtail].status & E1000E_RXDESC_STATUS_OK))
  {
    //queue is full hold off a bit
    //cprintf("rx queue empty %d\n", newtail);
    return -1;
  }
  else
  {
    //good to go
    rxtail=newtail;
    //cprintf("rx status OK tail:%d\n", rxtail);
    struct rx_desc *curdesc=&rx_ring_buffer[rxtail]; //grab a ptr to the desc
    int eop=curdesc->status & E1000E_RXDESC_STATUS_EOP;
    int length=curdesc->length;
    if (length>len)
      length=len; //truncate
    memcpy(buf, rx_desc_buffers[rxtail], length); //copy data into it
    //update fields
    curdesc->length=0;
    curdesc->checksum=0;
    curdesc->status=0;
    curdesc->errors=0;
    curdesc->special=0;

    //now update tail on the nic
    bar_write(E1000E_RDT, &rxtail, 4);
    return length;
  }
  panic("e1000e receive unexpected state");
}
