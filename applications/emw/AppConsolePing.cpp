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
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "stm32u5xx_hal.h"
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cstdbool>
#include <cstring>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsolePing::execute(int32_t argc, char *argvPtrs[])
{
  int32_t status;
  const char *peer_name_string_ptr;
  bool use_ipv6 = false;
  STD_PRINTF("\nAppConsolePing::execute()>\n")

  if (argc == 1) {
    peer_name_string_ptr = AppConsolePing::PING_HOST_NAME_STING;
  }
  else if (argc > 1) {
    int32_t i = 0;
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
        peer_name_string_ptr = AppConsolePing::PING_HOST_NAME_STING;
        std::printf("%s: (ipv6) default host: %s\n", this->getName(), peer_name_string_ptr);
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
    std::printf("%s: error with bad argc %" PRId32 "!\n", this->getName(), argc);
    return -1;
  }
  std::printf("%s: \"%s\"\n", this->getName(), peer_name_string_ptr);
  if (use_ipv6) {
    status =  this->doPing6(peer_name_string_ptr);
  }
  else {
    status =  this->doPing(peer_name_string_ptr);
  }
  return status;
}

int32_t AppConsolePing::doPing(const char *peerNameStringPtr)
{
  int32_t status = -1;
  EmwAddress::SockAddrIn_t s_addr_in;
  int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const uint32_t response_size = sizeof(responses) / sizeof(responses[0]);
  class EmwApiEmw *const emw_ptr = static_cast<class EmwApiEmw *>(this->contextPtr);

  STD_PRINTF("\nAppConsolePing::doPing()>\n")

  if (0 > emw_ptr->socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t *>(&s_addr_in), peerNameStringPtr)) {
    std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameStringPtr);
  }
  else {
    const EmwAddress::IpAddr_t ip4_addr(s_addr_in.inAddr.addr);
    char ip4_addr_string[] = {"000.000.000.000"};

    EmwAddress::networkToAscii(ip4_addr, ip4_addr_string, sizeof(ip4_addr_string));
    std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameStringPtr,
                response_size, ip4_addr_string);
    if (0 == emw_ptr->socketPing(ip4_addr_string, response_size, AppConsolePing::PING_DELAY_MS, responses)) {
      status = 0;
      for (uint32_t i = 0U; i < response_size; i++) {
        if (responses[i] >= 0) {
          std::printf("%s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
        }
      }
    }
    else {
      std::printf("%s: failed to ping\n", this->getName());
    }
  }
  STD_PRINTF("\nAppConsolePing::doPing()<\n\n")
  return status;
}

int32_t AppConsolePing::doPing6(const char *peerNameStringPtr)
{
  int32_t status = -1;
  EmwAddress::SockAddrIn6_t s_addr_in6;
  int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const uint32_t response_size = sizeof(responses) / sizeof(responses[0]);
  class EmwApiEmw *const emw_ptr = static_cast<class EmwApiEmw *>(this->contextPtr);
  const EmwAddress::AddrInfo_t hints(0, EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
  EmwAddress::AddrInfo_t result;

  STD_PRINTF("\nAppConsolePing::doPing6()>\n")

  if (0 > emw_ptr->socketGetAddrInfo(peerNameStringPtr, nullptr, hints, &result)) {
    std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameStringPtr);
  }
  else {
    char ip6_addr_string[] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
    const EmwAddress::Ip6Addr_t ip6_addr(result.sAddr.data2[1], result.sAddr.data2[2],
                                         result.sAddr.data3[0], result.sAddr.data3[1]);

    EmwAddress::networkToAscii(ip6_addr, ip6_addr_string, sizeof(ip6_addr_string));
    std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameStringPtr,
                response_size, ip6_addr_string);
    if (0 == emw_ptr->socketPing6(ip6_addr_string, response_size, AppConsolePing::PING_DELAY_MS, responses)) {
      status = 0;
      for (uint32_t i = 0U; i < response_size; i++) {
        if (responses[i] >= 0) {
          std::printf("%s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
        }
      }
    }
    else {
      std::printf("%s: failed to ping\n", this->getName());
    }
  }
  STD_PRINTF("\nAppConsolePing::doPing6()<\n\n")
  return status;
}

const char AppConsolePing::PING_HOST_NAME_STING[] = {"google.fr"};
