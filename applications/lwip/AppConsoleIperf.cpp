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
#include "AppConsoleIperf.hpp"
#include "EmwApiEmwBypass.hpp"
#include "lwip/apps/lwiperf.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string_view>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static const char *ReportTypeToString(enum lwiperf_report_type reportType);


AppConsoleIperf::AppConsoleIperf(struct netif &netif) noexcept
  : lwipNetif(netif)
  , statePtr(nullptr)
{
  DEBUG_STD_PRINTF(" AppConsoleIperf::AppConsoleIperf()>\n")
  DEBUG_STD_PRINTF(" AppConsoleIperf::AppConsoleIperf()< %p\n", static_cast<const void*>(&this->lwipNetif))
}

AppConsoleIperf::~AppConsoleIperf(void) noexcept
{
  DEBUG_STD_PRINTF(" AppConsoleIperf::~AppConsoleIperf()> %p\n", static_cast<const void*>(&this->lwipNetif))
  lwiperf_abort(this->statePtr);
  DEBUG_STD_PRINTF(" AppConsoleIperf::~AppConsoleIperf()<\n")
}

std::int32_t AppConsoleIperf::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status = -1;

  DEBUG_STD_PRINTF("\n AppConsoleIperf::execute()>\n")

  if (nullptr == this->statePtr) {
    std::string_view cmd_argv = ((argc >= 2) && argvPtrs[1]) ? argvPtrs[1] : std::string_view{};

    if ((argc == 3) && (cmd_argv == "-c")) {
      std::string_view remote_ip_argv = argvPtrs[2];
      ip_addr_t remote_ip_addr = IPADDR4_INIT(0);

      if (1 == ipaddr_aton(remote_ip_argv.data(), &remote_ip_addr)) {
        this->statePtr = lwiperf_start_tcp_client_default(&remote_ip_addr, &AppConsoleIperf::ReportFunction, this);
        if (nullptr != this->statePtr) {
          status = 0;
          STD_PRINTF(" %s: Started a TCP client with given IP address %s on the default TCP port (5001)\n",
                     this->getName(), ipaddr_ntoa(&remote_ip_addr));
        }
      }
    }
    else if ((argc == 2) && (cmd_argv == "-s")) {
      this->statePtr = lwiperf_start_tcp_server_default(&AppConsoleIperf::ReportFunction, this);

      if (nullptr != this->statePtr) {
        status = 0;
        STD_PRINTF(" %s: Started a TCP server on the default TCP port (5001)\n", this->getName());
      }
    }
    else {
      STD_PRINTF(" %s: %s\n", this->getName(), this->getComment());
    }
  }
  DEBUG_STD_PRINTF("\n AppConsoleIperf::execute()<\n\n")
  return status;
}

void AppConsoleIperf::ReportFunction(void *argPtr, enum lwiperf_report_type reportType,
                                     const ip_addr_t *localAddrPtr, u16_t localPort,
                                     const ip_addr_t *remoteAddrPtr, u16_t remotePort,
                                     u32_t bytesTransferred, u32_t durationInMs, u32_t bandwidthKBitPerSec) noexcept
{
  class AppConsoleIperf *console_ptr = static_cast<class AppConsoleIperf *>(argPtr);

  STD_PRINTF("\n %s\n", ReportTypeToString(reportType));
  STD_PRINTF(" local address    : %s\n", ipaddr_ntoa(localAddrPtr));
  STD_PRINTF(" local port       : %0" PRIu32 "\n", static_cast<std::uint32_t>(localPort));
  STD_PRINTF(" remote address   : %s\n", ipaddr_ntoa(remoteAddrPtr));
  STD_PRINTF(" remote port      : %0" PRIu32 "\n", static_cast<std::uint32_t>(remotePort));
  STD_PRINTF(" bytes transferred: %0" PRIu32 "\n", static_cast<std::uint32_t>(bytesTransferred));
  STD_PRINTF(" duration         : %0" PRIu32 " ms\n", static_cast<std::uint32_t>(durationInMs));
  STD_PRINTF(" bandwidth        : %0" PRIu32 " kBits/s\n", static_cast<std::uint32_t>(bandwidthKBitPerSec));

  console_ptr->statePtr = nullptr;
}

#define CASE(x) case x: { return #x; /*break;*/ }
#define DEFAULT default: { return "UNKNOWN"; /*break;*/ }

static const char *ReportTypeToString(enum lwiperf_report_type reportType)
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
