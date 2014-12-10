#include <inc/stdio.h>
#include <inc/string.h>

#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

struct tx_desc tx_desc_list[N_TX_DESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_buf[N_TX_DESC];

struct rx_desc rx_desc_list[N_RX_DESC] __attribute__ ((aligned (16)));
struct rx_pkt rx_buf[N_RX_DESC];

void get_mac(uint32_t *high, uint32_t *low);

int e1000_init(struct pci_func *f) {
  pci_func_enable(f);
  e1000_regs = mmio_map_region(f->reg_base[0], f->reg_size[0]);
  size_t i;

  // BEGIN TX INITIALIZATION
  // Set up pointers in tx_desc's to tx_pkt's
  memset(tx_desc_list, 0x0, sizeof(struct tx_desc) * N_TX_DESC);
  memset(tx_buf, 0x0, sizeof(struct tx_pkt) * N_TX_DESC);
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

  e1000_regs[E1000_TIPG] = ((0x6 << 20) | (0x4 << 10) | 0xA);

  // END TX INITIALIZATION

  // BEGIN RX INITIALIZATION
  memset(rx_desc_list, 0x0, sizeof(struct rx_desc) * N_RX_DESC);
  memset(rx_buf, 0x0, sizeof(struct rx_pkt) * N_RX_DESC);
  for (i = 0; i < N_RX_DESC; i++) {
    rx_desc_list[i].addr = PADDR(rx_buf[i].buf);
  }

  // Initialize RX
  uint32_t high, low;
  get_mac(&high, &low);
  e1000_regs[E1000_RAL] = low;
  e1000_regs[E1000_RAH] = (1 << 31) | high;

  // e1000_regs[E1000_RAL] = E1000_MAC_ADDR & 0x0000FFFFFFFF;
  // e1000_regs[E1000_RAH] = E1000_MAC_ADDR >> 32;
  // e1000_regs[E1000_RAH] |= 1 << 31;  // address valid bit

  e1000_regs[E1000_MTA] = 0x0;

  e1000_regs[E1000_RDBAL] = PADDR(rx_desc_list);
  e1000_regs[E1000_RDBAH] = 0x0;

  e1000_regs[E1000_RDLEN] = N_RX_DESC * sizeof(struct rx_desc);

  e1000_regs[E1000_RDH] = 0x0;
  e1000_regs[E1000_RDT] = 127;

  e1000_regs[E1000_RCTL] |= E1000_RCTL_EN;
  e1000_regs[E1000_RCTL] &= ~E1000_RCTL_LPE;
  e1000_regs[E1000_RCTL] &= ~E1000_RCTL_LBM;
  e1000_regs[E1000_RCTL] &= ~E1000_RCTL_RDMTS;
  e1000_regs[E1000_RCTL] |= E1000_RCTL_BAM;
  e1000_regs[E1000_RCTL] &= ~E1000_RCTL_BSIZE_2048;
  e1000_regs[E1000_RCTL] |= E1000_RCTL_SECRC;

  return 1;
}

void get_mac(uint32_t *high, uint32_t *low) {
  e1000_regs[E1000_EERD] = 0x0 | E1000_EEPROM_RW_REG_START;
  while (!(e1000_regs[E1000_EERD] & E1000_EEPROM_RW_REG_DONE));  // wait
  *low = e1000_regs[E1000_EERD] >> 16;

  e1000_regs[E1000_EERD] = 0x100 | E1000_EEPROM_RW_REG_START;
  while (!(e1000_regs[E1000_EERD] & E1000_EEPROM_RW_REG_DONE));
  *low |= (e1000_regs[E1000_EERD] >> 16) << 16;   // clear bottom 16

  e1000_regs[E1000_EERD] = 0x200 | E1000_EEPROM_RW_REG_START;
  while (!(e1000_regs[E1000_EERD] & E1000_EEPROM_RW_REG_DONE));
  *high = e1000_regs[E1000_EERD] >> 16;
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

  e1000_regs[E1000_TDT] = (tail + 1) % N_TX_DESC;
  return 0;
}

int e1000_rx_pkt(char *buf) {
  uint32_t tail = (e1000_regs[E1000_RDT] + 1) % N_RX_DESC;
  if (!(rx_desc_list[tail].status & E1000_RXD_STAT_DD)) {
    return -E_RX_EMPTY;
  }

  e1000_regs[E1000_RDT] = tail;

  int len = rx_desc_list[tail].len;
  memmove(buf, rx_buf[tail].buf, len);
  rx_desc_list[tail].status &= ~E1000_RXD_STAT_DD;
  rx_desc_list[tail].status &= ~E1000_RXD_STAT_EOP;
  return len;
}
