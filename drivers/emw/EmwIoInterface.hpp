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

#include <stdint.h>
#if defined(COMPILATION_WITH_SPI)
#define EMW_USE_SPI_DMA
#define EMW_IO_NAME_STRING "SPI+DMA"
#endif /* COMPILATION_WITH_SPI */

class EmwApiCore;

class EmwIoInterface {
  public:
    EmwIoInterface(void) {}
  public:
    virtual ~EmwIoInterface(void) {}
  public:
    enum InitializationMode {
      eINITIALIZE = 0,
      eRESET = 1
    };
  public:
    virtual void initialize(const class EmwApiCore &core, EmwIoInterface::InitializationMode mode) = 0;
  public:
    virtual void pollData(const class EmwApiCore &core, uint32_t timeoutInMs) = 0;
  public:
    virtual void processPollingData(const class EmwApiCore &core, uint32_t timeoutInMs) = 0;
  public:
    virtual uint16_t send(const class EmwApiCore &core, const uint8_t *dataPtr, uint16_t dataLength) = 0;
  public:
    virtual int8_t unInitialize(const class EmwApiCore &core) = 0;
  public:
    static void pollData(void *THIS, const class EmwApiCore *corePtr, uint32_t timeoutInMs)
    {
      (reinterpret_cast<EmwIoInterface *>(THIS))->pollData(*corePtr, timeoutInMs);
    }
};
