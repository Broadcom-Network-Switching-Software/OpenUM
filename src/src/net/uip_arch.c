
#include "uip.h"

#define BUF         ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define BUF6        ((struct uip6_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

#if UIP_ARCH_CHKSUM
/*---------------------------------------------------------------------------*/
#ifndef UIP_ARCH_IPCHKSUM
u16_t
uip_ipchksum(void)
{
  u16_t sum;

  sum = sal_checksum(0, &uip_buf[UIP_LLH_LEN], UIP_IPH_LEN);
  /* Take the one's complement of sum */
  sum = ~sum;
  SAL_DEBUGF(("uip_ipchksum: sum 0x%04x\n", sum));
  return uip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
static u16_t
upper_layer_chksum(u8_t proto)
{
  u16_t upper_layer_len;
  u16_t sum;

#if CFG_UIP_IPV6_ENABLED
  if (uip_ipv6) {
    upper_layer_len = 
        ((u16_t)(BUF6->len[0]) << 8) + BUF6->len[1] - uip_ext_len;
  } else 
#endif /* CFG_UIP_IPV6_ENABLED */
  {
    upper_layer_len = 
        (((u16_t)(BUF->len[0]) << 8) + BUF->len[1]) - UIP_IPH_LEN;
  }

  /* 
   * First sum pseudoheader. 
   */
  
  /* IP protocol and length fields. This addition cannot carry. */
  sum = upper_layer_len + proto;
  /* Sum IP source and destination addresses. */
#if CFG_UIP_IPV6_ENABLED
  if (uip_ipv6) {
    sum = sal_checksum(sum, (u8_t *)&BUF6->srcipaddr, 2 * sizeof(uip_ip6addr_t));
  } else 
#endif /* CFG_UIP_IPV6_ENABLED */
    sum = sal_checksum(sum, (u8_t *)&BUF->srcipaddr, 2 * sizeof(uip_ipaddr_t));

  /* Sum TCP header and data. */
#if CFG_UIP_IPV6_ENABLED
  if (uip_ipv6) {
    sum = sal_checksum(sum, &uip_buf[UIP6_IPH_LEN + UIP_LLH_LEN + uip_ext_len],
            upper_layer_len);
  } else 
#endif /* CFG_UIP_IPV6_ENABLED */
    sum = sal_checksum(sum, &uip_buf[UIP_IPH_LEN + UIP_LLH_LEN], 
            upper_layer_len);

  return uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
#if CFG_UIP_IPV6_ENABLED
u16_t
uip_icmp6chksum(void)
{
  return upper_layer_chksum(UIP_PROTO_ICMP6);
  
}
#endif /* CFG_UIP_IPV6_ENABLED */
#if UIP_TCP
/*---------------------------------------------------------------------------*/
u16_t
uip_tcpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_TCP);
}
#endif
/*---------------------------------------------------------------------------*/
#if UIP_UDP_CHECKSUMS
u16_t
uip_udpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_UDP);
}
#endif /* UIP_UDP_CHECKSUMS */

#if (UIP_ARCH_ADD32 && UIP_TCP)
void
uip_add32(u8_t *op32, u16_t op16)
{
  uip_acc32[3] = op32[3] + (op16 & 0xff);
  uip_acc32[2] = op32[2] + (op16 >> 8);
  uip_acc32[1] = op32[1];
  uip_acc32[0] = op32[0];
  
  if(uip_acc32[2] < (op16 >> 8)) {
    ++uip_acc32[1];
    if(uip_acc32[1] == 0) {
      ++uip_acc32[0];
    }
  }
  
  
  if(uip_acc32[3] < (op16 & 0xff)) {
    ++uip_acc32[2];
    if(uip_acc32[2] == 0) {
      ++uip_acc32[1];
      if(uip_acc32[1] == 0) {
        ++uip_acc32[0];
      }
    }
  }
}

#endif /* UIP_ARCH_ADD32 && UIP_TCP */

#endif /* UIP_ARCH_CHKSUM */
