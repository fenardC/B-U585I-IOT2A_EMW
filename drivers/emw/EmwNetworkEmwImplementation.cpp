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


EmwNetworkStack::Buffer_t *EmwNetworkStack::AllocBuffer(void) noexcept
{
  EmwNetworkStack::Buffer_t *const network_packet_ptr \
    = static_cast<EmwBuffer_t *>(EmwOsInterface::Malloc(EmwNetworkStack::NETWORK_BUFFER_SIZE + sizeof(EmwBuffer_t) - 1U));

  if (nullptr != network_packet_ptr) {
    network_packet_ptr->length = EmwNetworkStack::NETWORK_BUFFER_SIZE;
    network_packet_ptr->headerLength = 0U;

    DEBUG_IO_LOG("\nEmwNetworkStack::AllocBuffer(): allocated %p (%" PRIu32 ")\n",
                 static_cast<const void *>(network_packet_ptr), network_packet_ptr->length)

    EMW_STATS_INCREMENT(alloc)
  }
  return network_packet_ptr;
}

void EmwNetworkStack::FreeBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  DEBUG_IO_LOG("\nEmwNetworkStack::FreeBuffer(): releasing %p\n", static_cast<const void *>(networkPacketPtr))

  EmwOsInterface::Free(static_cast<void *>(networkPacketPtr));
  EMW_STATS_INCREMENT(free)
}

std::uint8_t *EmwNetworkStack::GetBufferPayload(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  return &networkPacketPtr->data[networkPacketPtr->headerLength];
}

void EmwNetworkStack::SetBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr, std::uint32_t size) noexcept
{
  networkPacketPtr->length = size;
}

std::uint32_t EmwNetworkStack::GetBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  return networkPacketPtr->length;
}

void EmwNetworkStack::HideBufferHeader(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  const std::uint32_t size = 6U; /* EmwCoreIpc::HEADER_SIZE */
  networkPacketPtr->headerLength += size;
}

#define NETWORK_BUFFER_SIZE_DEFINED (2500U)
const std::uint16_t EmwNetworkStack::NETWORK_BUFFER_SIZE = NETWORK_BUFFER_SIZE_DEFINED;
const std::uint16_t EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE = NETWORK_BUFFER_SIZE_DEFINED - 6U;
