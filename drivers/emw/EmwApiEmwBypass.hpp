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

#include "EmwApiBase.hpp"
#include "EmwApiCore.hpp"
#include <stdint.h>

class EmwApiEmwBypass final : public EmwApiCore {
  public:
    EmwApiEmwBypass(void) : EmwApiCore() {}
  public:
    ~EmwApiEmwBypass(void) {}
  public:
    EmwApiBase::Status setMode(int32_t enable,
                               EmwApiBase::NetlinkInputCallback_t netlinkInputCallback, void *userArgsPtr);
  public:
    EmwApiBase::Status output(uint8_t data[], int32_t length, int32_t interface) const;
};
