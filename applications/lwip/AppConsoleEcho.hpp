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
#include "lwip/sockets.h"

class AppConsoleEcho final : public Cmd {
  public:
    explicit AppConsoleEcho(void *contextPtr)
      : contextPtr(contextPtr), bufferIn{0U}, bufferOut{0U} {}
  public:
    virtual ~AppConsoleEcho(void) override {}
  public:
    int32_t execute(int32_t argc, char *argvPtrs[]) override;
  public:
    const char *getComment(void) const override
    {
      return "echo [-cCount] [-6] <host>";
    }
  public:
    const char *getName(void) const override
    {
      return "echo";
    }

  private:
    uint32_t checkBuffer(uint8_t buffer[], uint32_t n, uint32_t offset);
  private:
    int32_t doEcho(const struct sockaddr_in &sAddressIn, uint32_t loop, uint32_t untilN);
  private:
    int32_t doEcho6(const struct sockaddr_in6 &sAddressIn6, uint32_t loop, uint32_t untilN);
  private:
    int32_t doEchoExchanges(int32_t sock, uint32_t loop, uint32_t untilN);
  private:
    void fillBuffer(uint8_t *bufferPtr, uint32_t n);
  private:
    uint16_t hostToNetworkShort(uint16_t hostShort)
    {
      return (((static_cast<uint16_t>(hostShort) & 0xFF00U) >> 8U) |
              ((static_cast<uint16_t>(hostShort) & 0x00FFU) << 8U));
    }
  private:
    void *contextPtr;

  private:
    static const char REMOTE_IP_ADDRESS_STRING[];
  private:
    static const char REMOTE_IP6_ADDRESS_STRING[];
  private:
    static const uint16_t REMOTE_TCP_PORT = 7U;
  private:
    static const uint32_t ITERATION_COUNT = 10U;
  private:
    static const int32_t TIMEOUT_10S_DEFINED = 10000;
  private:
    static const uint32_t TRANSFER_SIZE = 2000U;

  private:
    uint8_t bufferIn[TRANSFER_SIZE + AppConsoleEcho::ITERATION_COUNT];
  private:
    uint8_t bufferOut[TRANSFER_SIZE + AppConsoleEcho::ITERATION_COUNT];
};
