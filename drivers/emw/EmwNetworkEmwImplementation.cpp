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
#include "EmwOsInterface.hpp"

#if !defined(EMW_IO_DEBUG)
#define DEBUG_IO_LOG(...)
#endif /* EMW_IO_DEBUG */

typedef __PACKED_STRUCT {
  uint32_t length;
  uint32_t headerLength;
  uint8_t data[1];
} EmwBuffer_t;

EmwNetworkStack::Buffer_t *EmwNetworkStack::allocBuffer(void)
{
  EmwBuffer_t *const network_packet_ptr \
    = static_cast<EmwBuffer_t *>(EmwOsInterface::malloc(EmwNetworkStack::NETWORK_BUFFER_SIZE + sizeof(EmwBuffer_t) - 1U));

  if (nullptr != network_packet_ptr) {
    network_packet_ptr->length = EmwNetworkStack::NETWORK_BUFFER_SIZE;
    network_packet_ptr->headerLength = 0U;

    DEBUG_IO_LOG("\nEmwNetworkStack::allocBuffer(): allocated %p (%" PRIu32 ")\n",
                 static_cast<void *>(network_packet_ptr), network_packet_ptr->length)
    EMW_STATS_INCREMENT(alloc)
  }
  return reinterpret_cast<EmwNetworkStack::Buffer_t *>(network_packet_ptr);
}

void EmwNetworkStack::freeBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  EmwBuffer_t *const network_packet_ptr = reinterpret_cast<EmwBuffer_t *>(networkPacketPtr);
  DEBUG_IO_LOG("\nEmwNetworkStack::freeBuffer(): releasing %p\n", static_cast<void *>(networkPacketPtr))

  EmwOsInterface::free(static_cast<void *>(network_packet_ptr));
  EMW_STATS_INCREMENT(free)
}

uint8_t *EmwNetworkStack::getBufferPayload(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  EmwBuffer_t *const network_packet_ptr = reinterpret_cast<EmwBuffer_t *>(networkPacketPtr);
  return &network_packet_ptr->data[network_packet_ptr->headerLength];
}

void EmwNetworkStack::setBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr, uint32_t size)
{
  EmwBuffer_t *const network_packet_ptr = reinterpret_cast<EmwBuffer_t *>(networkPacketPtr);
  network_packet_ptr->length = size;
}

uint32_t EmwNetworkStack::getBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  const EmwBuffer_t *const network_packet_ptr = reinterpret_cast<EmwBuffer_t *>(networkPacketPtr);
  return network_packet_ptr->length;
}

void EmwNetworkStack::hideBufferHeader(EmwNetworkStack::Buffer_t *networkPacketPtr)
{
  EmwBuffer_t *const network_packet_ptr = reinterpret_cast<EmwBuffer_t *>(networkPacketPtr);
  const uint32_t size = 6U; /* EmwCoreIpc::HEADER_SIZE */
  network_packet_ptr->headerLength += size;
}

#define NETWORK_BUFFER_SIZE_DEFINED (2500U)
const uint16_t EmwNetworkStack::NETWORK_BUFFER_SIZE = (uint16_t) NETWORK_BUFFER_SIZE_DEFINED;
const uint16_t EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE = (uint16_t)(NETWORK_BUFFER_SIZE_DEFINED - 6U);
