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
#include "AppConsoleIperf.hpp"
#include "EmwApiEmwBypass.hpp"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <cstdbool>
#include "lwip/apps/lwiperf.h"
#include <cstring>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsoleIperf::execute(int32_t argc, char *argvPtrs[])
{
  int32_t status = -1;
  STD_PRINTF("\nAppConsoleIperf::execute()>\n")

  if (nullptr == this->statePtr) {
    if ((argc == 3) && (0 == std::strncmp("-c", argvPtrs[1], strlen(argvPtrs[1])))) {
      const char *remote_addr_ptr;
      ip_addr_t remote_addr = IPADDR4_INIT(0);

      remote_addr_ptr = argvPtrs[2];
      if (1 == ipaddr_aton(remote_addr_ptr, &remote_addr)) {
        this->statePtr = lwiperf_start_tcp_client_default(&remote_addr, &AppConsoleIperf::reportFunction, this);
        if (nullptr != this->statePtr) {
          status = 0;
          std::printf("%s: Started a TCP client with given IP address %s on the default TCP port (5001)\n",
                      this->getName(), ipaddr_ntoa(&remote_addr));
        }
      }
    }
    else if ((argc == 2) && (0 == std::strncmp("-s", argvPtrs[1], strlen(argvPtrs[1])))) {
      AppConsoleIperf::statePtr = lwiperf_start_tcp_server_default(&AppConsoleIperf::reportFunction, this);

      if (nullptr != this->statePtr) {
        status = 0;
        std::printf("%s: Started a TCP server on the default TCP port (5001)\n", this->getName());
      }
    }
    else {
      std::printf("%s: %s\n", this->getName(), this->getComment());
    }
  }
  STD_PRINTF("\AppConsoleIperf::execute()<\n\n")
  return status;
}

void AppConsoleIperf::reportFunction(void *argPtr, enum lwiperf_report_type reportType,
                                     const ip_addr_t *localAddr, u16_t localPort,
                                     const ip_addr_t *remoteAddr, u16_t remotePort,
                                     u32_t bytesTransferred, u32_t durationInMs, u32_t bandwidthKBitPerSec)
{
  class AppConsoleIperf *console_ptr = static_cast<class AppConsoleIperf *>(argPtr);

  std::printf("\n%s\n", AppConsoleIperf::reportTypeToString(reportType));
  std::printf("local address    : %s\n", ipaddr_ntoa(localAddr));
  std::printf("local port       : %0" PRIu32 "\n", static_cast<uint32_t>(localPort));
  std::printf("remote address   : %s\n", ipaddr_ntoa(remoteAddr));
  std::printf("remote port      : %0" PRIu32 "\n", static_cast<uint32_t>(remotePort));
  std::printf("bytes transferred: %0" PRIu32 "\n", static_cast<uint32_t>(bytesTransferred));
  std::printf("duration         : %0" PRIu32 " ms\n", static_cast<uint32_t>(durationInMs));
  std::printf("bandwidth        : %0" PRIu32 " kBits/s\n", static_cast<uint32_t>(bandwidthKBitPerSec));
  console_ptr->statePtr = nullptr;
}

#define CASE(x) case x: { return #x; /*break;*/ }
#define DEFAULT default: { return "UNKNOWN"; /*break;*/ }

const char *AppConsoleIperf::reportTypeToString(enum lwiperf_report_type reportType)
{
  switch (reportType) {
      CASE(LWIPERF_TCP_DONE_SERVER);
      CASE(LWIPERF_TCP_DONE_CLIENT);
      CASE(LWIPERF_TCP_ABORTED_LOCAL);
      CASE(LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
      CASE(LWIPERF_TCP_ABORTED_LOCAL_TXERROR);
      CASE(LWIPERF_TCP_ABORTED_REMOTE);
      DEFAULT;
  }
}
