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
#include "AppConsoleEcho.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstdbool>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <system_error>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsoleEcho::execute(int32_t argc, char *argvPtrs[])
{
  int32_t status = -1;
  const char *server_name_string_ptr = AppConsoleEcho::REMOTE_IP_ADDRESS_STRING;
  const uint16_t port = AppConsoleEcho::REMOTE_TCP_PORT;
  EmwAddress::SockAddrIn_t s_address_in;
  EmwAddress::SockAddrIn6_t s_address_in6;
  int count = 1;
  bool use_ipv6 = false;

  STD_PRINTF("\nAppConsoleEcho::execute()>\n")
  if (argc == 1) {
    server_name_string_ptr = AppConsoleEcho::REMOTE_IP_ADDRESS_STRING;
  }
  else if (argc > 1) {
    int32_t i;
    for (i = 1; i < argc; i++) {
      if (nullptr != argvPtrs[i]) {
        if (0 == std::strncmp("-6", argvPtrs[i], 2)) {
          use_ipv6 = true;
          server_name_string_ptr = AppConsoleEcho::REMOTE_IP6_ADDRESS_STRING;
        }
        else if (0 == std::strncmp("-c", argvPtrs[i], 2)) {
          count = std::atoi(argvPtrs[i] + 2);
        }
        else {
          server_name_string_ptr = argvPtrs[i];
        }
      }
    }
  }
  std::printf("%s: <%s>\n", this->getName(), server_name_string_ptr);
  if (use_ipv6) {
    EmwAddress::Ip6Addr_t address;
    EmwAddress::asciiToNetwork(server_name_string_ptr, address);
    s_address_in6.port = this->hostToNetworkShort(port);
    s_address_in6.in6Addr.un.u32Addr[0] = address.addr[0];
    s_address_in6.in6Addr.un.u32Addr[1] = address.addr[1];
    s_address_in6.in6Addr.un.u32Addr[2] = address.addr[2];
    s_address_in6.in6Addr.un.u32Addr[3] = address.addr[3];
  }
  else {
    EmwAddress::IpAddr_t address;
    EmwAddress::asciiToNetwork(server_name_string_ptr, address);
    s_address_in.port = this->hostToNetworkShort(port);
    s_address_in.inAddr.addr = address.addr;
  }
  for (int i = 1; i <= count; i++) {
    std::printf("\n\n*************** %" PRIi32 "/%" PRIi32 " ***************\n",
                static_cast<int32_t>(i), static_cast<int32_t>(count));
    for (auto transfer_size = 1000U; transfer_size <= AppConsoleEcho::TRANSFER_SIZE; transfer_size += 100) {
      if (use_ipv6) {
        status = this->doEcho6(s_address_in6, AppConsoleEcho::ITERATION_COUNT, transfer_size);
      }
      else {
        status = this->doEcho(s_address_in, AppConsoleEcho::ITERATION_COUNT, transfer_size);
      }
    }
  }
  STD_PRINTF("\nAppConsoleEcho::execute()<\n\n")
  return status;
}

uint32_t AppConsoleEcho::checkBuffer(uint8_t buffer[], uint32_t n, uint32_t offset)
{
  uint32_t error_count = 0;
  for (uint32_t i = 0; i < n; i++) {
    if (buffer[i] != ((i + offset) & 0x000000FF)) {
      std::printf("%s: ### received data are different from data sent "
                  "\"%" PRIu32 "\" <> \"%" PRIu32 "\" (%c) at index %" PRIu32 "\n",
                  this->getName(), static_cast<uint32_t>(i & 0xff),
                  static_cast<uint32_t>(buffer[i]), static_cast<char>(buffer[i]), static_cast<uint32_t>(i));
      error_count++;
    }
  }
  return error_count;
}

