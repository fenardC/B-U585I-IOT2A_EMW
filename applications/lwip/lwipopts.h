/**
  ******************************************************************************
  * Copyright (C) 2025 C.Fenard.
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program. If not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
   ------------------------------------
   -------------- NO SYS --------------
   ------------------------------------
*/

/**
  * NO_SYS==1: Use lwIP without OS-awareness (no thread, semaphores, mutexes or
  * mboxes). This means threaded APIs cannot be used (socket, netconn,
  * i.e. everything in the 'api' folder), only the callback-style raw API is
  * available (and you have to watch out for yourself that you don't access
  * lwIP functions/structures from more than one context at a time!)
  */
#define NO_SYS                          0

/*
   ------------------------------------
   ----------- Core locking -----------
   ------------------------------------
*/

/**
  * LWIP_TCPIP_CORE_LOCKING
  * Creates a global mutex that is held during TCPIP thread operations.
  * Can be locked by client code to perform lwIP operations without changing
  * into TCPIP thread using callbacks. See LOCK_TCPIP_CORE() and
  * UNLOCK_TCPIP_CORE().
  * Your system must provide mutexes supporting priority inversion to use this.
  */
#define LWIP_TCPIP_CORE_LOCKING         0

/**
  * SYS_LIGHTWEIGHT_PROT==1: enable inter-task protection (and task-vs-interrupt
  * protection) for certain critical regions during buffer allocation, deallocation
  * and memory allocation and deallocation.
  * ATTENTION: This is required when using lwIP from more than one context! If
  * you disable this, you must be sure what you are doing!
  */
#define SYS_LIGHTWEIGHT_PROT            1

/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/
/**
  * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
  * instead of the lwip internal allocator. Can save code size if you
  * already use it.
  */
#define MEM_LIBC_MALLOC                 0

/**
  * MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
  * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
  * speed (heap alloc can be much slower than pool alloc) and usage from interrupts
  * (especially if your netif driver allocates PBUF_POOL pbufs for received frames
  * from interrupt)!
  * ATTENTION: Currently, this uses the heap for ALL pools (also for private pools,
  * not only for internal pools defined in memp_std.h)!
  */
#define MEMP_MEM_MALLOC                 0

/**
  * MEMP_MEM_INIT==1: Force use of memset to initialize pool memory.
  * Useful if pool are moved in uninitialized section of memory. This will ensure
  * default values in pcbs struct are well initialized in all conditions.
  */
#define MEMP_MEM_INIT                   1

/**
  * MEM_ALIGNMENT: must be set to the alignment of the CPU
  *    4 byte alignment -> \#define MEM_ALIGNMENT 4
  *    2 byte alignment -> \#define MEM_ALIGNMENT 2
  */
#define MEM_ALIGNMENT                   4

/**
  * MEM_SIZE: the size of the heap memory. If the application will send
  * a lot of data that needs to be copied, this must be set high.
  */
#define MEM_SIZE                       (60 * 1024)

/**
  * MEMP_OVERFLOW_CHECK: memp overflow protection reserves a configurable
  * amount of bytes before and after each memp element in every pool and fills
  * it with a prominent default value.
  *    MEMP_OVERFLOW_CHECK == 0 no checking
  *    MEMP_OVERFLOW_CHECK == 1 checks each element when it is freed
  *    MEMP_OVERFLOW_CHECK >= 2 checks each element in every pool every time
  *      memp_malloc() or memp_free() is called (useful but slow!)
  */
#define MEMP_OVERFLOW_CHECK             2

/**
  * MEMP_SANITY_CHECK==1: run a sanity check after each memp_free() to make
  * sure that there are no cycles in the linked lists.
  */
#define MEMP_SANITY_CHECK               1

/**
  * MEM_OVERFLOW_CHECK: mem overflow protection reserves a configurable
  * amount of bytes before and after each heap allocation chunk and fills
  * it with a prominent default value.
  *    MEM_OVERFLOW_CHECK == 0 no checking
  *    MEM_OVERFLOW_CHECK == 1 checks each element when it is freed
  *    MEM_OVERFLOW_CHECK >= 2 checks all heap elements every time
  *      mem_malloc() or mem_free() is called (useful but slow!)
  */
