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
#include "AppConsoleEcho.hpp"
#include "EmwApiEmw.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <system_error>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort);


AppConsoleEcho::AppConsoleEcho(EmwApiEmw& wifiEmw) noexcept
  : emw(wifiEmw)
  , bufferIn{0U}
  , bufferOut{0U}
{
  DEBUG_STD_PRINTF(" AppConsoleEcho::AppConsoleEcho()>\n")
  DEBUG_STD_PRINTF(" AppConsoleEcho::AppConsoleEcho()< %p\n", static_cast<const void*>(&this->emw))
}

AppConsoleEcho::~AppConsoleEcho(void)
{
  DEBUG_STD_PRINTF(" AppConsoleEcho::~AppConsoleEcho()>\n")
  DEBUG_STD_PRINTF(" AppConsoleEcho::~AppConsoleEcho()< %p\n", static_cast<const void*>(&this->emw))
}

std::int32_t AppConsoleEcho::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status = -1;
  const char *server_name_string_ptr = nullptr;
  const std::uint16_t port = AppConsoleEcho::REMOTE_TCP_PORT;
  EmwAddress::SockAddrIn_t sock_address_in;
  EmwAddress::SockAddrIn6_t sock_address_in6;
  int count = 1;
  bool use_ipv6 = false;

  DEBUG_STD_PRINTF("\n AppConsoleEcho::execute()>\n")

  for (std::int32_t i = 1; i < argc; ++i) {
    std::string_view argv = argvPtrs[i] ? argvPtrs[i] : "";

    if (argv == "-6") {
      use_ipv6 = true;
    }
    else if (argv.starts_with("-c")) {
      auto count_argv = argv.substr(2);
      if (false == count_argv.empty()) {
        count = static_cast<std::uint32_t>(std::atoi(count_argv.data()));
      }
    }
    else if (false == argv.empty()) {
      if (server_name_string_ptr != nullptr) {
        STD_PRINTF(" %s: error: multiple hosts provided: \"%s\" and \"%s\"\n",
                   this->getName(), server_name_string_ptr, argv.data());
        DEBUG_STD_PRINTF("\n AppConsoleEcho::execute()<\n\n")
        return -1;
      }
      server_name_string_ptr = argv.data();
    }
  }

  if (nullptr == server_name_string_ptr) {
    if (use_ipv6) {
      server_name_string_ptr = AppConsoleEcho::REMOTE_IP6_ADDRESS_STRING;
    }
    else {
      server_name_string_ptr = AppConsoleEcho::REMOTE_IP_ADDRESS_STRING;
    }
    STD_PRINTF(" %s: default request: \"%s\"\n", this->getName(), server_name_string_ptr);
  }

  STD_PRINTF(" %s: <%s>\n", this->getName(), server_name_string_ptr);
  if (use_ipv6) {
    EmwAddress::Ip6Addr_t address;

    EmwAddress::AsciiToNetwork(server_name_string_ptr, address);
    sock_address_in6.port = HostToNetworkShort(port);
    sock_address_in6.in6Addr.un.u32Addr[0] = address.addr[0];
    sock_address_in6.in6Addr.un.u32Addr[1] = address.addr[1];
    sock_address_in6.in6Addr.un.u32Addr[2] = address.addr[2];
    sock_address_in6.in6Addr.un.u32Addr[3] = address.addr[3];
  }
  else {
    EmwAddress::IpAddr_t address;

    EmwAddress::AsciiToNetwork(server_name_string_ptr, address);
    sock_address_in.port = HostToNetworkShort(port);
    sock_address_in.inAddr.addr = address.addr;
  }

  for (int i = 1; i <= count; i++) {
    STD_PRINTF("\n\n *************** %" PRIi32 "/%" PRIi32 " ***************\n",
               static_cast<std::int32_t>(i), static_cast<std::int32_t>(count));
    for (auto transfer_size = 1000U; transfer_size <= AppConsoleEcho::TRANSFER_SIZE; transfer_size += 100U) {
      if (use_ipv6) {
        status = this->doEcho6(sock_address_in6, AppConsoleEcho::ITERATION_COUNT, transfer_size);
      }
      else {
        status = this->doEcho(sock_address_in, AppConsoleEcho::ITERATION_COUNT, transfer_size);
      }
    }
  }
  DEBUG_STD_PRINTF("\n AppConsoleEcho::execute()<\n\n")
  return status;
}

