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
#include <cstdio>

class AppConsolePing final : public Cmd {
  public:
    explicit AppConsolePing(void *contextPtr)
      : contextPtr(contextPtr) {}
  public:
    virtual ~AppConsolePing(void) {}
  public:
    int32_t execute(int32_t argc, char *argvPtrs[]) override;
  public:
    const char *getComment(void) override
    {
#if defined(OPTION_WITH_IPV6)
      return "ping [-6] <host> (default is google.fr)";
#else
      return "ping <host> (default is google.fr)";
#endif /* OPTION_WITH_IPV6 */
    }
  public:
    const char *getName(void) override
    {
      return "ping";
    }

  private:
    int32_t doPing(const char *peerNameStringPtr);
  private:
    int32_t doPing6(const char *peerNameStringPtr);


  private:
    void *contextPtr;
  private:
    static const uint32_t PING_DELAY_MS = 100U;
  private:
    static const char PING_HOST_NAME_STING[];
};
