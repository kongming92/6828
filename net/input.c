#include "ns.h"

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

  int len;
  char buf[2048];
  while (1) {
    if ((len = sys_net_recv(buf)) < 0) {
      sys_yield();
      continue;
    }
    sys_page_alloc(0, &nsipcbuf, PTE_W | PTE_U | PTE_P);
    memmove(nsipcbuf.pkt.jp_data, buf, len);
    nsipcbuf.pkt.jp_len = len;
    ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_W | PTE_U | PTE_P);
  }
}