std::uint32_t AppConsoleEcho::checkBufferIn(std::uint32_t untilN, std::uint32_t offset) const noexcept
{
  std::uint32_t error_count = 0;

  for (std::uint32_t i = 0U; i < untilN; i++) {
    if (this->bufferIn[i] != ((i + offset) & 0x000000FF)) {
      STD_PRINTF(" %s: ### received data are different from data sent "
                 "\"%" PRIu32 "\" <> \"%" PRIu32 "\" (%c) at index %" PRIu32 "\n",
                 this->getName(), static_cast<std::uint32_t>(i & 0xff),
                 static_cast<std::uint32_t>(this->bufferIn[i]), static_cast<char>(this->bufferIn[i]),
                 static_cast<std::uint32_t>(i));
      error_count++;
    }
  }
  return error_count;
}

std::int32_t AppConsoleEcho::doEcho(const EmwAddress::SockAddrIn_t &sockAddressIn, std::uint32_t loop,
                                    std::uint32_t untilN) noexcept
{
  std::int32_t status = 0;
  char ip4_addr_string[16] = {"000.000.000.000"};
  std::int32_t socket = -1;

  {
    EmwAddress::IpAddr_t ip4_addr(sockAddressIn.inAddr.addr);

    EmwAddress::NetworkToAscii(ip4_addr, ip4_addr_string);
    DEBUG_STD_PRINTF("\n AppConsoleEcho::doEcho()> \"%s\"\n", ip4_addr_string);
  }
  try {
    socket = this->emw.socketCreate(EMW_AF_INET, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
    if (socket < 0) {
      throw std::runtime_error("lwip_socket() failed");
    }
    {
      const std::int32_t connect_status = this->emw.socketConnect(socket,
                                          reinterpret_cast<const EmwAddress::SockAddr_t &>(sockAddressIn),
                                          sizeof(sockAddressIn));
      if (0 != connect_status) {
        throw std::runtime_error("socketConnect() failed");
      }
    }
    STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip4_addr_string);
    status = this->doEchoExchanges(socket, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
    static_cast<void>(this->emw.socketClose(socket));
  }
  return status;
}

std::int32_t AppConsoleEcho::doEcho6(const EmwAddress::SockAddrIn6_t &sockAddressIn6, std::uint32_t loop,
                                     std::uint32_t untilN) noexcept
{
  std::int32_t status = 0;
  char ip6_addr_string[40] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
  std::int32_t socket = -1;

  {
    EmwAddress::Ip6Addr_t ip6_addr(sockAddressIn6.in6Addr.un.u32Addr[0], sockAddressIn6.in6Addr.un.u32Addr[1],
                                   sockAddressIn6.in6Addr.un.u32Addr[2], sockAddressIn6.in6Addr.un.u32Addr[3]);

    EmwAddress::NetworkToAscii(ip6_addr, ip6_addr_string);
    DEBUG_STD_PRINTF("\n AppConsoleEcho::doEcho6()> \"%s\"\n", ip6_addr_string);
  }
  try {
    socket = this->emw.socketCreate(EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
    if (socket < 0) {
      throw std::runtime_error("socketCreate() failed");
    }
    {
      const std::int32_t connect_status = this->emw.socketConnect(socket,
                                          reinterpret_cast<const EmwAddress::SockAddr_t &>(sockAddressIn6),
                                          sizeof(sockAddressIn6));
      if (0 != connect_status) {
        throw std::runtime_error("socketConnect() failed");
      }
    }
    STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip6_addr_string);
    status = this->doEchoExchanges(socket, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
    static_cast<void>(this->emw.socketClose(socket));
  }
  return status;
}

std::int32_t AppConsoleEcho::doEchoExchanges(std::int32_t socket, std::uint32_t loop, std::uint32_t untilN) noexcept
{
  std::int32_t status = 0;

  STD_PRINTF(" %s: starting transfers ", this->getName());
  try {
    this->fillBufferOut(untilN + loop);
    {
      std::uint32_t transfer = 0U;
      std::uint32_t error_count = 0U;
      const auto tstart = HAL_GetTick();
      auto tstop = HAL_GetTick();

      for (std::uint32_t i = 0U; i < loop; i++) {
        {
          std::uint32_t transfer_out = 0U;
          std::uint32_t retries_max = 0U;
          do {
            std::int32_t count_done = this->emw.socketSend(socket,
                                      reinterpret_cast<std::uint8_t (&)[]>(* &this->bufferOut[i + transfer_out]),
                                      untilN - transfer_out, 0);
            STD_PRINTF(".");
            if (count_done < 0) {
              STD_PRINTF(" %s: failed to send data to echo server (%" PRId32 "), try again\n",
                         this->getName(), count_done);
              count_done = 0;
              retries_max++;
            }
            transfer_out += static_cast<std::uint32_t>(count_done);
          }
          while ((transfer_out < untilN) && (retries_max < 10));
          transfer += transfer_out;
        }
        std::memset(this->bufferIn, 0x00, sizeof(this->bufferIn));
        {
          std::uint32_t transfer_in = 0U;
          do {
            std::int32_t count_done = this->emw.socketReceive(socket,
                                      reinterpret_cast<std::uint8_t (&)[]>(* &this->bufferIn[transfer_in]),
                                      untilN - transfer_in, 0);
            std::printf("x");
            if (count_done < 0) {
              STD_PRINTF("\n %s: receive failed with (%" PRId32 ")\n", this->getName(), count_done);
              this->bufferIn[sizeof(this->bufferIn) / 8] = 0;
              STD_PRINTF("  <--- \"%s\"\n\n", this->bufferIn);
              throw std::runtime_error("socketReceive() failed");
            }
            transfer_in += static_cast<std::uint32_t>(count_done);
          }
          while (transfer_in < untilN);
          tstop = HAL_GetTick();
          error_count = this->checkBufferIn(untilN, i);
          transfer += transfer_in;
        }
      }
      if (error_count == 0) {
        STD_PRINTF("\n %s: successful echo transfer and receive %" PRIu32 " x %" PRIu32 " with %" PRIu32 " bytes"\
                   " in %" PRId32 " ms, bit rate = %" PRId32 " Kbit/sec\n" \
                   , this->getName(), loop, untilN, transfer, tstop - tstart, (transfer * 8) / (tstop - tstart));
      }
      else {
        status = -1;
        STD_PRINTF("\n %s: failed, found %" PRId32 " different bytes\n", this->getName(), error_count);
      }
    }
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
  }
  static_cast<void>(this->emw.socketShutDown(socket, 2));
  static_cast<void>(this->emw.socketClose(socket));
  DEBUG_STD_PRINTF("\n AppConsoleEcho::doEchoExchanges()<\n\n")
  return status;
}

void AppConsoleEcho::fillBufferOut(std::uint32_t untilN) noexcept
{
  for (std::uint32_t i = 0U; i < untilN; i++) {
    this->bufferOut[i] = static_cast<std::uint8_t>(i);
  }
}

const char AppConsoleEcho::REMOTE_IP_ADDRESS_STRING[] = {"192.168.1.9"};
//const char AppConsoleEcho::REMOTE_IP_ADDRESS_STRING[] = {"echo.mbedcloudtesting.com"};
const char AppConsoleEcho::REMOTE_IP6_ADDRESS_STRING[] = {"2001:861:3881:3e70:4998:30e5:9660:843"};

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort)
{
  return (((static_cast<std::uint16_t>(hostShort) & 0xFF00U) >> 8U) |
          ((static_cast<std::uint16_t>(hostShort) & 0x00FFU) << 8U));
}
