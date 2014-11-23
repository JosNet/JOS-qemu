#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

#define E1000E_VEND_ID 0x8086
#define E1000E_DEV_ID 0x100E
#define E1000E_TDBAL 0x3800 //transmit base address low
#define E1000E_TDBAH 0x3804 //transmit base address high
#define E1000E_TDLEN 0x3808 //length in bytes of ring buffer
#define E1000E_TDH 0x3810 //transmit descriptor HEAD
#define E1000E_TDT 0x3818 //transmit descriptor TAIL
#define E1000E_TCTL 0x400 //transmit control reg
#define E1000E_TIPG 0x410 //transmit IPG reg


//TCTL tx enable control values
#define E1000E_TCTL_ENABLE 0x2 //TCTL enable bit
#define E1000E_PSP (0x1 << 3) //TCTL pad short packets
#define E1000E_CTL (0x0F << 4) //TCTL collision threshold
#define E1000E_COLD (0x40 << 12) //TCTL collision distance
#define E1000E_RTLC (0x1 << 24) //retransmit on late collision


//TIPG tx IEEE802.3 control values
#define E1000E_IPGT 10
#define E1000E_IPGR1 8<<10
#define E1000E_IPGR2 6<<20


//transmit desc control values
#define E1000E_TXDESC_CMD_EOP 0x1 //end of packet
#define E1000E_TXDESC_CMD_RS (0x1 << 3) //report status bit
#define E1000E_TXDESC_STATUS_DONE 0x1 //is nic done with this thing?


#define TX_ARRAY_SIZE 8
#define TX_BUFFER_SIZE 2000 //maximum size of tx desc data


struct tx_desc
{
  uint64_t addr;    //host buffer address
  uint16_t length;  //length of data
  uint8_t cso;      //checksum offset
  uint8_t cmd;      //command field
  uint8_t status;   //status field
  uint8_t css;      //checksum start field
  uint16_t special; //speshul
};

int e1000e_init(struct pci_func *f);
int e1000e_tx_init();
int e1000e_transmit(char* data, int size);
#endif	// JOS_KERN_E1000_H
