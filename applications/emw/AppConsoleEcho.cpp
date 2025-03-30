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
#include <inttypes.h>
#include <stdbool.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <system_error>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsoleEcho::execute(int32_t argc, char *argvPtrs[])
{
  const char *server_name_string_ptr = AppConsoleEcho::REMOTE_IP_ADDRESS_STRING;
  const uint16_t port = AppConsoleEcho::REMOTE_TCP_PORT;
  EmwAddress::SockAddrIn_t s_address_in;
  int count = 1;
  STD_PRINTF("\nAppConsoleEcho::AppConsoleEcho()>\n")

  if (argc > 1) {
    int32_t i;
    for (i = 1; i < argc; i++) {
      if (nullptr != argvPtrs[i]) {
        if (0 == strncmp("-c", argvPtrs[i], 2)) {
          count = atoi(argvPtrs[i] + 2);
        }
        else {
          server_name_string_ptr = argvPtrs[i];
        }
      }
    }
  }
  std::printf("%s: <%s>\n", this->getName(), server_name_string_ptr);
  {
    EmwAddress::IpAddr_t address;

    EmwAddress::asciiToNetwork(server_name_string_ptr, address);
    s_address_in.port = this->hostToNetworkShort(port);
    s_address_in.inAddr.addr = address.addr;
  }
  for (int i = 1; i <= count; i++) {
    std::printf("\n\n*************** %" PRIi32 "/%" PRIi32 " ***************\n", (int32_t)i, (int32_t)count);
    for (auto transfer_size = 1000U; transfer_size <= AppConsoleEcho::TRANSFER_SIZE; transfer_size += 100) {
      {
        this->doEcho(s_address_in, AppConsoleEcho::ITERATION_COUNT, transfer_size);
      }
    }
  }
  STD_PRINTF("\nAppConsoleEcho::AppConsoleEcho()<\n\n")
  return 0;
}

uint32_t AppConsoleEcho::checkBuffer(uint8_t buffer[], uint32_t n, uint32_t offset)
{
  uint32_t error_count = 0;
  for (uint32_t i = 0; i < n; i++) {
    if (buffer[i] != ((i + offset) & 0x000000FF)) {
      std::printf("%s: ### received data are different from data sent "
                  "\"%" PRIu32 "\" <> \"%" PRIu32 "\" (%c) at index %" PRIu32 "\n",
                  this->getName(), (uint32_t)(i & 0xff), (uint32_t)buffer[i], (char)buffer[i], (uint32_t)i);
      error_count++;
    }
  }
  return error_count;
}

int32_t AppConsoleEcho::doEcho(const EmwAddress::SockAddrIn_t &sAddressIn, uint32_t loop, uint32_t untilN)
{
  class EmwApiEmw *const emw_ptr = static_cast<EmwApiEmw *>(this->contextPtr);
  EmwAddress::IpAddr_t ip_addr(sAddressIn.inAddr.addr);
  char ip_addr_string[] = {"000.000.000.000"};
  int32_t status = 0;

  EmwAddress::networkToAscii(ip_addr, ip_addr_string, sizeof(ip_addr_string));
  std::printf("\nAppConsoleEcho::doEcho()> \"%s\"\n", ip_addr_string);
  try {
    const int32_t sock = emw_ptr->socketCreate(EMW_AF_INET, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
    if (sock < 0) {
      STD_PRINTF("%s: failed to create the client socket (%" PRId32 ")\n", this->getName(), sock)
      throw std::runtime_error("lwip_socket() failed");
    }
    {
      const int32_t connect_status = emw_ptr->socketConnect(sock,
                                     reinterpret_cast<const EmwAddress::SockAddr_t *>(&sAddressIn),
                                     sizeof(sAddressIn));

      if (EmwApiBase::eEMW_STATUS_OK != connect_status) {
        STD_PRINTF("%s: failed to connect with the socket (%" PRId32 ")\n", this->getName(),
                   (int32_t)connect_status);
        throw std::runtime_error("socketConnect() failed");
      }
    }
    std::printf("%s: device connected to %s\n", this->getName(), ip_addr_string);
    status = this->doEchoExchanges(sock, loop, untilN);
  }
  catch (const std::runtime_error &error) {
    std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
  }
  return status;
}

int32_t AppConsoleEcho::doEchoExchanges(int32_t sock, uint32_t loop, uint32_t untilN)
{
  class EmwApiEmw *const emw_ptr = static_cast<EmwApiEmw *>(this->contextPtr);
  std::printf("%s: starting transfers ", this->getName());
  try {
    this->fillBuffer(this->bufferOut, untilN + loop);
    {
      uint32_t transfer = 0;
      uint32_t error_count = 0;
      const auto tstart = HAL_GetTick();
      auto tstop = HAL_GetTick();
      for (uint32_t i = 0; i < loop; i++) {
        {
          uint32_t transfer_out = 0;
          uint32_t retries_max = 0;
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
            transfer_out += (uint32_t)count_done;
          }
          while ((transfer_out < untilN) && (retries_max < 10));
          transfer += transfer_out;
        }
        memset(this->bufferIn, 0x00, sizeof(this->bufferIn));
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
            transfer_in += (uint32_t)count_done;
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
        std::printf("\n%s: failed, found %" PRId32 " different bytes\n", this->getName(), error_count);
      }
    }
  }
  catch (const std::runtime_error &error) {
    std::printf("%s: %s\n", this->getName(), error.what());
  }
  (void) emw_ptr->socketShutDown(sock, 2);
  (void) emw_ptr->socketClose(sock);
  STD_PRINTF("\nAppConsoleEcho::doEcho()<\n\n")
  return 0;
}

void AppConsoleEcho::fillBuffer(uint8_t *bufferPtr, uint32_t n)
{
  for (uint32_t i = 0; i < n; i++) {
    *bufferPtr++ = (uint8_t)i;
  }
}

const char AppConsoleEcho::REMOTE_IP_ADDRESS_STRING[] = {"192.168.1.139"};
