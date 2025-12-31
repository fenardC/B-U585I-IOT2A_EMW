/**
  ******************************************************************************
  * Copyright (C) 2025-2026 C.Fenard.
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
#pragma once

#include <cstdint>

#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif /* __PACKED_STRUCT */

struct EmwTimeval_s {
  long tv_sec;
  long tv_usec;
};

#define EMW_AF_UNSPEC 0
#define EMW_AF_INET   2
#define EMW_AF_INET6  10U

#define EMW_SOCK_STREAM 1
#define EMW_SOCK_DGRAM  2

#define EMW_IPPROTO_IP  0
#define EMW_IPPROTO_TCP 6
#define EMW_IPPROTO_UDP 17

#define EMW_SOL_SOCKET 0xFFF

enum /*class*/ EmwSockOptVal : std::int32_t {
  eEMW_SO_DEBUG = 0x0001,
  eEMW_SO_ACCEPTCONN = 0x0002,
  eEMW_SO_REUSEADDR = 0x0004,
  eEMW_SO_KEEPALIVE = 0x0008,
  eEMW_SO_DONTROUTE = 0x0010,
  eEMW_SO_BROADCAST = 0x0020,
  eEMW_SO_USELOOPBACK = 0x0040,
  eEMW_SO_LINGER = 0x0080,
  eEMW_SO_OOBINLINE = 0x0100,
  eEMW_SO_REUSEPORT = 0x0200,
  eEMW_SO_BLOCKMODE = 0x1000,
  eEMW_SO_SNDBUF = 0x1001,
  eEMW_SO_SNDTIMEO = 0x1005,
  eEMW_SO_RCVTIMEO = 0x1006,
  eEMW_SO_ERROR = 0x1007,
  eEMW_SO_TYPE = 0x1008,
  eEMW_SO_NO_CHECK = 0x100A
} ;

enum class EmwIpOptVal : std::uint16_t {
  eEMW_IP_ADD_MEMBERSHIP = 0x0003,
  eEMW_IP_DROP_MEMBERSHIP = 0x0004,
  eEMW_IP_MULTICAST_TTL = 0x0005,
  eEMW_IP_MULTICAST_IF = 0x0006,
  eEMW_IP_MULTICAST_LOOP = 0x0007
} ;

#define EMW_FD_SETSIZE    64U
#define EMW_HOWMANY(x, y) (((x) + ((y) - 1)) / (y))
#define EMW_NBBY          8U
#define EMW_NFDBITS       (sizeof(unsigned long) * EMW_NBBY)
#define EMW_FDSET_MASK(n) ((unsigned long)1 << ((n) % EMW_NFDBITS))

typedef struct {
  unsigned long fds_bits[EMW_HOWMANY(EMW_FD_SETSIZE, EMW_NFDBITS)];
} EmwFdSet_t;

#define EMW_FD_SET(n, p)   ((p)->fds_bits[(n)/EMW_NFDBITS] |= EMW_FDSET_MASK(n))
#define EMW_FD_CLR(n, p)   ((p)->fds_bits[(n)/EMW_NFDBITS] &= ~EMW_FDSET_MASK(n))
#define EMW_FD_ISSET(n, p) ((p)->fds_bits[(n)/EMW_NFDBITS] & EMW_FDSET_MASK(n))
#define EMW_FD_ZERO(P) std::memset((P), 0, sizeof(*(P)))

#define EMW_IP6_ADDR_INVALID     0x00
#define EMW_IP6_ADDR_TENTATIVE   0x08
#define EMW_IP6_ADDR_TENTATIVE_1 0x09
#define EMW_IP6_ADDR_TENTATIVE_2 0x0a
#define EMW_IP6_ADDR_TENTATIVE_3 0x0b
#define EMW_IP6_ADDR_TENTATIVE_4 0x0c
#define EMW_IP6_ADDR_TENTATIVE_5 0x0d
#define EMW_IP6_ADDR_TENTATIVE_6 0x0e
#define EMW_IP6_ADDR_TENTATIVE_7 0x0f
#define EMW_IP6_ADDR_VALID       0x10
#define EMW_IP6_ADDR_PREFERRED   0x30
#define EMW_IP6_ADDR_DEPRECATED  0x50

#define EMW_IP6_ADDR_ISINVALID(addr_state)    ((addr_state) == EMW_IP6_ADDR_INVALID)
#define EMW_IP6_ADDR_ISTENTATIVE(addr_state)  ((addr_state) & EMW_IP6_ADDR_TENTATIVE)

