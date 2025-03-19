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
#include "AppConsolePing.hpp"
#include "AppLwipServices.hpp"
#include "stm32u5xx_hal.h"
#include "lwip/api.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/icmp.h"
#include "lwip/icmp6.h"
#include "lwip/ip4.h"
#include "lwip/ip6.h"
#include "lwip/inet_chksum.h"
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cstring>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

AppConsolePing::AppConsolePing(struct netif &netif) noexcept
  : netif(netif)
{
  STD_PRINTF("AppConsolePing::AppConsolePing()>\n")
  STD_PRINTF("AppConsolePing::AppConsolePing(): %p\n", static_cast<const void*>(&netif))
  STD_PRINTF("AppConsolePing::AppConsolePing()<\n")
}

AppConsolePing::~AppConsolePing(void) noexcept
{
  STD_PRINTF("AppConsolePing::~AppConsolePing()>\n")
  STD_PRINTF("AppConsolePing::~AppConsolePing()< %p\n", static_cast<const void*>(&netif))
}

std::int32_t AppConsolePing::execute(std::int32_t argc, char *argvPtrs[]) noexcept
{
  std::int32_t status;
  const char *peer_name_string_ptr;
  bool use_ipv6 = false;

  STD_PRINTF("\nAppConsolePing::execute()>\n")

  if (argc == 1) {
    peer_name_string_ptr = AppConsolePing::PING_HOSTNAME_STRING;
  }
  else if (argc > 1) {
    std::int32_t i = 0;
    for (i = 1; i < argc; i++) {
      if (nullptr != argvPtrs[i]) {
        if (0 == std::strncmp("-6", argvPtrs[i], 2)) {
          use_ipv6 = true;
          break;
        }
      }
    }
    if (use_ipv6) {
      if (argc == 2) { /* ping -6 */
        peer_name_string_ptr = AppConsolePing::PING_HOSTNAME_STRING;
        (void) std::printf("%s: (ipv6) default host: %s\n", this->getName(), peer_name_string_ptr);
      }
      else if (i == 1) { /* ping -6 <host> */
        peer_name_string_ptr = argvPtrs[2];
      }
      else { /* ping <host> -6 */
        peer_name_string_ptr = argvPtrs[1];
      }
    }
    else { /* ping <host> */
      peer_name_string_ptr = argvPtrs[1];
    }
  }
  else {
    (void) std::printf("%s: error with bad argc %" PRId32 "!\n", this->getName(), argc);
    return -1;
  }

  (void) std::printf("%s: \"%s\"\n", this->getName(), peer_name_string_ptr);
  if (use_ipv6) {
    status = this->doPing6(reinterpret_cast<const char(&)[255]>(* &peer_name_string_ptr[0]));
  }
  else {
    status = this->doPing(reinterpret_cast<const char(&)[255]>(* &peer_name_string_ptr[0]));
  }
  STD_PRINTF("\nAppConsolePing::execute()<\n")
  return status;
}

std::int32_t AppConsolePing::doPing(const char (&peerNameString)[255]) noexcept
{
  std::int32_t status = -1;
  struct sockaddr_in s_address_in = {sizeof(s_address_in), AF_INET, 0, {0}, {0}};
  std::int32_t responses[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::uint32_t response_size = sizeof(responses) / sizeof(responses[0]);

  STD_PRINTF("\nAppConsolePing::doPing()>\n")

  if (AppLwipService::getHostByName(reinterpret_cast<struct sockaddr *>(&s_address_in), peerNameString) < 0) {
    (void) std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameString);
  }
  else {
    {
      ip4_addr_t ip4_addr;
      char ip4_addr_string[] = {"000.000.000.000"};

      inet_addr_to_ip4addr(&ip4_addr, &s_address_in.sin_addr);
      ip4addr_ntoa_r(&ip4_addr, ip4_addr_string, sizeof(ip4_addr_string));
      (void) std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameString,
                         response_size, ip4_addr_string);
    }
    if (0 == this->icmpPing(reinterpret_cast<struct sockaddr *>(&s_address_in), response_size,
                            AppConsolePing::PING_DELAY_MS,
                            reinterpret_cast<std::int32_t(&)[10]>(responses))) {
      status = 0;
      for (std::uint32_t i = 0U; i < response_size; i++) {
        if (responses[i] >= 0) {
          (void) std::printf("%s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
        }
      }
    }
    else {
      (void) std::printf("%s: failed to ping\n", this->getName());
    }
  }
  STD_PRINTF("\nAppConsolePing::doPing()<\n\n")
  return status;
}

