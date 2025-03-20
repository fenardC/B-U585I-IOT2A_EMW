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

#include <cstdint>

#if defined(COMPILATION_WITH_SPI)
#define EMW_USE_SPI_DMA
#define EMW_IO_NAME_STRING "SPI+DMA"
#endif /* COMPILATION_WITH_SPI */

namespace EmwIoInterfaceTypes {
  enum InitializationMode {
    eINITIALIZE = 0,
    eRESET = 1
  };
}

template <class EmwIo> class EmwIoInterface {
  protected:
    EmwIoInterface(void) noexcept {}
  public:
    virtual ~EmwIoInterface(void) noexcept {}
  public:

  public:
    void initialize(EmwIoInterfaceTypes::InitializationMode mode) noexcept
    {
      static_cast<EmwIo *>(this)->initializeImp(mode);
    }
  public:
    void pollData(std::uint32_t timeoutInMs) noexcept
    {
      static_cast<EmwIo *>(this)->pollDataImp(timeoutInMs);
    }
  public:
    void processPollingData(std::uint32_t timeoutInMs) noexcept
    {
      static_cast<EmwIo *>(this)->processPollingDataImp(timeoutInMs);
    }
  public:
    std::uint16_t send(const std::uint8_t *dataPtr, std::uint16_t dataLength) noexcept
    {
      return static_cast<EmwIo *>(this)->sendImp(dataPtr, dataLength);
    }
  public:
    std::int8_t unInitialize(void) noexcept
    {
      return static_cast<EmwIo *>(this)->unInitializeImp();
    }
  public:
    static void PollData(void *THIS, const void *argumentPtr, std::uint32_t timeoutInMs) noexcept
    {
      static_cast<void>(argumentPtr);

      (static_cast<EmwIo *>(THIS))->pollData(timeoutInMs);
    }
};
