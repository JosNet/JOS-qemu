#include "ns.h"
#define PACKET_PAGE_RECV ((char*)0xc00000) //addr to map ipc page at

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	cprintf("input started\n");
  sys_page_alloc(0, PACKET_PAGE_RECV, PTE_P | PTE_W | PTE_U);
  while (1)
  {
    nsipcbuf.pkt.jp_len=sys_nic_receive(&nsipcbuf.pkt.jp_data[0], PGSIZE);
    if (nsipcbuf.pkt.jp_len<0)
    {
      sys_yield();
      continue;
    }
    cprintf("recv got %s\n", &nsipcbuf.pkt.jp_data[0]);
    memcpy(PACKET_PAGE_RECV, &nsipcbuf, PGSIZE);
    ipc_send(ns_envid, 0x666, PACKET_PAGE_RECV, PTE_P|PTE_W|PTE_U);
    sys_yield();
    sys_yield();
  }
}
