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
#include <cstring>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

AppConsolePing::AppConsolePing(EmwApiEmw& emw) noexcept
  : emw(emw)
{
  STD_PRINTF("AppConsolePing::AppConsolePing()>\n")
  STD_PRINTF("AppConsolePing::AppConsolePing(): %p\n", static_cast<const void*>(&emw))
  STD_PRINTF("AppConsolePing::AppConsolePing()<\n")
}

AppConsolePing::~AppConsolePing(void) noexcept
{
  STD_PRINTF("AppConsolePing::~AppConsolePing()>\n")
  STD_PRINTF("AppConsolePing::~AppConsolePing()< %p\n", static_cast<const void*>(&emw))
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
  EmwAddress::SockAddrIn_t s_address_in;
  std::int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::uint32_t response_size = sizeof(responses) / sizeof(responses[0]);

  STD_PRINTF("\nAppConsolePing::doPing()>\n")

  if (0 > this->emw.socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t &>(s_address_in), peerNameString)) {
    (void) std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameString);
  }
  else {
    const EmwAddress::IpAddr_t ip4_addr(s_address_in.inAddr.addr);
    char ip4_addr_string[255] = {"000.000.000.000"};

    EmwAddress::NetworkToAscii(ip4_addr, ip4_addr_string, sizeof(ip4_addr_string));
    (void) std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameString,
                       response_size, ip4_addr_string);

    if (0 == this->emw.socketPing(ip4_addr_string, response_size,
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
  EmwAddress::SockAddrIn6_t s_address_in6;
  std::int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::uint32_t response_size = sizeof(responses) / sizeof(responses[0]);
  const EmwAddress::AddrInfo_t hints(0, EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
  EmwAddress::AddrInfo_t result;
  const char empty[255] = {""};

  STD_PRINTF("\nAppConsolePing::doPing6()>\n")

  if (0 > this->emw.socketGetAddrInfo(peerNameString, empty, hints, result)) {
    (void) std::printf("%s: failed to find the host name \"%s\"\n", this->getName(), peerNameString);
  }
  else {
    char ip6_addr_string[255] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
    const EmwAddress::Ip6Addr_t ip6_addr(result.sAddr.data2[1], result.sAddr.data2[2],
                                         result.sAddr.data3[0], result.sAddr.data3[1]);

    EmwAddress::NetworkToAscii(ip6_addr, ip6_addr_string, sizeof(ip6_addr_string));
    (void) std::printf("%s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameString,
                       response_size, ip6_addr_string);
    if (0 == this->emw.socketPing6(ip6_addr_string,
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

const char AppConsolePing::PING_HOSTNAME_STRING[255] = {"google.fr"};
