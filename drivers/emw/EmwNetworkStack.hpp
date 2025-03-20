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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "cmsis_compiler.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */
#include <cstdint>
#include <cstdbool>

#if defined(COMPILATION_WITH_EMW)
#define EMW_NETWORK_EMW_MODE
#define NETWORK_NAME_STRING "Network on module"
__PACKED_STRUCT buffer_s;
#endif /* COMPILATION_WITH_EMW */

#if defined(COMPILATION_WITH_LWIP)
#define EMW_NETWORK_BYPASS_MODE
#define NETWORK_NAME_STRING "Network on STM32"
struct buffer_s;
#endif /* COMPILATION_WITH_LWIP */

class EmwNetworkStack final {
  public:
    typedef buffer_s Buffer_t;
  private:
    EmwNetworkStack(void) {};
  public:
    static EmwNetworkStack::Buffer_t *allocBuffer(void);
  public:
    static void freeBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr);
  public:
    static uint8_t *getBufferPayload(EmwNetworkStack::Buffer_t *networkPacketPtr);
  public:
    static uint32_t getBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr);
  public:
    static void hideBufferHeader(EmwNetworkStack::Buffer_t *networkPacketPtr);
  public:
    static void setBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr, uint32_t size);
  public:
    static const uint16_t NETWORK_BUFFER_SIZE;
  public:
    static const uint16_t NETWORK_IPC_PAYLOAD_SIZE;
#if defined(COMPILATION_WITH_LWIP)
  public:
    static const uint16_t NETWORK_MTU_SIZE;
#endif /* COMPILATION_WITH_LWIP */
};
