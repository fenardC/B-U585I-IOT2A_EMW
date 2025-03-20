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
#include "EmwNetworkStack.hpp"
#include "emw_conf.hpp"
#include "lwip/pbuf.h"

#if !defined(EMW_IO_DEBUG)
#define DEBUG_IO_LOG(...)
#endif /* EMW_IO_DEBUG */

#define EMW_NETWORK_MTU_SIZE_DEFINED                (1500U)
#define EMW_NETWORK_PBUF_LINK_HLEN_DEFINED          (14U)
#if (EMW_NETWORK_PBUF_LINK_HLEN_DEFINED != PBUF_LINK_HLEN)
#error "EMW_NETWORK_PBUF_LINK_HLEN_DEFINED must be equal to PBUF_LINK_HLEN."
#endif /* EMW_NETWORK_PBUF_LINK_HLEN */

#define EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED      (28U)
#if (EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED != PBUF_LINK_ENCAPSULATION_HLEN)
#error "EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED must be equal to PBUF_LINK_ENCAPSULATION_HLEN."
#endif /* EMW_NETWORK_BYPASS_HEADER_SIZE */

static struct pbuf *EmwBufferToLwipBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr);

EmwNetworkStack::Buffer_t *EmwNetworkStack::allocBuffer(void)
{
  struct pbuf *const network_packet_ptr = pbuf_alloc(PBUF_RAW, (u16_t)EmwNetworkStack::NETWORK_BUFFER_SIZE, PBUF_POOL);

  if (nullptr != network_packet_ptr) {
    DEBUG_IO_LOG("\nEmwNetworkStack::allocBuffer(): allocated %p (%" PRIu32 ")\n",
                 static_cast<void *>(network_packet_ptr), (uint32_t)EmwNetworkStack::NETWORK_BUFFER_SIZE)
    EMW_STATS_INCREMENT(alloc)
  }
  return reinterpret_cast<EmwNetworkStack::Buffer_t *>(network_packet_ptr);
}

void EmwNetworkStack::freeBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  DEBUG_IO_LOG("\nEmwNetworkStack::freeBuffer() : releasing %p\n", (void *)networkPacketPtr)
  (void) pbuf_free(EmwBufferToLwipBuffer(networkPacketPtr));
  EMW_STATS_INCREMENT(free)
}

uint8_t *EmwNetworkStack::getBufferPayload(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  return static_cast<uint8_t *>((EmwBufferToLwipBuffer(networkPacketPtr))->payload);
}

uint32_t EmwNetworkStack::getBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  return static_cast<uint32_t>(EmwBufferToLwipBuffer(networkPacketPtr)->len);
  return 0;
}

void EmwNetworkStack::hideBufferHeader(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  const int16_t size = static_cast<int16_t>(EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED);
  (void) pbuf_header(EmwBufferToLwipBuffer(networkPacketPtr), - size);
}

void EmwNetworkStack::setBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr, uint32_t size)
{
  pbuf_realloc(EmwBufferToLwipBuffer(networkPacketPtr), (u16_t)size);
}

#define NETWORK_BUFFER_SIZE_DEFINED \
  (EMW_NETWORK_MTU_SIZE_DEFINED + EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED + EMW_NETWORK_PBUF_LINK_HLEN_DEFINED)

const uint16_t EmwNetworkStack::NETWORK_MTU_SIZE = (uint16_t) EMW_NETWORK_MTU_SIZE_DEFINED;
const uint16_t EmwNetworkStack::NETWORK_BUFFER_SIZE = (uint16_t) NETWORK_BUFFER_SIZE_DEFINED;
const uint16_t EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE = (uint16_t)(NETWORK_BUFFER_SIZE_DEFINED - 6U);

static struct pbuf *EmwBufferToLwipBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  return reinterpret_cast<struct pbuf *>(networkPacketPtr);
}
