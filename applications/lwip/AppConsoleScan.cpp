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
#include "AppConsoleScan.hpp"
#include "WiFiNetwork.hpp"
#include <cstdio>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)


AppConsoleScan::AppConsoleScan(void) noexcept
{
  STD_PRINTF("AppConsoleScan::AppConsoleScan()>\n")
  STD_PRINTF("AppConsoleScan::AppConsoleScan(): %p\n", static_cast<const void*>(&emw))
  STD_PRINTF("AppConsoleScan::AppConsoleScan()<\n")
}

AppConsoleScan::~AppConsoleScan(void)
{
  STD_PRINTF("AppConsoleScan::~AppConsoleScan()>\n")
  STD_PRINTF("AppConsoleScan::~AppConsoleScan()< %p\n", static_cast<const void*>(&emw))
}

std::int32_t AppConsoleScan::execute(std::int32_t argc, char *argvPtrs[]) noexcept
{
  std::int32_t status;
  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  STD_PRINTF("\nAppConsoleScan::execute()>\n")

  status = WiFiNetwork::Scan();
  STD_PRINTF("\nAppConsoleScan::execute()<\n")
  return status;
}
