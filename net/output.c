#include "ns.h"
#define PACKET_PAGE ((char*)0xb00000) //addr to map ipc page at

extern union Nsipc nsipcbuf;

static int count=0;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
  cprintf("output start\n");
  envid_t who;
  while (ipc_recv(&who, PACKET_PAGE, 0)==NSREQ_OUTPUT)
  {
    if (who!=ns_envid)
    {
      cprintf("env id is wrong\n");
      continue;
    }
      union Nsipc *packet=(union Nsipc*)PACKET_PAGE;
      //send the packet
      int transmit=sys_nic_transmit((void*)packet->pkt.jp_data, packet->pkt.jp_len);
      //cprintf("sent %s\n", packet->pkt.jp_data);
      //++count;
  }
}