std::int32_t AppConsolePing::doPing6(const char (&peerNameString)[255]) noexcept
{
  std::int32_t status = -1;
  struct sockaddr_in6 s_address_in6 = {sizeof(s_address_in6), AF_INET6, 0U, 0U, {0U, 0U, 0U, 0U}, 0U};
  std::int32_t responses[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::uint32_t response_size = sizeof(responses) / sizeof(responses[0]);

  STD_PRINTF("\nAppConsolePing::doPing6()>\n")

  if (0 > AppLwipService::getHostByName(reinterpret_cast<struct sockaddr *>(&s_address_in6), peerNameString)) {
    (void) std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameString);
  }
  else {
    {
      char ip6_addr_string[] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
      ip6_addr_t ip6_addr;

      inet6_addr_to_ip6addr(&ip6_addr, &s_address_in6.sin6_addr);
      ip6addr_ntoa_r(&ip6_addr, ip6_addr_string, sizeof(ip6_addr_string));
      (void) std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameString,
                         response_size, ip6_addr_string);
    }
    if (0 == this->icmpPing(reinterpret_cast<struct sockaddr *>(&s_address_in6),
                            response_size, AppConsolePing::PING_DELAY_MS,
                            reinterpret_cast<std::int32_t(&)[10]>(responses))) {
      status = 0;
      for (std::uint32_t i = 0U; i < response_size; i++) {
        if (responses[i] >= 0) {
          (void) std::printf("%s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
        }
      }
    }
    else {
      (void) std::printf("%s: failed to ping\n", this->getName());
    }
  }
  STD_PRINTF("\nAppConsolePing::doPing6()<\n\n")
  return status;
}

static const std::uint16_t PING_ID = 0xAFAF;