#define EMW_IP6_ADDR_ISVALID(addr_state)      ((addr_state) & EMW_IP6_ADDR_VALID)
#define EMW_IP6_ADDR_ISPREFERRED(addr_state)  ((addr_state) == EMW_IP6_ADDR_PREFERRED)

#define EMW_IP6_ADDR_ISDEPRECATED(addr_state) ((addr_state) == EMW_IP6_ADDR_DEPRECATED)

class EmwAddress final {
  private:
    EmwAddress(void) {}
  public:
    typedef std::uint32_t SockLen_t;

  public:
    typedef struct IpAddr_s {
      constexpr IpAddr_s(void) noexcept = default;
      constexpr explicit IpAddr_s(std::uint32_t address): addr(address) {}
      std::uint32_t addr {};
    } IpAddr_t;

    typedef struct Ip6Addr_s {
      constexpr Ip6Addr_s(void) noexcept = default;
      constexpr explicit Ip6Addr_s(std::uint32_t block0, std::uint32_t block1, std::uint32_t block2, std::uint32_t block3)
        : addr{block0, block1, block2, block3}, zone(0) {}
      std::uint32_t addr[4] {};
      std::uint8_t zone {};
    } Ip6Addr_t;

    typedef struct SockAddr_s {
      std::uint8_t length {};
      std::uint8_t family {};
      std::uint8_t data[14] {};
    } SockAddr_t;

    typedef struct InAddr_s {
      constexpr InAddr_s(void) noexcept = default;
      constexpr explicit InAddr_s(std::uint32_t address): addr(address) {}
      std::uint32_t addr {};
    } InAddr_t;

    typedef struct SockAddrIn_s {
      SockAddrIn_s(void) noexcept = default;
      constexpr explicit SockAddrIn_s(std::uint16_t port, std::uint32_t address)
        : port(port), inAddr(address) {}
      std::uint8_t length {sizeof(*this)};
      std::uint8_t family {EMW_AF_INET};
      std::uint16_t port {};
      InAddr_t inAddr;
      std::uint8_t zero[8] {};
    } SockAddrIn_t;

    typedef struct In6Addr_s {
      constexpr In6Addr_s(void) noexcept = default;
      constexpr In6Addr_s(std::uint32_t data0, std::uint32_t data1, std::uint32_t data2, std::uint32_t data3) noexcept
        : un{data0, data1, data2, data3} {}
      union {
          std::uint32_t u32Addr[4];
          std::uint8_t u8Addr[16];
        } un {};
    } In6Addr_t;

    typedef struct SockAddrIn6_s {
      constexpr SockAddrIn6_s(void) = default;
      constexpr explicit SockAddrIn6_s(std::uint16_t port, const In6Addr_t &address) noexcept
        : port(port), in6Addr(address) {}
      std::uint8_t length {sizeof(*this)};
      std::uint8_t family {EMW_AF_INET6};
      std::uint16_t port {};
      std::uint32_t flowInfo {};
      In6Addr_t in6Addr;
      std::uint32_t scopeId {};
    } SockAddrIn6_t;

    typedef __PACKED_STRUCT SockAddrStorage_s {
      constexpr SockAddrStorage_s(void) noexcept = default;
      std::uint8_t length {sizeof(*this)};
      std::uint8_t family {EMW_AF_UNSPEC};
      std::uint8_t data1[2] {};
      std::uint32_t data2[3] {};
      std::uint32_t data3[3] {};
    } SockAddrStorage_t;

    typedef __PACKED_STRUCT AddrInfo_s {
      constexpr AddrInfo_s(void) noexcept = default;
      constexpr explicit AddrInfo_s(std::int32_t flags, std::int32_t family, std::int32_t sockType, std::int32_t protocol) noexcept
        : flags(flags), family(family), sockType(sockType), protocol(protocol) {}
      std::int32_t flags {};
      std::int32_t family {};
      std::int32_t sockType {};
      std::int32_t protocol {};
      SockLen_t addrLen {};
      SockAddrStorage_t sAddr {};
      char canonName[255 + 1] {};
      struct AddrInfo_s *nextPtr {};
    } AddrInfo_t;

  public:
    static std::int32_t AsciiToNetwork(const char *addressStringPtr, EmwAddress::IpAddr_t &address) noexcept;
  public:
    static std::int32_t AsciiToNetwork(const char *addressStringPtr, EmwAddress::Ip6Addr_t &address) noexcept;
  public:
    static char *NetworkToAscii(const EmwAddress::IpAddr_t &address, char (&addressString)[16]) noexcept;
  public:
    static char *NetworkToAscii(const EmwAddress::Ip6Addr_t &address, char (&addressString)[40]) noexcept;

  private:
    static std::uint32_t HostToNetworkLong(std::uint32_t hostLong);
};