#define MEM_OVERFLOW_CHECK              2

/**
  * MEM_SANITY_CHECK==1: run a sanity check after each mem_free() to make
  * sure that the linked list of heap elements is not corrupted.
  */
#define MEM_SANITY_CHECK                1

/**
  * MEM_USE_POOLS==1: Use an alternative to malloc() by allocating from a set
  * of memory pools of various sizes. When mem_malloc is called, an element of
  * the smallest pool that can provide the length needed is returned.
  * To use this, MEMP_USE_CUSTOM_POOLS also has to be enabled.
  */
#define MEM_USE_POOLS                   0

/**
  * MEMP_USE_CUSTOM_POOLS==1: whether to include a user file lwippools.h
  * that defines additional pools beyond the "standard" ones required
  * by lwIP. If you set this to 1, you must have lwippools.h in your
  * include path somewhere.
  */
#define MEMP_USE_CUSTOM_POOLS           0

#define LWIP_SUPPORT_CUSTOM_PBUF        0

/*
   ------------------------------------------------
   ---------- Internal Memory Pool Sizes ----------
   ------------------------------------------------
*/
/**
  * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
  * If the application sends a lot of data out of ROM (or other static memory),
  * this must be set high.
  */
#define MEMP_NUM_PBUF                   8

/**
  * MEMP_NUM_RAW_PCB: Number of raw connection PCBs
  * (requires the LWIP_RAW option)
  */
#define MEMP_NUM_RAW_PCB                4

/**
  * MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
  * per active UDP "connection".
  * (requires the LWIP_UDP option)
  */
#define MEMP_NUM_UDP_PCB                4

/**
  * MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections.
  * (requires the LWIP_TCP option)
  */
#define MEMP_NUM_TCP_PCB                32

/**
  * MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections.
  * (requires the LWIP_TCP option)
  */
#define MEMP_NUM_TCP_PCB_LISTEN         8

/**
  * MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
  * (requires the LWIP_TCP option)
  */
#define MEMP_NUM_TCP_SEG                24

/**
  * MEMP_NUM_NETBUF: the number of struct netbufs.
  * (only needed if you use the sequential API, like api_lib.c)
  */
#define MEMP_NUM_NETBUF                 4

/**
  * MEMP_NUM_NETCONN: the number of struct netconns.
  * (only needed if you use the sequential API, like api_lib.c)
  */
#define MEMP_NUM_NETCONN                24

/**
  * MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
  * for incoming packets.
  * (only needed if you use tcpip.c)
  */
#define MEMP_NUM_TCPIP_MSG_INPKT        16

/**
  * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
  */
#define PBUF_POOL_SIZE                  32

/*
   ---------------------------------
   ---------- ARP options ----------
   ---------------------------------
*/
/**
  * LWIP_ARP==1: Enable ARP functionality.
  */
#define LWIP_ARP                        1

/*
   --------------------------------
   ---------- IP options ----------
   --------------------------------
*/
/**
  * LWIP_IPV4==1: Enable IPv4
  */
/* Stay with default IPV4. */
#define LWIP_IPV4                       1

/*
   ----------------------------------
   ---------- ICMP options ----------
   ----------------------------------
*/
/**
  * LWIP_ICMP==1: Enable ICMP module inside the IP stack.
  * Be careful, disable that make your product non-compliant to RFC1122
  */
#define LWIP_ICMP                       1

/*
   ---------------------------------
   ---------- RAW options ----------
   ---------------------------------
*/
/**
  * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
  */
#define LWIP_RAW                        1

/*
   ----------------------------------
   ---------- DHCP options ----------
   ----------------------------------
*/
/**
  * LWIP_DHCP==1: Enable DHCP module.
  */
#define LWIP_DHCP                       1

/**
  * LWIP_DHCP_MAX_DNS_SERVERS > 0: Request DNS servers with discover/select.
  * DNS servers received in the response are passed to DNS via @ref dns_setserver()
  * (up to the maximum limit defined here).
  */
#define LWIP_DHCP_MAX_DNS_SERVERS       DNS_MAX_SERVERS

/*
   -----------------------------------
   ---------- AUTOIP options ---------
   -----------------------------------
*/

