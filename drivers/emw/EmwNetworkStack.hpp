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

#if defined(COMPILATION_WITH_EMW)
#define EMW_NETWORK_EMW_MODE
#define NETWORK_NAME_STRING "Network on EMW"
#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif /* __PACKED_STRUCT */
typedef __PACKED_STRUCT {
  std::uint32_t length;
  std::uint32_t headerLength;
  std::uint8_t data[1];
} EmwBuffer_t;

#elif defined(COMPILATION_WITH_LWIP)
#include "lwip/pbuf.h"

#define EMW_NETWORK_BYPASS_MODE
#define NETWORK_NAME_STRING "Network on STM32"
#endif /* COMPILATION_WITH_LWIP */

class EmwNetworkStack final {
  public:
#if defined(EMW_NETWORK_EMW_MODE)
    typedef EmwBuffer_t Buffer_t;
#elif defined(EMW_NETWORK_BYPASS_MODE)
    typedef struct pbuf Buffer_t;
#endif /* EMW_NETWORK_BYPASS_MODE */

  private:
    EmwNetworkStack(void) {};
  public:
    static EmwNetworkStack::Buffer_t *AllocBuffer(void) noexcept;
  public:
    static void FreeBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept;
  public:
    static std::uint8_t *GetBufferPayload(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept;
  public:
    static std::uint32_t GetBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept;
  public:
    static void HideBufferHeader(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept;
  public:
    static void SetBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr, std::uint32_t size) noexcept;
  public:
    static const std::uint16_t NETWORK_BUFFER_SIZE;
  public:
    static const std::uint16_t NETWORK_IPC_PAYLOAD_SIZE;
#if defined(COMPILATION_WITH_LWIP)
  public:
    static const std::uint16_t NETWORK_MTU_SIZE;
#endif /* COMPILATION_WITH_LWIP */
};
