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

class EmwApiEmw;

class AppConsolePing final : public Cmd {
  public:
    explicit AppConsolePing(EmwApiEmw& emw) noexcept;
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
    EmwApiEmw &emw;
  private:
    static const std::uint32_t PING_DELAY_MS = 100U;
  private:
    static const char PING_HOSTNAME_STRING[255];
};