/*
   ----------------------------------
   ----- SNMP MIB2 support      -----
   ----------------------------------
*/

/*
   ----------------------------------
   -------- Multicast options -------
   ----------------------------------
*/

/*
   ----------------------------------
   ---------- IGMP options ----------
   ----------------------------------
*/

/*
   ----------------------------------
   ---------- DNS options -----------
   ----------------------------------
*/
/**
  * LWIP_DNS==1: Turn on DNS module. UDP must be available for DNS
  * transport.
  */
#define LWIP_DNS                        1

/** The maximum of DNS servers
  * The first server can be initialized automatically by defining
  * DNS_SERVER_ADDRESS(ipaddr), where 'ipaddr' is an 'ip_addr_t*'
  */
#define DNS_MAX_SERVERS                 3

/*
   ---------------------------------
   ---------- UDP options ----------
   ---------------------------------
*/
/**
  * LWIP_UDP==1: Turn on UDP.
  */
#define LWIP_UDP                        1

/**
  * UDP_TTL: Default Time-To-Live value.
  */
#define UDP_TTL                         255

/*
   ---------------------------------
   ---------- TCP options ----------
   ---------------------------------
*/
/**
  * LWIP_TCP==1: Turn on TCP.
  */
#define LWIP_TCP                        1

/**
  * TCP_TTL: Default Time-To-Live value.
  */
#define TCP_TTL                         255

/**
  * TCP_WND: The size of a TCP window.  This must be at least
  * (2 * TCP_MSS) for things to work well.
  * ATTENTION: when using TCP_RCV_SCALE, TCP_WND is the total size
  * with scaling applied. Maximum window value in the TCP header
  * will be TCP_WND >> TCP_RCV_SCALE
  */
#define TCP_WND                        (24 * TCP_MSS)

/**
  * TCP_QUEUE_OOSEQ==1: TCP will queue segments that arrive out of order.
  * Define to 0 if your device is low on memory.
  */
#define TCP_QUEUE_OOSEQ                 0

/**
  * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
  * you might want to increase this.)
  * For the receive side, this MSS is advertised to the remote side
  * when opening a connection. For the transmit size, this MSS sets
  * an upper limit on the MSS advertised by the remote host.
  */
/* IPV6: TCP_MSS = (Ethernet MTU - IP header size(40) - TCP header size(20)) */
/* IPV4: TCP_MSS = (Ethernet MTU - IP header size(20) - TCP header size(20)) */
#define TCP_MSS                        (1500 - PBUF_IP_HLEN - PBUF_TRANSPORT_HLEN)

/**
  * TCP_SND_BUF: TCP sender buffer space (bytes).
  * To achieve good performance, this must be at least 2 * TCP_MSS.
  */
#define TCP_SND_BUF                    (8 * TCP_MSS)

/**
  * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
  * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
  */
#define TCP_SND_QUEUELEN               (2 * TCP_SND_BUF/TCP_MSS)

/**
  * TCP_LISTEN_BACKLOG: Enable the backlog option for tcp listen pcb.
  */
#define TCP_LISTEN_BACKLOG              1

/**
  * TCP_OVERSIZE: The maximum number of bytes that tcp_write may
  * allocate ahead of time in an attempt to create shorter pbuf chains
  * for transmission. The meaningful range is 0 to TCP_MSS. Some
  * suggested values are:
  *
  * 0:         Disable oversized allocation. Each tcp_write() allocates a new
               pbuf (old behaviour).
  * 1:         Allocate size-aligned pbufs with minimal excess. Use this if your
  *            scatter-gather DMA requires aligned fragments.
  * 128:       Limit the pbuf/memory overhead to 20%.
  * TCP_MSS:   Try to create unfragmented TCP packets.
  * TCP_MSS/4: Try to create 4 fragments or less per TCP packet.
  */
#define TCP_OVERSIZE                    1
/*#define TCP_OVERSIZE                    TCP_MSS*/

/*
   ----------------------------------
   ---------- Pbuf options ----------
   ----------------------------------
*/
/**
  * PBUF_LINK_HLEN: the number of bytes that must be allocated for a
  * link level header. The default is 14, the standard value for
  * Ethernet.
  */
