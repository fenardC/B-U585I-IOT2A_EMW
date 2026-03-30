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
#pragma once

#include "EmwApiCore.hpp"
#if defined(COMPILATION_WITH_EMW)
#include "EmwApiEmw.hpp"
#elif defined(COMPILATION_WITH_LWIP)
#include "EmwApiEmwBypass.hpp"
#endif /* COMPILATION_WITH_EMW) */


#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

void InitializeEmw(EmwApiCore &emw) noexcept;
std::int32_t Scan(EmwApiCore &emw) noexcept;

#if defined(COMPILATION_WITH_EMW)
extern class EmwApiEmw Emw;
#elif defined(COMPILATION_WITH_LWIP)
extern class EmwApiEmwBypass EmwBypass;
#endif /* COMPILATION_WITH_EMW) */
