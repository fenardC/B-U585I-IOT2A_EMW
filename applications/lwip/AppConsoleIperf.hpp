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
#include <cstdio>

class AppConsoleIperf final : public Cmd {
  public:
    explicit AppConsoleIperf(void *contextPtr)
      : contextPtr(contextPtr)
      , statePtr(nullptr) {}
  public:
    ~AppConsoleIperf(void) override
    {
      lwiperf_abort(this->statePtr);
    }
  public:
    int32_t execute(int32_t argc, char *argvPtrs[]) override;
  public:
    const char *getComment(void) override
    {
      return "iperf [-s | -c <host>]";
    }
  public:
    const char *getName(void) override
    {
      return "iperf";
    }

  private:
    static void reportFunction(void *argPtr, enum lwiperf_report_type reportType,
                               const ip_addr_t *localAddr, u16_t localPort,
                               const ip_addr_t *remoteAddr, u16_t remotePort,
                               u32_t bytesTransferred, u32_t durationMs, u32_t bandwidthKBitPerSec);
  private:
    static const char *reportTypeToString(enum lwiperf_report_type reportType);
  private:
    void *contextPtr;
  private:
    void *statePtr;
};
