/*****************************************************************************
 lwipopts.h - Configuration file for lwIP



 NOTE:  This file has been derived from the lwIP/src/include/lwip/opt.h
 header file. For additional details, refer to the original "opt.h" file, and lwIP
 documentation.
*****************************************************************************/

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__


/*****************************************************************************
---------- Platform specific locking ----------
*****************************************************************************/
#define SYS_LIGHTWEIGHT_PROT            1           // default is 0
#define NO_SYS                          0           // default is 0

/*****************************************************************************
 ---------- Memory options ----------
*****************************************************************************/
#define MEM_ALIGNMENT                   4           // default is 1
#define MEM_SIZE                        (10 * 1024)  // default is 1600, was 16K
#define MEMP_SANITY_CHECK               1

/*****************************************************************************
 ---------- Internal Memory Pool Sizes ----------
*****************************************************************************/
#define MEMP_NUM_PBUF                     16    // Default 16, was 16
#define MEMP_NUM_TCP_PCB                  5    // Default 5, was 12
#define MEMP_NUM_SYS_TIMEOUT              10
#define PBUF_POOL_SIZE                    16    // Default 16, was 36

/*****************************************************************************
 ---------- ARP options ----------
*****************************************************************************/
#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  10
#define ARP_QUEUEING                    1
#define ETHARP_TRUST_IP_MAC             1

/*****************************************************************************
 ---------- IP options ----------
*****************************************************************************/
#define IP_FORWARD                      0
#define IP_OPTIONS_ALLOWED              1
#define IP_REASSEMBLY                   1           // default is 1
#define IP_FRAG                         1           // default is 1
#define IP_DEFAULT_TTL                  255

/*****************************************************************************
 ---------- ICMP options ----------
*****************************************************************************/
#define LWIP_ICMP                       1
#define ICMP_TTL                       (IP_DEFAULT_TTL)

/*****************************************************************************
 ---------- DHCP options ----------
*****************************************************************************/
#define LWIP_DHCP                       1           // default is 0

/*****************************************************************************
 ---------- AUTOIP options ----------
*****************************************************************************/
#define LWIP_AUTOIP                     0           // default is 0
#define LWIP_DHCP_AUTOIP_COOP           ((LWIP_DHCP) && (LWIP_AUTOIP))
                                                    // default is 0

/*****************************************************************************
 ---------- UDP options ----------
*****************************************************************************/
#define LWIP_UDP                        1
#define UDP_TTL                         (IP_DEFAULT_TTL)

/*****************************************************************************
 ---------- TCP options ----------
*****************************************************************************/
#define TCP_TTL                         (IP_DEFAULT_TTL)
#define TCP_WND                         2048   // default is 2048
#define TCP_MSS                        128        // default is 128
#define TCP_SND_BUF                     (4 * TCP_MSS)
                                                    // default is 256, was 6 *

/*****************************************************************************
 ---------- Pbuf options ----------
*****************************************************************************/
#define PBUF_LINK_HLEN                  16          // default is 14

                                                    // default is LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN)
#define ETH_PAD_SIZE                    2           // default is 0

/*****************************************************************************
 ---------- Thread options ----------
*****************************************************************************/
#define TCPIP_THREAD_NAME              "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE          300
#define TCPIP_THREAD_PRIO               3
#define DEFAULT_THREAD_NAME            "lwIP"
#define DEFAULT_THREAD_STACKSIZE        128
#define DEFAULT_THREAD_PRIO             3

/*****************************************************************************
 ---------- Sequential layer options ----------
*****************************************************************************/
#define LWIP_NETCONN                    1           // default is 1

/*****************************************************************************
 ---------- Socket Options ----------
*****************************************************************************/
#define LWIP_SOCKET                     0           // default is 1
#define LWIP_SO_RCVTIMEO                1
/**
 * TCP_LISTEN_BACKLOG: Enable the backlog option for tcp listen pcb.
 */
#define TCP_LISTEN_BACKLOG              1

/*****************************************************************************
 ---------- Statistics options ----------
*****************************************************************************/
#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              0
#define LINK_STATS                      1
#define ETHARP_STATS                    (LWIP_ARP)
#define IP_STATS                        1
#define ICMP_STATS                      0
#define UDP_STATS                       (LWIP_UDP)
#define TCP_STATS                       (LWIP_TCP)
#define MEM_STATS                       1
#define MEMP_STATS                      1
#define SYS_STATS                       1

/*****************************************************************************
 ---------- PPP options ----------
*****************************************************************************/
#define PPP_SUPPORT                     0
#define PPPOE_SUPPORT                   0


/*****************************************************************************
 ---------- Debugging options ----------
*****************************************************************************/
#define U8_F "c"
#define S8_F "c"
#define X8_F "x"
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"


extern void LWIPDebug(const char *pcString, ...);

#define LWIP_PLATFORM_DIAG(x) {LWIPDebug x;}

#define LWIP_DEBUG						1


//#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_OFF
//#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_OFF
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_WARNING
//#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_SERIOUS
//#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_SEVERE

#define LWIP_DBG_TYPES_ON               LWIP_DBG_OFF
//#define LWIP_DBG_TYPES_ON               (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH)

#define ETHARP_DEBUG                    LWIP_DBG_OFF     // default is OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF     // default is OFF
#define PBUF_DEBUG                     LWIP_DBG_OFF
#define API_LIB_DEBUG                  LWIP_DBG_OFF
#define API_MSG_DEBUG                  LWIP_DBG_OFF
#define SOCKETS_DEBUG                  LWIP_DBG_OFF
#define ICMP_DEBUG                     LWIP_DBG_OFF
#define IGMP_DEBUG                     LWIP_DBG_OFF
#define INET_DEBUG                     LWIP_DBG_OFF
#define IP_DEBUG                        LWIP_DBG_OFF     // default is OFF
#define IP_REASS_DEBUG                 LWIP_DBG_OFF
#define RAW_DEBUG                      LWIP_DBG_OFF
#define MEM_DEBUG                      LWIP_DBG_OFF
#define MEMP_DEBUG                     LWIP_DBG_OFF
#define SYS_DEBUG                      LWIP_DBG_OFF
#define TCP_DEBUG                      LWIP_DBG_OFF
#define TCP_INPUT_DEBUG                LWIP_DBG_OFF
#define TCP_FR_DEBUG                   LWIP_DBG_OFF
#define TCP_RTO_DEBUG                  LWIP_DBG_OFF
#define TCP_CWND_DEBUG                 LWIP_DBG_OFF
#define TCP_WND_DEBUG                  LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG               LWIP_DBG_OFF
#define TCP_RST_DEBUG                  LWIP_DBG_OFF
#define TCP_QLEN_DEBUG                 LWIP_DBG_OFF
#define UDP_DEBUG                       LWIP_DBG_OFF     // default is OFF
#define TCPIP_DEBUG                    LWIP_DBG_OFF
#define PPP_DEBUG                      LWIP_DBG_OFF
#define SLIP_DEBUG                     LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF     // default is OFF
#define AUTOIP_DEBUG                   LWIP_DBG_OFF
#define SNMP_MSG_DEBUG                 LWIP_DBG_OFF
#define SNMP_MIB_DEBUG                 LWIP_DBG_OFF
#define DNS_DEBUG                      LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */
