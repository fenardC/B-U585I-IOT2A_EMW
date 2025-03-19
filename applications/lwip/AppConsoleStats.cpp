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
#include <cstdio>
#include "lwip/stats.h"

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsoleStats::execute(int32_t argc, char *argvPtrs[])
{
  static_cast<void>(argc);
  static_cast<void>(argvPtrs);
  STD_PRINTF("\nAppConsoleStats::execute()>\n")
#if LWIP_STATS && LWIP_STATS_DISPLAY
  stats_display();
#endif /* LWIP_STATS && LWIP_STATS_DISPLAY */
  STD_PRINTF("\AppConsoleStats::execute()<\n\n")
  return 0;
}
