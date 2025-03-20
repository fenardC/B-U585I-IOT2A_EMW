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

EmwNetworkStack::Buffer_t *EmwNetworkStack::AllocBuffer(void) noexcept
{
  struct pbuf *const network_packet_ptr = pbuf_alloc(PBUF_RAW, EmwNetworkStack::NETWORK_BUFFER_SIZE, PBUF_POOL);

  if (nullptr != network_packet_ptr) {
    DEBUG_IO_LOG("\nEmwNetworkStack::AllocBuffer(): allocated %p (%" PRIu32 ")\n",
                 static_cast<const void *>(network_packet_ptr), static_cast<std::uint32_t>(EmwNetworkStack::NETWORK_BUFFER_SIZE))

    EMW_STATS_INCREMENT(alloc)
  }
  return network_packet_ptr;
}

void EmwNetworkStack::FreeBuffer(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  DEBUG_IO_LOG("\nEmwNetworkStack::FreeBuffer() : releasing %p\n", static_cast<const void *>(networkPacketPtr))

  (void) pbuf_free(networkPacketPtr);
  EMW_STATS_INCREMENT(free)
}

std::uint8_t *EmwNetworkStack::GetBufferPayload(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  return static_cast<std::uint8_t *>(networkPacketPtr->payload);
}

std::uint32_t EmwNetworkStack::GetBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  return static_cast<std::uint32_t>(networkPacketPtr->len);
}

void EmwNetworkStack::HideBufferHeader(EmwNetworkStack::Buffer_t *networkPacketPtr) noexcept
{
  const std::int16_t size = static_cast<std::int16_t>(EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED);
  (void) pbuf_header(networkPacketPtr, - size);
}

void EmwNetworkStack::SetBufferPayloadSize(EmwNetworkStack::Buffer_t *networkPacketPtr, std::uint32_t size) noexcept
{
  pbuf_realloc(networkPacketPtr, static_cast<u16_t>(size));
}

#define NETWORK_BUFFER_SIZE_DEFINED \
  (EMW_NETWORK_MTU_SIZE_DEFINED + EMW_NETWORK_BYPASS_HEADER_SIZE_DEFINED + EMW_NETWORK_PBUF_LINK_HLEN_DEFINED)

const std::uint16_t EmwNetworkStack::NETWORK_MTU_SIZE = EMW_NETWORK_MTU_SIZE_DEFINED;
const std::uint16_t EmwNetworkStack::NETWORK_BUFFER_SIZE = NETWORK_BUFFER_SIZE_DEFINED;
const std::uint16_t EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE = NETWORK_BUFFER_SIZE_DEFINED - 6U;