#define PBUF_LINK_HLEN                  (14 + ETH_PAD_SIZE)

/**
  * PBUF_LINK_ENCAPSULATION_HLEN: the number of bytes that must be allocated
  * for an additional encapsulation header before ethernet headers (e.g. 802.11)
  */
/* For EMW: EmwCoreIpc::HEADER_SIZE(6) + sizeof(WifiBypassOutParams_t)(22) */
#define PBUF_LINK_ENCAPSULATION_HLEN    28

/**
  * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
  * designed to accommodate single full size TCP frame in one pbuf, including
  * TCP_MSS, IP header, and link header.
  */
/**
  * in case of IPv4: for a MTU of 1500, TCP_MSS of 1460: 1542 rounded up to 1544
  * in case of IPv6: for a MTU of 1500, TCP_MSS of 1440: 1542 rounded up to 1544
  */
#define PBUF_POOL_BUFSIZE  \
  LWIP_MEM_ALIGN_SIZE(TCP_MSS+PBUF_IP_HLEN+PBUF_TRANSPORT_HLEN+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)

/*
   ------------------------------------------------
   ---------- Network Interfaces options ----------
   ------------------------------------------------
*/
/**
  * LWIP_SINGLE_NETIF==1: use a single netif only. This is the common case for
  * small real-life targets. Some code like routing etc. can be left out.
  */
#define LWIP_SINGLE_NETIF               0

/**
  * LWIP_NETIF_HOSTNAME==1: use DHCP_OPTION_HOSTNAME with netif's hostname
  * field.
  */
#define LWIP_NETIF_HOSTNAME             1

/**
  * LWIP_NETIF_STATUS_CALLBACK==1: Support a callback function whenever an interface
  * changes its up/down status (i.e., due to DHCP IP acquisition)
  */
#define LWIP_NETIF_STATUS_CALLBACK      1

/**
  * LWIP_NETIF_EXT_STATUS_CALLBACK==1: Support an extended callback function
  * for several netif related event that supports multiple subscribers.
  * @see netif_ext_status_callback
  */
#define LWIP_NETIF_EXT_STATUS_CALLBACK  1

/**
  * LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
  * whenever the link changes (i.e., link down)
  */
#define LWIP_NETIF_LINK_CALLBACK        1

/**
  * LWIP_NETIF_REMOVE_CALLBACK==1: Support a callback function that is called
  * when a netif has been removed
  */
#define LWIP_NETIF_REMOVE_CALLBACK      1

/**
  * LWIP_NETIF_TX_SINGLE_PBUF: if this is set to 1, lwIP *tries* to put all data
  * to be sent into one single pbuf. This is for compatibility with DMA-enabled
  * MACs that do not support scatter-gather.
  * Beware that this might involve CPU-memcpy before transmitting that would not
  * be needed without this flag! Use this only if you need to!
  *
  * ATTENTION: a driver should *NOT* rely on getting single pbufs but check TX
  * pbufs for being in one piece. If not, @ref pbuf_clone can be used to get
  * a single pbuf:
  *   if (p->next != NULL) {
  *     struct pbuf *q = pbuf_clone(PBUF_RAW, PBUF_RAM, p);
  *     if (q == NULL) {
  *       return ERR_MEM;
  *     }
  *     p = q; ATTENTION: do NOT free the old 'p' as the ref belongs to the caller!
  *   }
  */
#define LWIP_NETIF_TX_SINGLE_PBUF       1

/*
   ------------------------------------
   ---------- LOOPIF options ----------
   ------------------------------------
*/

/*
   ------------------------------------
   ---------- Thread options ----------
   ------------------------------------
*/
/**
  * TCPIP_THREAD_NAME: The name assigned to the main tcpip thread.
  */
#define TCPIP_THREAD_NAME               "LWIP-TCP/IP"

/**
  * TCPIP_THREAD_STACKSIZE: The stack size used by the main tcpip thread.
  * The stack size value itself is platform-dependent, but is passed to
  * sys_thread_new() when the thread is created.
  */
#define TCPIP_THREAD_STACKSIZE          (360 + 600)

