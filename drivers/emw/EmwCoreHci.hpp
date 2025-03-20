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

#include "EmwNetworkStack.hpp"
#include "EmwOsInterface.hpp"
#include <stdint.h>

class EmwApiCore;

class EmwCoreHci {
  private:
    EmwCoreHci(void) {};
  public:
    static void free(EmwNetworkStack::Buffer_t *networkBufferPtr);
  public:
    static void initialize(const class EmwApiCore *corePtr);
  public:
    static void input(EmwNetworkStack::Buffer_t *networkBufferPtr);
  public:
    static EmwNetworkStack::Buffer_t *receive(uint32_t timeoutInMs);
  public:
    static int32_t send(const class EmwApiCore &core, const uint8_t *payloadPtr, uint16_t payloadLength);
  public:
    static int32_t unInitialize(void);

  private:
    static EmwOsInterface::Queue_t NetworkPacketFifo;
};
