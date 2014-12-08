#include <inc/stdio.h>
#include <inc/string.h>

#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

struct tx_desc tx_desc_list[N_TX_DESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_buf[N_TX_DESC];

int e1000_init(struct pci_func *f) {

  pci_func_enable(f);

  e1000_regs = mmio_map_region(f->reg_base[0], f->reg_size[0]);
  cprintf("status is %x\n", e1000_regs[E1000_STATUS]);

  // Set up pointers in tx_desc's to tx_pkt's
  memset(tx_desc_list, 0x0, sizeof(struct tx_desc) * N_TX_DESC);
  memset(tx_buf, 0x0, sizeof(struct tx_pkt) * N_TX_DESC);

  size_t i;
  for (i = 0; i < N_TX_DESC; i++) {
    tx_desc_list[i].addr = PADDR(tx_buf[i].buf);
    tx_desc_list[i].status |= E1000_TXD_STAT_DD;
  }

  // Initialize TX
  e1000_regs[E1000_TDBAL] = PADDR(tx_desc_list);
  e1000_regs[E1000_TDBAH] = 0x0;

  e1000_regs[E1000_TDLEN] = N_TX_DESC * sizeof(struct tx_desc);

  e1000_regs[E1000_TDH] = 0x0;
  e1000_regs[E1000_TDT] = 0x0;

  e1000_regs[E1000_TCTL] = E1000_TCTL_EN | E1000_TCTL_PSP;
  e1000_regs[E1000_TCTL] &= ~E1000_TCTL_CT;
  e1000_regs[E1000_TCTL] |= (0x10 << 4);
  e1000_regs[E1000_TCTL] &= ~E1000_TCTL_COLD;
  e1000_regs[E1000_TCTL] |= (0x40 << 12);

  e1000_regs[E1000_TIPG] = ((0x4 << 20) | (0x6 << 10) | 0xA);

  // TEST
  char test[3];
  test[0] = 12;
  test[1] = 22;
  test[2] = 32;
  e1000_tx_pkt(test, 3);

  return 1;
}

int e1000_tx_pkt(char *buf, int len) {
  if (len > TX_PACKET_SIZE) {
    return -E_TX_TOO_LONG;
  }

  uint32_t tail = e1000_regs[E1000_TDT];
  // check the tail
  if (!(tx_desc_list[tail].status & E1000_TXD_STAT_DD)) {
    return -E_TX_LIST_FULL;
  }
  memmove(tx_buf[tail].buf, buf, len);

  tx_desc_list[tail].status &= ~E1000_TXD_STAT_DD;
  tx_desc_list[tail].length = len;
  tx_desc_list[tail].cmd |= E1000_TXD_CMD_RS;
  tx_desc_list[tail].cmd |= E1000_TXD_CMD_EOP;

  cprintf("CMD: %x\n", tx_desc_list[tail].cmd);

  e1000_regs[E1000_TDT] = (tail + 1) % N_TX_DESC;

  cprintf("success\n");
  return 0;
}
