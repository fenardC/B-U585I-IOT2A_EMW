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

struct netif;

class AppConsolePing final : public Cmd {
  public:
    explicit AppConsolePing(struct netif &netif) noexcept;
  public:
    virtual ~AppConsolePing(void) noexcept override;
  public:
    std::int32_t execute(std::int32_t argc, char *argvPtrs[]) noexcept override;
  public:
    const char *getComment(void) const noexcept override
    {
      return "ping [-6] <hostname> (default is google.fr)";
    }
  public:
    const char *getName(void) const noexcept override
    {
      return "ping";
    }

  private:
    std::int32_t doPing(const char (&peerNameString)[255]) noexcept;
  private:
    std::int32_t doPing6(const char (&peerNameString)[255]) noexcept;
  private:
    std::int32_t icmpPing(struct sockaddr *sAddrPtr,
                          std::int32_t count, std::uint32_t timeoutInMs, std::int32_t (&responses)[10]) noexcept;
  private:
    void preparePingEcho(std::uint8_t icmpType, struct icmp_echo_hdr *dataPtr, std::uint16_t length,
                         std::uint16_t sequenceNumber) noexcept;

  private:
    struct netif &netif;
  private:
    static const std::uint32_t PING_DELAY_MS = 500U;
  private:
    static const char PING_HOSTNAME_STRING[255];
};
