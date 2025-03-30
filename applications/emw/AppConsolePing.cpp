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
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsolePing::execute(int32_t argc, char *argvPtrs[])
{
  const char *peer_name_string_ptr;
  bool use_ipv6 = false;
  STD_PRINTF("\nAppConsolePing::execute()>\n")

  if (argc == 1) {
    peer_name_string_ptr = AppConsolePing::PING_HOST_NAME_STING;
  }
  else if (argc > 1) {
    int32_t i = 0;
#if defined(OPTION_WITH_IPV6)
    for (i = 1; i < argc; i++) {
      if (nullptr != argvPtrs[i]) {
        if (0 == strncmp("-6", argvPtrs[i], 2)) {
          use_ipv6 = true;
          break;
        }
      }
    }
#endif /* OPTION_WITH_IPV6 */
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
    return this->doPing6(peer_name_string_ptr);
  }
  else {
    return this->doPing(peer_name_string_ptr);
  }
}

int32_t AppConsolePing::doPing(const char *peerNameStringPtr)
{
  EmwAddress::SockAddrIn_t s_addr_in;
  int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const uint32_t response_size = sizeof(responses) / sizeof(responses[0]);
  class EmwApiEmw *const emw_ptr = static_cast<class EmwApiEmw *>(this->contextPtr);
  char peer_ip_addr_string[] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
  const size_t peer_ip_addr_string_size = sizeof(peer_ip_addr_string);
  STD_PRINTF("\nAppConsolePing::doPing()>\n") {
    if (0 > emw_ptr->socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t *>(&s_addr_in), peerNameStringPtr)) {
      std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameStringPtr);
      return -1;
    }
    else {
      const EmwAddress::IpAddr_t ip_addr(s_addr_in.inAddr.addr);
      EmwAddress::networkToAscii(ip_addr, peer_ip_addr_string, peer_ip_addr_string_size);
    }
  }
  std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameStringPtr,
              response_size, peer_ip_addr_string);
  {
    int32_t status;
    status = emw_ptr->socketPing(peer_ip_addr_string, response_size, AppConsolePing::PING_DELAY_MS, responses);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("%s: failed to ping (%" PRId32 ")\n", this->getName(), (int32_t)status);
    }
    else {
      for (uint32_t i = 0U; i < response_size ; ++i) {
        std::printf("%s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
      }
    }
  }
  STD_PRINTF("\nAppConsolePing::doPing()<\n\n")
  return 0;
}

int32_t AppConsolePing::doPing6(const char *peerNameStringPtr)
{
  EmwAddress::SockAddrIn6_t s_addr_in6;
  int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const uint32_t response_size = sizeof(responses) / sizeof(responses[0]);
  class EmwApiEmw *const emw_ptr = static_cast<class EmwApiEmw *>(this->contextPtr);
  char peer_ip_addr_string[] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
  const size_t peer_ip_addr_string_size = sizeof(peer_ip_addr_string);
  STD_PRINTF("\nAppConsolePing::doPing6()>\n") {
    EmwAddress::AddrInfo_t hints(0, EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
    EmwAddress::AddrInfo_t result;
    if (0 > emw_ptr->socketGetAddrInfo(peerNameStringPtr, nullptr, &hints, &result)) {
      std::printf("%s: Failed to find the host name \"%s\"\n", this->getName(), peerNameStringPtr);
      return -1;
    }
    else {
      const EmwAddress::Ip6Addr_t ip6_addr(result.sAddr.data2[1], result.sAddr.data2[2],
                                           result.sAddr.data3[0], result.sAddr.data3[1]);
      EmwAddress::networkToAscii(ip6_addr, peer_ip_addr_string, peer_ip_addr_string_size);
    }
  }

  std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameStringPtr,
              response_size, peer_ip_addr_string);
  {
    int32_t status;
    status = emw_ptr->socketPing6(peer_ip_addr_string, response_size, AppConsolePing::PING_DELAY_MS, responses);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("%s: failed to ping (%" PRId32 ")\n", this->getName(), (int32_t)status);
    }
    else {
      for (uint32_t i = 0U; i < response_size ; ++i) {
        std::printf("%s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
      }
    }
  }
  STD_PRINTF("\nAppConsolePing::doPing6()<\n\n")
  return 0;
}

const char AppConsolePing::PING_HOST_NAME_STING[] = {"google.fr"};
