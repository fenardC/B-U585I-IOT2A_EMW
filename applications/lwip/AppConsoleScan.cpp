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
#include "AppConsoleScan.hpp"
#include "wifi_emw.hpp"
#include <inttypes.h>
#include <cstdio>
#include <cstring>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))


AppConsoleScan::AppConsoleScan(void) noexcept
{
  DEBUG_STD_PRINTF(" AppConsoleScan::AppConsoleScan()>\n")
  DEBUG_STD_PRINTF(" AppConsoleScan::AppConsoleScan()<\n")
}

AppConsoleScan::~AppConsoleScan(void)
{
  DEBUG_STD_PRINTF(" AppConsoleScan::~AppConsoleScan()>\n")
  DEBUG_STD_PRINTF(" AppConsoleScan::~AppConsoleScan()<\n")
}

std::int32_t AppConsoleScan::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status;

  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  DEBUG_STD_PRINTF("\n AppConsoleScan::execute()>\n")

  status = Scan(EmwBypass);
  if (0 != status) {
    STD_PRINTF(" %s: operation failed (%" PRIi32 ")!\n", this->getName(), static_cast<std::int32_t>(status));
  }
  DEBUG_STD_PRINTF("\n AppConsoleScan::execute()<\n")
  return status;
}
