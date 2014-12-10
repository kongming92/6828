#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_STATUS  0x00008 / 4  /* Device Status - RO */

#define E1000_TDBAL   0x03800 / 4  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH   0x03804 / 4  /* TX Descriptor Base Address High - RW */
#define E1000_TDBAH   0x03804 / 4  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN   0x03808 / 4  /* TX Descriptor Length - RW */
#define E1000_TDH     0x03810 / 4  /* TX Descriptor Head - RW */
#define E1000_TDT     0x03818 / 4  /* TX Descriptor Tail - RW */
#define E1000_TCTL    0x00400 / 4  /* TX Control - RW */
#define E1000_TIPG    0x00410 / 4  /* TX Inter-packet gap -RW */

#define E1000_RAL     0x05400 / 4  /* Receive Address - RW Array */
#define E1000_RAH     0x05404 / 4  /* Receive Address - RW Array */
#define E1000_MTA     0x05200 / 4  /* Multicast Table Array - RW Array */
#define E1000_RDBAL   0x02800 / 4  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH   0x02804 / 4  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN   0x02808 / 4  /* RX Descriptor Length - RW */
#define E1000_RDH     0x02810 / 4  /* RX Descriptor Head - RW */
#define E1000_RDT     0x02818 / 4  /* RX Descriptor Tail - RW */
#define E1000_RCTL    0x00100 / 4  /* RX Control - RW */

#define E1000_EERD    0x00014 / 4  /* EEPROM Read - RW */
#define E1000_EEPROM_RW_REG_START  1    /* First bit for telling part to start operation */
#define E1000_EEPROM_RW_REG_DONE   0x10 /* Offset to READ/WRITE done bit */

/* Transmit bit descriptions */
#define E1000_TXD_STAT_DD     0x00000001 /* Descriptor Done */
#define E1000_TXD_CMD_EOP     0x01 /* End of Packet */
#define E1000_TXD_CMD_RS      0x08 /* Report Status */

/* Transmit Control */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */

/* Receive bit descriptions */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */

/* Receive Control */
#define E1000_RCTL_EN     0x00000002    /* enable */
#define E1000_RCTL_LPE    0x00000020    /* long packet enable */
#define E1000_RCTL_LBM    0x000000C0    /* loopback mode */
#define E1000_RCTL_RDMTS  0x00000300    /* rx desc min threshold size */
#define E1000_RCTL_BAM    0x00008000    /* broadcast enable */
#define E1000_RCTL_BSIZE_2048   0x00030000    /* rx buffer size 2048 */
#define E1000_RCTL_SECRC  0x04000000    /* Strip Ethernet CRC */

#endif  // JOS_KERN_E1000_H

volatile uint32_t *e1000_regs;

/* TX constants and structs */
#define N_TX_DESC 64
#define TX_PACKET_SIZE 1518

#define E_TX_TOO_LONG 100
#define E_TX_LIST_FULL 101

struct tx_desc {
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
} __attribute__ ((packed));

struct tx_pkt {
  uint8_t buf[TX_PACKET_SIZE];
} __attribute__ ((packed));

int e1000_init(struct pci_func *f);
int e1000_tx_pkt(char *buf, int len);

/* RX constants and structs */
#define N_RX_DESC 128
#define RX_PACKET_SIZE 2048

#define E1000_MAC_ADDR 0x563412005452

#define E_RX_EMPTY 102

struct rx_desc {
  uint64_t addr;
  uint16_t len;
  uint16_t cksum;
  uint8_t status;
  uint8_t errors;
  uint16_t special;
} __attribute__ ((packed));

struct rx_pkt {
  uint8_t buf[RX_PACKET_SIZE];
} __attribute__ ((packed));

int e1000_rx_pkt(char *buf);

