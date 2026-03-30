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
#include "AppConsolePing.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <string_view>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

AppConsolePing::AppConsolePing(EmwApiEmw& wifiEmw) noexcept
  : emw(wifiEmw)
{
  DEBUG_STD_PRINTF(" AppConsolePing::AppConsolePing()>\n")
  DEBUG_STD_PRINTF(" AppConsolePing::AppConsolePing()< %p\n", static_cast<const void*>(&this->emw))
}

AppConsolePing::~AppConsolePing(void) noexcept
{
  DEBUG_STD_PRINTF(" AppConsolePing::~AppConsolePing()>\n")
  DEBUG_STD_PRINTF(" AppConsolePing::~AppConsolePing()< %p\n", static_cast<const void*>(&this->emw))
}

std::int32_t AppConsolePing::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status;
  const char *peer_name_string_ptr = nullptr;
  bool use_ipv6 = false;

  DEBUG_STD_PRINTF("\n AppConsolePing::execute()>\n")

  if (argc <= 0) {
    STD_PRINTF("%s: error with bad argc %" PRId32 "!\n", this->getName(), argc);
    DEBUG_STD_PRINTF("\n AppConsolePing::execute()<\n")
    return -1;
  }

  for (std::int32_t i = 1; i < argc; ++i) {
    std::string_view argv = argvPtrs[i] ? argvPtrs[i] : "";

    if (argv == "-6") {
      use_ipv6 = true;
    }
    else if (false == argv.empty()) {
      if (peer_name_string_ptr != nullptr) {
        STD_PRINTF(" %s: error: multiple hosts provided: \"%s\" and \"%s\"\n",
                   this->getName(), peer_name_string_ptr, argv.data());
        DEBUG_STD_PRINTF("\nAppConsolePing::execute()<\n")
        return -1;
      }
      peer_name_string_ptr = argv.data();
    }
  }

  if (nullptr == peer_name_string_ptr) {
    peer_name_string_ptr = AppConsolePing::PING_HOSTNAME_STRING;
    STD_PRINTF(" %s: default host: \"%s\"\n", this->getName(), peer_name_string_ptr);
  }

  STD_PRINTF(" %s:%s \"%s\"\n", this->getName(), use_ipv6 ? " (ipv6)" : "", peer_name_string_ptr);
  {
    char peer_name_string[255] = "";

    std::strncpy(peer_name_string, peer_name_string_ptr, sizeof(peer_name_string));
    peer_name_string[254] = '\0';
    if (use_ipv6) {
      status = this->doPing6(peer_name_string);
    }
    else {
      status = this->doPing(peer_name_string);
    }
  }
  DEBUG_STD_PRINTF("\n AppConsolePing::execute()<\n")
  return status;
}

std::int32_t AppConsolePing::doPing(const char (&peerNameString)[255]) noexcept
{
  std::int32_t status = -1;
  EmwAddress::SockAddrIn_t sock_address_in;
  std::int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::uint32_t response_size = sizeof(responses) / sizeof(responses[0]);

  DEBUG_STD_PRINTF("\n AppConsolePing::doPing()>\n")

  if (0 > this->emw.socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t &>(sock_address_in), peerNameString)) {
    STD_PRINTF(" %s: failed to find the host name \"%s\"\n", this->getName(), peerNameString);
  }
  else {
    const EmwAddress::IpAddr_t ip4_addr(sock_address_in.inAddr.addr);
    char ip4_addr_string[16] = {"000.000.000.000"};

    EmwAddress::NetworkToAscii(ip4_addr, ip4_addr_string);
    STD_PRINTF(" %s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameString,
               response_size, ip4_addr_string);

    if (0 == this->emw.socketPing(ip4_addr_string, response_size,
                                  AppConsolePing::PING_DELAY_MS,
                                  reinterpret_cast<std::int32_t(&)[10]>(responses))) {
      status = 0;
      for (std::uint32_t i = 0U; i < response_size; i++) {
        if (responses[i] >= 0) {
          STD_PRINTF(" %s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
        }
      }
    }
    else {
      STD_PRINTF(" %s: failed to ping\n", this->getName());
    }
  }
  DEBUG_STD_PRINTF("\n AppConsolePing::doPing()<\n\n")
  return status;
}

std::int32_t AppConsolePing::doPing6(const char (&peerNameString)[255]) noexcept
{
  std::int32_t status = -1;
  std::int32_t responses[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  const std::uint32_t response_size = sizeof(responses) / sizeof(responses[0]);
  const EmwAddress::AddrInfo_t hints(0, EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
  EmwAddress::AddrInfo_t result;
  const char empty[255] = {""};

  DEBUG_STD_PRINTF("\n AppConsolePing::doPing6()>\n")

  if (0 > this->emw.socketGetAddrInfo(peerNameString, empty, hints, result)) {
    STD_PRINTF(" %s: Failed to find the host name \"%s\"\n", this->getName(), peerNameString);
  }
  else {
    char ip6_addr_string[40] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
    const EmwAddress::Ip6Addr_t ip6_addr(result.sAddr.data2[1], result.sAddr.data2[2],
                                         result.sAddr.data3[0], result.sAddr.data3[1]);

    EmwAddress::NetworkToAscii(ip6_addr, ip6_addr_string);
    STD_PRINTF(" %s: pinging \"%s\" %" PRId32 " times: with \"%s\"\n", this->getName(), peerNameString,
               response_size, ip6_addr_string);

    if (0 == this->emw.socketPing6(ip6_addr_string,
                                   response_size, AppConsolePing::PING_DELAY_MS,
                                   reinterpret_cast<std::int32_t(&)[10]>(responses))) {
      status = 0;
      for (std::uint32_t i = 0U; i < response_size; i++) {
        if (responses[i] >= 0) {
          STD_PRINTF(" %s: iteration #%" PRIu32 " round trip %" PRId32 "\n", this->getName(), i, responses[i]);
        }
      }
    }
    else {
      STD_PRINTF(" %s: failed to ping\n", this->getName());
    }
  }
  DEBUG_STD_PRINTF("\n AppConsolePing::doPing6()<\n\n")
  return status;
}

const char AppConsolePing::PING_HOSTNAME_STRING[255] = {"google.fr"};
