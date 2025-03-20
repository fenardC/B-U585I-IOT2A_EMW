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
#include "EmwCoreHci.hpp"
#include "EmwIoInterface.hpp"
#if defined(EMW_USE_SPI_DMA)
#include "EmwIoSpi.hpp"
#endif /* EMW_USE_SPI_DMA) */
#include "emw_conf.hpp"
#include <cinttypes>
#include <cstdint>

#if !defined(EMW_HCI_DEBUG)
#define DEBUG_HCI_LOG(...)
#endif /* EMW_HCI_DEBUG */

void EmwCoreHci::Free(EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept
{
  if (nullptr != networkBufferPtr) {
    EmwNetworkStack::FreeBuffer(networkBufferPtr);
  }
}

void EmwCoreHci::Initialize(void) noexcept
{
  DEBUG_HCI_LOG("\n[%6" PRIu32 "] EmwCoreHci::Initialize()>\n", HAL_GetTick())

  EmwCoreHci::Io.initialize(EmwIoInterfaceTypes::eINITIALIZE);
  {
    static const char network_packet_fifo_name[] = {"EMW-HciNetworkPacketFifo"};
    EmwOsInterface::Status os_status = EmwOsInterface::CreateMessageQueue(EmwCoreHci::NetworkPacketFifo,
                                       network_packet_fifo_name, EMW_HCI_MAX_RX_BUFFER_COUNT);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
#if defined(EMW_WITH_NO_OS) && defined(EMW_USE_SPI_DMA)
  {
    /**
      * The PollData() will be called when the caller enters
      * the getMessageQueue() operation.
      * It performs polling as the dedicated thread can do when using a RTOS.
      */
    EmwOsInterface::AddMessageQueueHook(EmwCoreHci::NetworkPacketFifo,
                                        EmwIoInterface<EmwIoSpi>::PollData, &EmwCoreHci::Io, nullptr);
  }
#endif /* EMW_WITH_NO_OS && EMW_USE_SPI_DMA */
  DEBUG_HCI_LOG("\n[%6" PRIu32 "] EmwCoreHci::Initialize()<\n", HAL_GetTick())
}

void EmwCoreHci::Input(EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept
{
  if (nullptr != networkBufferPtr) {
    const std::uint8_t *const buffer_payload_ptr = EmwNetworkStack::GetBufferPayload(networkBufferPtr);
    const std::uint32_t buffer_payload_size = EmwNetworkStack::GetBufferPayloadSize(networkBufferPtr);

    DEBUG_HCI_LOG("\nEmwCoreHci::Input(): %" PRIu32 "\n", buffer_payload_size)

    if ((nullptr != buffer_payload_ptr) && (0U < buffer_payload_size)) {
#if 0
      for (uint32_t i = 0; i < buffer_payload_size; i++) {
        DEBUG_HCI_LOG("%02" PRIx32 " ", static_cast<std::uint32_t>(*(buffer_payload_ptr + i)))
      }
#endif /* 0 */
      if (EmwOsInterface::eOK \
          != EmwOsInterface::PutMessageQueue(EmwCoreHci::NetworkPacketFifo, networkBufferPtr, EMW_OS_TIMEOUT_FOREVER)) {
        DRIVER_ERROR_VERBOSE("HCI push input queue error!\n")
        EmwNetworkStack::FreeBuffer(networkBufferPtr);
      }
      else {
        DEBUG_HCI_LOG("\n EmwCoreHci::Input(): input length %" PRIu32 "\n", buffer_payload_size)
        EMW_STATS_INCREMENT(fifoIn)
      }
    }
  }
}

EmwNetworkStack::Buffer_t *EmwCoreHci::Receive(std::uint32_t timeoutInMs) noexcept
{
  EmwNetworkStack::Buffer_t *network_buffer_ptr = nullptr;
  const void *message_ptr = nullptr;
  const EmwOsInterface::Status os_status \
    = EmwOsInterface::GetMessageQueue(EmwCoreHci::NetworkPacketFifo, timeoutInMs, message_ptr);

  if ((EmwOsInterface::eOK == os_status) && (nullptr != message_ptr)) {
    network_buffer_ptr = reinterpret_cast<EmwNetworkStack::Buffer_t *>(const_cast<void*>(message_ptr));
    EMW_STATS_INCREMENT(fifoOut)
#if 0
    {
      const std::uint32_t length = EmwNetworkStack::GetBufferPayloadSize(network_buffer_ptr);
      DEBUG_HCI_LOG("\n EmwCoreHci::Receive(): %p %" PRIu32 "\n", static_cast<const void *>(network_buffer_ptr), length)
    }
#endif /* 0 */
  }
  return network_buffer_ptr;
}

void EmwCoreHci::ResetIo(void) noexcept
{
  DEBUG_HCI_LOG("\n[%6" PRIu32 "] EmwCoreHci::ResetIo()>\n", HAL_GetTick())

  EmwCoreHci::Io.initialize(EmwIoInterfaceTypes::eRESET);
  DEBUG_HCI_LOG("\n[%6" PRIu32 "] EmwCoreHci::ResetIo()<\n", HAL_GetTick())
}

std::int32_t EmwCoreHci::Send(const std::uint8_t *payloadPtr, std::uint16_t payloadLength) noexcept
{
  std::int32_t status = 0;

  {
    const std::uint16_t sent = EmwCoreHci::Io.send(payloadPtr, payloadLength);

    if (payloadLength != sent) {
      DEBUG_HCI_LOG(" EmwCoreHci::Send(): ERROR sent= %" PRIu32 "!\n", static_cast<std::uint32_t>(sent))
      DRIVER_ERROR_VERBOSE("HCI output (SPI) error!\n")
      status = -1;
    }
  }
  return status;
}

void EmwCoreHci::UnInitialize(void) noexcept
{
  DEBUG_HCI_LOG("\n EmwCoreHci::UnInitialize()>\n")

  EmwOsInterface::DeleteMessageQueue(EmwCoreHci::NetworkPacketFifo);
  EmwCoreHci::Io.unInitialize();

  DEBUG_HCI_LOG("\n EmwCoreHci::UnInitialize()<\n")
}

#if defined(EMW_USE_SPI_DMA)
class EmwIoSpi EmwCoreHci::IoSpi;
class EmwIoInterface<EmwIoSpi> &EmwCoreHci::Io = EmwCoreHci::IoSpi;
#endif /* EMW_USE_SPI_DMA) */

EmwOsInterface::Queue_t EmwCoreHci::NetworkPacketFifo;