/**
  * TCPIP_THREAD_PRIO: The priority assigned to the main tcpip thread.
  * The priority value itself is platform-dependent, but is passed to
  * sys_thread_new() when the thread is created.
  */
#define TCPIP_THREAD_PRIO               17

/**
  * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
  * The queue size value itself is platform-dependent, but is passed to
  * sys_mbox_new() when tcpip_init is called.
  */
#define TCPIP_MBOX_SIZE                 32

/**
  * DEFAULT_THREAD_NAME: The name assigned to any other lwIP thread.
  */
#define DEFAULT_THREAD_NAME             "lwIP"

/**
  * DEFAULT_THREAD_STACKSIZE: The stack size used by any other lwIP thread.
  * The stack size value itself is platform-dependent, but is passed to
  * sys_thread_new() when the thread is created.
  */
#define DEFAULT_THREAD_STACKSIZE        (512)

/**
  * DEFAULT_THREAD_PRIO: The priority assigned to any other lwIP thread.
  * The priority value itself is platform-dependent, but is passed to
  * sys_thread_new() when the thread is created.
  */
#define DEFAULT_THREAD_PRIO             16

/**
  * DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
  * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
  * to sys_mbox_new() when the recvmbox is created.
  */
#define DEFAULT_RAW_RECVMBOX_SIZE       4
/**
  * DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
  * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
  * to sys_mbox_new() when the recvmbox is created.
  */
#define DEFAULT_UDP_RECVMBOX_SIZE       16

/**
  * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
  * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
  * to sys_mbox_new() when the recvmbox is created.
  */
#define DEFAULT_TCP_RECVMBOX_SIZE       24

/**
  * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
  * The queue size value itself is platform-dependent, but is passed to
  * sys_mbox_new() when the acceptmbox is created.
  */
#define DEFAULT_ACCEPTMBOX_SIZE         24

/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
  * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
  */
#define LWIP_NETCONN                    0

/** LWIP_NETCONN_SEM_PER_THREAD==1: Use one (thread-local) semaphore per
  * thread calling socket/netconn functions instead of allocating one
  * semaphore per netconn (and per select etc.)
  * ATTENTION: a thread-local semaphore for API calls is needed:
  * - LWIP_NETCONN_THREAD_SEM_GET() returning a sys_sem_t*
  * - LWIP_NETCONN_THREAD_SEM_ALLOC() creating the semaphore
  * - LWIP_NETCONN_THREAD_SEM_FREE() freeing the semaphore
  * The latter 2 can be invoked up by calling netconn_thread_init()/netconn_thread_cleanup().
  * Ports may call these for threads created with sys_thread_new().
  */
#define LWIP_NETCONN_SEM_PER_THREAD     1

/** LWIP_NETCONN_FULLDUPLEX==1: Enable code that allows reading from one thread,
  * writing from a 2nd thread and closing from a 3rd thread at the same time.
  * LWIP_NETCONN_SEM_PER_THREAD==1 is required to use one socket/netconn from
  * multiple threads at once!
  */
#define LWIP_NETCONN_FULLDUPLEX         1

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
  * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
  */
#define LWIP_SOCKET                     1


/**
 * LWIP_COMPAT_SOCKETS==1: Enable BSD-style sockets functions names through defines.
 * LWIP_COMPAT_SOCKETS==2: Same as ==1 but correctly named functions are created.
 * While this helps code completion, it might conflict with existing libraries.
 * (only used if you use sockets.c)
 */
#define LWIP_COMPAT_SOCKETS             0

/**
 * LWIP_POSIX_SOCKETS_IO_NAMES==1: Enable POSIX-style sockets functions names.
 * Disable this option if you use a POSIX operating system that uses the same
 * names (read, write & close). (only used if you use sockets.c)
 */
#define LWIP_POSIX_SOCKETS_IO_NAMES     0

/**
  * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
  * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
  * in seconds. (does not require sockets.c, and will affect tcp.c)
  */
#define LWIP_TCP_KEEPALIVE              1

/**
  * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
  * SO_SNDTIMEO processing.
  */
#define LWIP_SO_SNDTIMEO                1

/**
  * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
  * SO_RCVTIMEO processing.
  */
