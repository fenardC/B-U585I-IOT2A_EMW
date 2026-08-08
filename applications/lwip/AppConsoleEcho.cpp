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
#include "lwip/sockets.h"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <system_error>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

AppConsoleEcho::AppConsoleEcho(void) noexcept
  : bufferIn{0U}
  , bufferOut{0U} {
  DEBUG_STD_PRINTF(" AppConsoleEcho::AppConsoleEcho()>\n")
  DEBUG_STD_PRINTF(" AppConsoleEcho::AppConsoleEcho()<\n")
}

AppConsoleEcho::~AppConsoleEcho(void)
{
  DEBUG_STD_PRINTF(" AppConsoleEcho::~AppConsoleEcho()>\n")
  DEBUG_STD_PRINTF(" AppConsoleEcho::~AppConsoleEcho()<\n")
}

std::int32_t AppConsoleEcho::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status = -1;
  const char *server_name_string_ptr = nullptr;
  const std::uint16_t port = AppConsoleEcho::REMOTE_TCP_PORT;
  struct sockaddr_in sock_address_in {};
  struct sockaddr_in6 sock_address_in6 {};
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
    sock_address_in6.sin6_len = sizeof(sock_address_in6);
    sock_address_in6.sin6_family = AF_INET6;
    {
      ip6_addr_t address;

      ip6addr_aton(server_name_string_ptr, &address);
      inet6_addr_from_ip6addr(&sock_address_in6.sin6_addr, &address);
    }
    sock_address_in6.sin6_port = lwip_htons(port);
  }
  else {
    sock_address_in.sin_len = sizeof(sock_address_in);
    sock_address_in.sin_family = AF_INET;
    {
      ip4_addr_t address;

      ip4addr_aton(server_name_string_ptr, &address);
      inet_addr_from_ip4addr(&sock_address_in.sin_addr, &address);
    }
    sock_address_in.sin_port = lwip_htons(port);
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

std::int32_t AppConsoleEcho::doEcho(const struct sockaddr_in &sockAddressIn, std::uint32_t loop,
                                    std::uint32_t untilN) noexcept
{
  std::int32_t status = 0;
  char ip4_addr_string[16] = {"000.000.000.000"};
  std::int32_t socket = -1;

  {
    ip4_addr_t ip4_addr;

    inet_addr_to_ip4addr(&ip4_addr, &sockAddressIn.sin_addr);
    ip4addr_ntoa_r(&ip4_addr, ip4_addr_string, sizeof(ip4_addr_string));
    DEBUG_STD_PRINTF("\n AppConsoleEcho::doEcho()> \"%s\"\n", ip4_addr_string);
  }
  try {
    socket = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == socket) {
      throw std::runtime_error("lwip_socket() failed");
    }
    {
      const std::int32_t timeout_in_ms = AppConsoleEcho::TIMEOUT_10S_DEFINED;
      std::int32_t socket_status = lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
      if (socket_status == -1) {
        throw std::runtime_error("lwip_setsockopt() for receiving failed");
      }
      socket_status = lwip_setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
      if (socket_status == -1) {
        throw std::runtime_error("lwip_setsockopt() for sending failed");
      }
    }
    {
      const std::int32_t connect_status = lwip_connect(socket,
                                          reinterpret_cast<const struct sockaddr *>(&sockAddressIn),
                                          sizeof(sockAddressIn));
      if (0 != connect_status) {
        throw std::runtime_error("lwip_connect() failed");
      }
    }
    STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip4_addr_string);
    status = this->doEchoExchanges(socket, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
    static_cast<void>(lwip_close(socket));
  }
  return status;
}

std::int32_t AppConsoleEcho::doEcho6(const struct sockaddr_in6 &sockAddressIn6, std::uint32_t loop,
                                     std::uint32_t untilN) noexcept
{
  std::int32_t status = 0;
  char ip6_addr_string[40] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
  std::int32_t socket = -1;

  {
    ip6_addr_t ip6_addr;
    inet6_addr_to_ip6addr(&ip6_addr, &sockAddressIn6.sin6_addr);
    ip6addr_ntoa_r(&ip6_addr, ip6_addr_string, sizeof(ip6_addr_string));
    DEBUG_STD_PRINTF("\n AppConsoleEcho::doEcho6()> \"%s\"\n", ip6_addr_string);
  }
  try {
    socket = lwip_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == socket) {
      throw std::runtime_error("lwip_socket() failed");
    }
    {
      const std::int32_t timeout_in_ms = AppConsoleEcho::TIMEOUT_10S_DEFINED;
      std::int32_t socket_status = lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
      if (socket_status == -1) {
        throw std::runtime_error("lwip_setsockopt() for receiving failed");
      }
      socket_status = lwip_setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
      if (socket_status == -1) {
        throw std::runtime_error("lwip_setsockopt() for sending failed");
      }
    }
    {
      const std::int32_t connect_status = lwip_connect(socket,
                                          reinterpret_cast<const struct sockaddr *>(&sockAddressIn6),
                                          sizeof(sockAddressIn6));
      if (0 != connect_status) {
        throw std::runtime_error("lwip_connect() failed");
      }
    }
    STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip6_addr_string);
    status = this->doEchoExchanges(socket, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
    static_cast<void>(lwip_close(socket));
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
          do {
            std::int32_t count_done = lwip_send(socket,
                                                &this->bufferOut[i + transfer_out],
                                                untilN - transfer_out, 0);
            STD_PRINTF(".");
            if (count_done < 0) {
              STD_PRINTF(" %s: failed to send data to echo server (%" PRId32 "), try again\n",
                         this->getName(), count_done);
              count_done = 0;
            }
            transfer_out += static_cast<std::uint32_t>(count_done);
          }
          while (transfer_out < untilN);
          transfer += transfer_out;
        }
        std::memset(this->bufferIn, 0x00, sizeof(this->bufferIn));
        {
          std::uint32_t transfer_in = 0U;
          do {
            std::int32_t count_done = lwip_recv(socket,
                                                &this->bufferIn[transfer_in],
                                                untilN - transfer_in, 0);
            std::printf("x");
            if (count_done < 0) {
              STD_PRINTF("\n %s: receive failed with (%" PRId32 ")\n", this->getName(), count_done);
              this->bufferIn[sizeof(this->bufferIn) / 8] = 0;
              STD_PRINTF("  <--- \"%s\"\n\n", this->bufferIn);
              throw std::runtime_error("lwip_recv() failed");
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
  static_cast<void>(lwip_shutdown(socket, SHUT_RDWR));
  static_cast<void>(lwip_close(socket));
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
