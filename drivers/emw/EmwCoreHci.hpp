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

#include "EmwIoInterface.hpp"
#if defined(EMW_USE_SPI_DMA)
#include "EmwIoSpi.hpp"
#endif /* EMW_USE_SPI_DMA) */
#include "EmwNetworkStack.hpp"
#include "EmwOsInterface.hpp"

#include <cstdint>

class EmwCoreHci final {
  private:
    EmwCoreHci(void) noexcept {};
  public:
    static void Free(EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
  public:
    static void Initialize(void) noexcept;
  public:
    static void Input(EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
  public:
    static EmwNetworkStack::Buffer_t *Receive(std::uint32_t timeoutInMs) noexcept;
  public:
    static void ResetIo(void) noexcept;
  public:
    static std::int32_t Send(const std::uint8_t *payloadPtr, std::uint16_t payloadLength) noexcept;
  public:
    static void UnInitialize(void) noexcept;

#if defined(EMW_USE_SPI_DMA)
  private:
    static EmwIoSpi IoSpi;
  private:
    static EmwIoInterface<EmwIoSpi> &Io;
#endif /* EMW_USE_SPI_DMA) */

  private:
    static EmwOsInterface::Queue_t NetworkPacketFifo;
};
