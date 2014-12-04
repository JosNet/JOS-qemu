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


#define TX_ARRAY_SIZE 32
#define TX_BUFFER_SIZE 2048 //maximum size of tx desc data

#define RX_ARRAY_SIZE 128
#define RX_BUFFER_SIZE 2048 //maximum size of rx desc data

//rx control values
#define MAC_HIGH_BITS 0x00005634
#define MAC_LOW_BITS 0x12005452
#define E1000E_RAL0 0x5400            //receive addr low bits
#define E1000E_RAH0 0x5404            //receive addr high bits
#define E1000E_IMS 0xD0               //interrupt set register
#define E1000E_RDBAL 0x2800           //receive base addr low
#define E1000E_RDBAH 0x2804           //receive base addr high
#define E1000E_RDLEN 0x2808           //length in bytes of ring buffer
#define E1000E_RDH 0x2810             //receive descriptor HEAD
#define E1000E_RDT 0x2818             //receive descriptor TAIL
#define E1000E_RCTL 0x100             //receive control reg
#define E1000E_RCTL_EN (1<<1)         //receive enable
#define E1000E_RCTL_UPE (1<<3)        //receive unicast promiscuous
#define E1000E_RCTL_MPE (1<<4)        //receive multicast promiscuous
#define E1000E_RCTL_BAM (1<<15)       //receive broadcasts
#define E1000E_RCTL_BSIZE (0x0 << 16) //receive buffer size
#define E1000E_RCTL_BSEX (0x0 << 25)  //receive buffer size extension
#define E1000E_RCTL_SECRC (0x1 << 26) //strip crc from receive packet
#define E1000E_RXDESC_STATUS_OK 0x1   //is nic done using this thing?
#define E1000E_RXDESC_STATUS_EOP 0x2  //is this the last chunk of packet?
#define E1000E_MTA_START 0x5200       //first reg in MTA array
#define E1000E_MTA_END 0x53FC         //last reg in MTA array


//eeprom vals
#define E1000E_EERD 0x14                //eeprom read register
#define E1000E_EEPROM_START 0x1         //bit to start eeprom read
#define E1000E_EEPROM_ADDR_SHIFT 8     //amt to shift read addr by
#define E1000E_EEPROM_DONE (0x1 << 4)   //when done bit 4 will be set
#define E1000E_EEPROM_RDMASK 0xFFFF0000 //top half of reg is read data
#define E1000E_EEPROM_ETH_ADDR_0 0x0   //high chunk of MAC
#define E1000E_EEPROM_ETH_ADDR_1 0x1   //middle chunk of MAC
#define E1000E_EEPROM_ETH_ADDR_2 0x2   //low chunk of MAC



struct tx_desc
{
  uint64_t addr;    //host buffer address
  uint16_t length;  //length of data
  uint8_t cso;      //checksum offset
  uint8_t cmd;      //command field
  uint8_t status;   //status field
  uint8_t css;      //checksum start field
  uint16_t special; //speshul
}__attribute__((aligned(0x10)));

struct rx_desc
{
  uint64_t addr;     //host buffer address
  uint16_t length;   //length of data
  uint16_t checksum; //packet checksum
  uint8_t status;    //status field
  uint8_t errors;    //errors field
  uint16_t special;  //speshul
}__attribute__((aligned(0x10)));

int e1000e_init(struct pci_func *f);
int e1000e_tx_init();
int e1000e_rx_init();
int e1000e_transmit(char* data, int size);
int e1000e_recv(char* buf, int len);
#endif	// JOS_KERN_E1000_H
