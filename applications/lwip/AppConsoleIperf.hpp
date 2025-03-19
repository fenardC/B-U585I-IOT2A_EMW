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
#pragma once

#include "Console.hpp"
#include "lwip/apps/lwiperf.h"

struct netif;

class AppConsoleIperf final : public Cmd {
  public:
    explicit AppConsoleIperf(struct netif &netif) noexcept;
  public:
    ~AppConsoleIperf(void) noexcept override;
  public:
    std::int32_t execute(std::int32_t argc, char *argvPtrs[]) noexcept override;
  public:
    const char *getComment(void) const noexcept override
    {
      return "iperf [-s | -c <ip>]";
    }
  public:
    const char *getName(void) const noexcept override
    {
      return "iperf";
    }

  private:
    static void ReportFunction(void *argPtr, enum lwiperf_report_type reportType,
                               const ip_addr_t *localAddrPtr, u16_t localPort,
                               const ip_addr_t *remoteAddrPtr, u16_t remotePort,
                               u32_t bytesTransferred, u32_t durationInMs, u32_t bandwidthKBitPerSec) noexcept;
  private:
    struct netif &netif;
  private:
    void *statePtr;
};