std::int32_t AppConsolePing::icmpPing(struct sockaddr *sAddrPtr,
                                      std::int32_t count, std::uint32_t timeoutInMs, std::int32_t (&responses)[10]) noexcept
{
  std::int32_t status = 0;
  struct sockaddr *from_ptr = nullptr;
  struct ip_hdr *ip_header_ptr = nullptr;
  struct icmp_echo_hdr *echo_data_ptr = nullptr;
  const std::size_t ping_size = sizeof(struct icmp_echo_hdr) + 32;
  std::int32_t socket;
  std::int32_t reply_min_size = 0;
  std::uint8_t icmp_type;

  STD_PRINTF("\nAppConsolePing::icmpPing()>\n")

  if (AF_INET6 == sAddrPtr->sa_family) {
    socket = lwip_socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    icmp_type = ICMP6_TYPE_EREQ;
    reply_min_size = sizeof(struct ip6_hdr) + sizeof(struct icmp_echo_hdr);
  }
  else {
    socket = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    icmp_type = ICMP_ECHO;
    reply_min_size = sizeof(struct icmp_echo_hdr);
  }
  if (socket < 0) {
    (void) std::printf("%s: lwip_socket() failed\n", this->getName());
    status = -1;
  }
  else if (lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeoutInMs, sizeof(timeoutInMs)) < 0) {
    (void) std::printf("%s: lwip_setsockopt() failed\n", this->getName());
    status = -1;
  }
  else {
    echo_data_ptr = static_cast<struct icmp_echo_hdr *>(mem_malloc(static_cast<mem_size_t>(ping_size)));
    if (echo_data_ptr == NULL) {
      (void) std::printf("%s: mem_malloc() failed\n", this->getName());
      status = -1;
    }
  }
  if (status == 0) {
    static u16_t ping_sequence_number = 1U;
    struct ifreq interface_name = {{""}};
    /* Set interface name like "MS0", "MA0". */
    interface_name.ifr_name[0] = this->netif.name[0];
    interface_name.ifr_name[1] = this->netif.name[1];
    interface_name.ifr_name[2] = '0' + this->netif.num;
    status = lwip_setsockopt(socket, SOL_SOCKET, SO_BINDTODEVICE, static_cast<void *>(&interface_name),
                             sizeof(interface_name));
    for (std::int32_t i = 0; i < count; i++) {
      responses[i] = -1;
      if (echo_data_ptr != NULL) {
        this->preparePingEcho(icmp_type, echo_data_ptr, static_cast<std::uint16_t>(ping_size),
                              static_cast<std::uint16_t>(ping_sequence_number));
      }
#if defined(ENABLE_DEBUG)
      (void) std::printf("AppConsolePing::icmpPing(): TX len: %" PRIu32 "\n", static_cast<std::uint32_t>(ping_size));
#endif /* ENABLE_DEBUG */
      if (lwip_sendto(socket, static_cast<void *>(echo_data_ptr), ping_size, 0, sAddrPtr, sAddrPtr->sa_len) < 0) {
        (void) std::printf("lwip_sendto() failed\n");
        break;
      }
      {
        const std::uint32_t ping_start_time_in_ms = HAL_GetTick();
        do {
          char buf[64 + 40] = {0}; /* extra 40 bytes for IPv6 header */
          socklen_t from_length = 0;
          const std::int32_t length = lwip_recvfrom(socket, buf, sizeof(buf), 0, from_ptr, &from_length);

#if defined(ENABLE_DEBUG)
          (void) std::printf("AppConsolePing::icmpPing(): RX len %" PRId32 " >= %" PRId32 " ?\n", length, reply_min_size);
#endif /* ENABLE_DEBUG */
          if (length >= reply_min_size) {
            const u16_t sequence_number = lwip_htons(ping_sequence_number);
            std::int32_t ip_header_length = 0;
            struct icmp_echo_hdr *echo_header_ptr = NULL;

            ip_header_ptr = reinterpret_cast<struct ip_hdr *>(buf);

            if (AF_INET6 == sAddrPtr->sa_family) {
              ip_header_length = sizeof(struct ip6_hdr);
            }
            else {
              ip_header_length = IPH_HL(ip_header_ptr) * 4;
            }
            echo_header_ptr = reinterpret_cast<struct icmp_echo_hdr *>(buf + ip_header_length);
#if defined(ENABLE_DEBUG)
            (void) std::printf("AppConsolePing::icmpPing(): RC type: %" PRIu32 ", code: %" PRIu32 ", id: 0x%04" PRIX32 ","
                               " seq: %" PRIu32 "\n",
                               static_cast<std::uint32_t>(echo_header_ptr->icmp_type), static_cast<std::uint32_t>(echo_header_ptr->code),
                               static_cast<std::uint32_t>(echo_header_ptr->id), static_cast<std::uint32_t>(echo_header_ptr->seqno));
#endif /* ENABLE_DEBUG */
            if ((echo_header_ptr->id == static_cast<std::uint16_t>(PING_ID)) && (echo_header_ptr->seqno == sequence_number)) {
              if (ICMPH_CODE(echo_header_ptr) == static_cast<std::uint8_t>(ICMP_ER)) {
                const std::uint32_t delta_in_ms = HAL_GetTick() - ping_start_time_in_ms;
                status = 0;
                responses[i] = static_cast<std::int32_t>(delta_in_ms);
                break;
              }
              else {
                (void) std::printf("%s: ICMP other response received\n", this->getName());
              }
            }
          }
          else {
            const std::uint32_t now_ms = HAL_GetTick();
            (void) std::printf("%s: No data start (%" PRId32 "): %" PRIu32 " .. %" PRIu32 "\n", this->getName(),
                               length, ping_start_time_in_ms, now_ms);
          }
        }
        while ((HAL_GetTick() - ping_start_time_in_ms) < timeoutInMs);
      }
      ping_sequence_number++;
    }
    if (echo_data_ptr != NULL) {
      mem_free(echo_data_ptr);
    }
    (void) lwip_close(socket);
  }
  STD_PRINTF("\nAppConsolePing::icmpPing()<\n\n")
  return status;
}

void AppConsolePing::preparePingEcho(std::uint8_t icmpType, struct icmp_echo_hdr *dataPtr, std::uint16_t length,
                                     std::uint16_t sequenceNumber) noexcept
{
  ICMPH_CODE_SET(dataPtr, 0);
  dataPtr->chksum = 0;
  dataPtr->id = PING_ID;
  dataPtr->seqno = static_cast<std::uint16_t>(lwip_htons(sequenceNumber));
  {
    char *wr_ptr = reinterpret_cast<char *>(dataPtr + 1);
    const std::size_t data_length = length - sizeof(struct icmp_echo_hdr);

    for (std::size_t i = 0U; i < data_length; i++) {
      *wr_ptr++ = static_cast<char>(i);
    }
  }
  ICMPH_TYPE_SET(dataPtr, icmpType);
  if (ICMP_ECHO == icmpType) {
    dataPtr->chksum = inet_chksum(dataPtr, length);
  }
}

const char AppConsolePing::PING_HOSTNAME_STRING[255] = {"google.fr"};
