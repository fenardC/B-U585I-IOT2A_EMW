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
#include "AppConsoleStats.hpp"
#include "lwip/stats.h"
#include <inttypes.h>
#include <cstdio>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

AppConsoleStats::AppConsoleStats(void) noexcept
{
  STD_PRINTF("AppConsoleStats::AppConsoleStats()>\n")
  STD_PRINTF("AppConsoleStats::AppConsoleStats()<\n")
}

AppConsoleStats::~AppConsoleStats(void)
{
  STD_PRINTF("AppConsoleStats::~AppConsoleStats()>\n")
  STD_PRINTF("AppConsoleStats::~AppConsoleStats()<\n")
}

std::int32_t AppConsoleStats::execute(std::int32_t argc, char *argvPtrs[]) noexcept
{
  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  STD_PRINTF("AppConsoleStats::execute()>\n")
#if LWIP_STATS && LWIP_STATS_DISPLAY
  stats_display();
#endif /* LWIP_STATS && LWIP_STATS_DISPLAY */
  STD_PRINTF("AppConsoleStats::execute()<\n")
  return 0;
}
