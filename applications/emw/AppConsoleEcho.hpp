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
#include "EmwAddress.hpp"

class EmwApiEmw;

class AppConsoleEcho final : public Cmd {
  public:
    explicit AppConsoleEcho(EmwApiEmw& emw) noexcept;
  public:
    virtual ~AppConsoleEcho(void) noexcept override;
  public:
    std::int32_t execute(std::int32_t argc, char *argvPtrs[]) noexcept override;
  public:
    const char *getComment(void) const noexcept override
    {
      return "echo [-cCount] [-6] <ip>";
    }
  public:
    const char *getName(void) const noexcept override
    {
      return "echo";
    }

  private:
    std::uint32_t checkBufferIn(std::uint32_t untilN, std::uint32_t offset) const noexcept;
  private:
    std::int32_t doEcho(const EmwAddress::SockAddrIn_t &sAddressIn, std::uint32_t loop, std::uint32_t untilN) noexcept;
  private:
    std::int32_t doEcho6(const EmwAddress::SockAddrIn6_t &sAddressIn6, std::uint32_t loop, std::uint32_t untilN) noexcept;
  private:
    std::int32_t doEchoExchanges(std::int32_t socket, std::uint32_t loop, std::uint32_t untilN) noexcept;
  private:
    void fillBufferOut(std::uint32_t untilN) noexcept;

  private:
    EmwApiEmw &emw;

  private:
    static const char REMOTE_IP_ADDRESS_STRING[];
  private:
    static const char REMOTE_IP6_ADDRESS_STRING[];
  private:
    static const std::uint16_t REMOTE_TCP_PORT = 7U;
  private:
    static const std::uint32_t ITERATION_COUNT = 10U;
  private:
    static const std::int32_t TIMEOUT_10S_DEFINED = 10000;
  private:
    static const std::uint32_t TRANSFER_SIZE = 2000U;

  private:
    std::uint8_t bufferIn[TRANSFER_SIZE + AppConsoleEcho::ITERATION_COUNT];
  private:
    std::uint8_t bufferOut[TRANSFER_SIZE + AppConsoleEcho::ITERATION_COUNT];
};