int32_t AppConsoleEcho::doEcho(const EmwAddress::SockAddrIn_t &sAddressIn, uint32_t loop, uint32_t untilN)
{
  int32_t status = 0;
  char ip4_addr_string[] = {"000.000.000.000"};
  int32_t sock = -1;
  class EmwApiEmw *const emw_ptr = static_cast<EmwApiEmw *>(this->contextPtr);

  {
    EmwAddress::IpAddr_t ip4_addr(sAddressIn.inAddr.addr);
    EmwAddress::networkToAscii(ip4_addr, ip4_addr_string, sizeof(ip4_addr_string));
    std::printf("\nAppConsoleEcho::doEcho()> \"%s\"\n", ip4_addr_string);
  }
  try {
    sock = emw_ptr->socketCreate(EMW_AF_INET, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
    if (sock < 0) {
      throw std::runtime_error("lwip_socket() failed");
    }
    {
      const int32_t connect_status = emw_ptr->socketConnect(sock,
                                     reinterpret_cast<const EmwAddress::SockAddr_t *>(&sAddressIn),
                                     sizeof(sAddressIn));
      if (0 != connect_status) {
        throw std::runtime_error("socketConnect() failed");
      }
    }
    std::printf("%s: device connected to %s\n", this->getName(), ip4_addr_string);
    status = this->doEchoExchanges(sock, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
    (void) emw_ptr->socketClose(sock);
  }
  return status;
}

int32_t AppConsoleEcho::doEcho6(const EmwAddress::SockAddrIn6_t &sAddressIn6, uint32_t loop, uint32_t untilN)
{
  int32_t status = 0;
  char ip6_addr_string[] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
  int32_t sock = -1;
  class EmwApiEmw *const emw_ptr = static_cast<EmwApiEmw *>(this->contextPtr);

  {
    EmwAddress::Ip6Addr_t ip6_addr(sAddressIn6.in6Addr.un.u32Addr[0], sAddressIn6.in6Addr.un.u32Addr[1],
                                   sAddressIn6.in6Addr.un.u32Addr[2], sAddressIn6.in6Addr.un.u32Addr[3]);
    EmwAddress::networkToAscii(ip6_addr, ip6_addr_string, sizeof(ip6_addr_string));
    std::printf("\nAppConsoleEcho::doEcho6()> \"%s\"\n", ip6_addr_string);
  }
  try {
    sock = emw_ptr->socketCreate(EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
    if (sock < 0) {
      throw std::runtime_error("lwip_socket() failed");
    }
    {
      const int32_t connect_status = emw_ptr->socketConnect(sock,
                                     reinterpret_cast<const EmwAddress::SockAddr_t *>(&sAddressIn6),
                                     sizeof(sAddressIn6));
      if (0 != connect_status) {
        throw std::runtime_error("socketConnect() failed");
      }
    }
    std::printf("%s: device connected to %s\n", this->getName(), ip6_addr_string);
    status = this->doEchoExchanges(sock, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
    (void) emw_ptr->socketClose(sock);
  }
  return status;
}

int32_t AppConsoleEcho::doEchoExchanges(int32_t sock, uint32_t loop, uint32_t untilN)
{
  int32_t status = 0;
  class EmwApiEmw *const emw_ptr = static_cast<EmwApiEmw *>(this->contextPtr);

  std::printf("%s: starting transfers ", this->getName());
  try {
    this->fillBuffer(this->bufferOut, untilN + loop);
    {
      uint32_t transfer = 0U;
      uint32_t error_count = 0U;
      const auto tstart = HAL_GetTick();
      auto tstop = HAL_GetTick();

      for (uint32_t i = 0; i < loop; i++) {
        {
          uint32_t transfer_out = 0U;
          uint32_t retries_max = 0U;
          do {
            int32_t count_done = emw_ptr->socketSend(sock, &this->bufferOut[i + transfer_out],
                                 untilN - transfer_out, 0);
            std::printf(".");
            if (count_done < 0) {
              std::printf("%s: failed to send data to echo server (%" PRId32 "), try again\n",
                          this->getName(), count_done);
              count_done = 0;
              retries_max++;
            }
            transfer_out += static_cast<uint32_t>(count_done);
          }
          while ((transfer_out < untilN) && (retries_max < 10));
          transfer += transfer_out;
        }
        std::memset(this->bufferIn, 0x00, sizeof(this->bufferIn));
        {
          uint32_t transfer_in = 0U;
          do {
            int32_t count_done = emw_ptr->socketReceive(sock, &this->bufferIn[transfer_in],
                                 untilN - transfer_in, 0);
            std::printf("x");
            if (count_done < 0) {
              std::printf("\n%s: receive failed with (%" PRId32 ")\n", this->getName(), count_done);
              this->bufferIn[sizeof(this->bufferIn) / 8] = 0;
              std::printf("  <--- \"%s\"\n\n", this->bufferIn);
              throw std::runtime_error("socketReceive() failed");
            }
            transfer_in += static_cast<uint32_t>(count_done);
          }
          while (transfer_in < untilN);
          tstop = HAL_GetTick();
          error_count = this->checkBuffer(this->bufferIn, untilN, i);
          transfer += transfer_in;
        }
      }
      if (error_count == 0) {
        std::printf("\n%s: successful echo transfer and receive %" PRIu32 " x %" PRIu32 " with %" PRIu32 " bytes"\
                    " in %" PRId32 " ms, bit rate = %" PRId32 " Kbit/sec\n" \
                    , this->getName(), loop, untilN, transfer, tstop - tstart, (transfer * 8) / (tstop - tstart));
      }
      else {
        status = -1;
        std::printf("\n%s: failed, found %" PRId32 " different bytes\n", this->getName(), error_count);
      }
    }
  }
  catch (const std::runtime_error &error) {
    std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
  }
  (void) emw_ptr->socketShutDown(sock, 2);
  (void) emw_ptr->socketClose(sock);
  STD_PRINTF("\nAppConsoleEcho::doEcho()<\n\n")
  return status;
}

void AppConsoleEcho::fillBuffer(uint8_t *bufferPtr, uint32_t n)
{
  for (uint32_t i = 0; i < n; i++) {
    *bufferPtr++ = static_cast<uint8_t>(i);
  }
}

const char AppConsoleEcho::REMOTE_IP_ADDRESS_STRING[] = {"192.168.1.19"};
//const char AppConsoleEcho::REMOTE_IP_ADDRESS_STRING[] = {"echo.mbedcloudtesting.com"};
const char AppConsoleEcho::REMOTE_IP6_ADDRESS_STRING[] = {"2001:861:3881:3e70:4998:30e5:9660:843"};