#define LWIP_SO_RCVTIMEO                1

/**
  * LWIP_SO_SNDRCVTIMEO_NONSTANDARD==1: SO_RCVTIMEO/SO_SNDTIMEO take an int
  * (milliseconds, much like winsock does) instead of a struct timeval (default).
  */
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 1

/*
   ----------------------------------------
   ---------- Statistics options ----------
   ----------------------------------------
*/
/**
  * LWIP_STATS==1: Enable statistics collection in lwip_stats.
  */
#define LWIP_STATS                      1

/**
  * MEM_STATS==1: Enable mem.c stats.
  */
#define MEM_STATS                       1

/**
  * MEMP_STATS==1: Enable memp.c pool stats.
  */
#define MEMP_STATS                      1

#define LWIP_STATS_DISPLAY              1

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/

#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_GEN_ICMP6              1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define CHECKSUM_CHECK_ICMP6            1

/*
   ---------------------------------------
   ---------- IPv6 options ---------------
   ---------------------------------------
*/

/**
  * LWIP_IPV6==1: Enable IPv6
  */
#define LWIP_IPV6                       1

#define LWIP_IPV6_SCOPES                1

/**
  * LWIP_ND6_RDNSS_MAX_DNS_SERVERS > 0: Use IPv6 Router Advertisement Recursive
  * DNS Server Option (as per RFC 6106) to copy a defined maximum number of DNS
  * servers to the DNS module.
  */
#define LWIP_ND6_RDNSS_MAX_DNS_SERVERS  DNS_MAX_SERVERS

/**
  * LWIP_IPV6_DHCP6==1: enable DHCPv6 stateful/stateless address autoconfiguration.
  */
#define LWIP_IPV6_DHCP6                 1

/**
  * LWIP_IPV6_FRAG==1: Fragment outgoing IPv6 packets that are too big.
  */
#define LWIP_IPV6_FRAG                  0

/*
   ---------------------------------------
   ---------- Hook options ---------------
   ---------------------------------------
*/

/*
   ---------------------------------------
   ---------- Debugging options ----------
   ---------------------------------------
*/
/* #define LWIP_DEBUG */

/**
  * LWIP_DBG_MIN_LEVEL: After masking, the value of the debug is
  * compared against this value. If it is smaller, then debugging
  * messages are written.
  * @see debugging_levels
  */
/* #define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_WARNING */
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL

#if defined(LWIP_DEBUG)
#define SOCKETS_DEBUG                   LWIP_DBG_ON
#define SYS_DEBUG                       LWIP_DBG_ON
#define DHCP_DEBUG                      LWIP_DBG_ON
#define DHCP6_DEBUG                     LWIP_DBG_ON
#define API_LIB_DEBUG                   LWIP_DBG_ON
#define API_MSG_DEBUG                   LWIP_DBG_ON
#define INET_DEBUG                      LWIP_DBG_ON
#define IP_DEBUG                        LWIP_DBG_ON
#define IP6_DEBUG                       LWIP_DBG_ON
#define NETIF_DEBUG                     LWIP_DBG_ON
#define RAW_DEBUG                       LWIP_DBG_ON
#define MEM_DEBUG                       LWIP_DBG_ON
#define MEMP_DEBUG                      LWIP_DBG_ON
#define UDP_DEBUG                       LWIP_DBG_ON
#define TCP_DEBUG                       LWIP_DBG_ON
#define TCP_INPUT_DEBUG                 LWIP_DBG_ON
#define TCP_CWND_DEBUG                  LWIP_DBG_ON
#define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
#define PBUF_DEBUG                      LWIP_DBG_ON
#endif /* LWIP_DEBUG */

/*
   --------------------------------------------------
   ---------- Performance tracking options ----------
   --------------------------------------------------
*/

/*
   ---------------------------------
   ---------- OS options ----------
   ---------------------------------
*/
#define LWIP_ERRNO_STDINCLUDE

#define LWIP_COMPAT_MUTEX               0
#define LWIP_COMPAT_MUTEX_ALLOWED       0

#define LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX    1
#define LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS 1
#define LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS          0

#define LWIP_PLATFORM_DIAG(x) do {printf x;} while(0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIPOPTS_H */
