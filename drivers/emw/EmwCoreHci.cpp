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
#include "EmwApiCore.hpp"
#include "EmwIoInterface.hpp"
#include "emw_conf.hpp"
#include <cinttypes>
#include <cstdint>
#include <cstdbool>

#if !defined(EMW_HCI_DEBUG)
#define DEBUG_HCI_LOG(...)
#endif /* EMW_HCI_DEBUG */

void EmwCoreHci::free(EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  if (nullptr != networkBufferPtr) {
    (void) EmwNetworkStack::freeBuffer(networkBufferPtr);
  }
}

void EmwCoreHci::initialize(const class EmwApiCore *corePtr)
{
#if defined(EMW_WITH_RTOS)
  (void) corePtr;
#endif /* EMW_WITH_RTOS */
  {
    static const char network_packet_fifo_name[] = {"EMW-HciNetworkPacketFifo"};
    EmwOsInterface::Status os_status = EmwOsInterface::createMessageQueue(&EmwCoreHci::NetworkPacketFifo,
                                       network_packet_fifo_name, EMW_HCI_MAX_RX_BUFFER_COUNT);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
#if defined(EMW_WITH_NO_OS)
  {
    /**
      * The pollData() will be called when the caller enters
      * the getMessageQueue() operation.
      * It performs polling as the dedicated thread can do when using a RTOS.
      */
    EmwOsInterface::addMessageQueueHook(&EmwCoreHci::NetworkPacketFifo, EmwIoInterface::pollData, corePtr->ioPtr,
                                        corePtr);
  }
#endif /* EMW_WITH_NO_OS */
}

void EmwCoreHci::input(EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  if (nullptr != networkBufferPtr) {
    const uint8_t *const buffer_payload_ptr = EmwNetworkStack::getBufferPayload(networkBufferPtr);
    const uint32_t buffer_payload_size = EmwNetworkStack::getBufferPayloadSize(networkBufferPtr);

    DEBUG_HCI_LOG("\nEmwCoreHci::input(): %" PRIu32 "\n", buffer_payload_size)

    if ((nullptr != buffer_payload_ptr) && (0U < buffer_payload_size)) {
#if 0
      for (uint32_t i = 0; i < buffer_payload_size; i++) {
        DEBUG_HCI_LOG("%02" PRIx32 " ", (uint32_t) * (buffer_payload_ptr + i))
      }
#endif /* 0 */
      if (EmwOsInterface::eOK \
          != EmwOsInterface::putMessageQueue(&EmwCoreHci::NetworkPacketFifo, networkBufferPtr, EMW_OS_TIMEOUT_FOREVER)) {
        DRIVER_ERROR_VERBOSE("HCI push input queue error!\n")
        EmwNetworkStack::freeBuffer(networkBufferPtr);
      }
      else {
        DEBUG_HCI_LOG("\nEmwCoreHci::input(): input length %" PRIu32 "\n", buffer_payload_size)
        EMW_STATS_INCREMENT(fifoIn)
      }
    }
  }
}

EmwNetworkStack::Buffer_t *EmwCoreHci::receive(uint32_t timeoutInMs)
{
  EmwNetworkStack::Buffer_t *network_buffer_ptr = nullptr;
  void const *message_ptr = nullptr;
  const EmwOsInterface::Status os_status \
    = EmwOsInterface::getMessageQueue(&EmwCoreHci::NetworkPacketFifo, timeoutInMs, &message_ptr);

  if ((EmwOsInterface::eOK == os_status) && (nullptr != message_ptr)) {
    network_buffer_ptr = reinterpret_cast<EmwNetworkStack::Buffer_t *>(const_cast<void *>(message_ptr));
    EMW_STATS_INCREMENT(fifoOut)
#if 0
    {
      const uint32_t length = EmwNetworkStack::getBufferPayloadSize(network_buffer_ptr);
      DEBUG_HCI_LOG("\nEmwCoreHci::receive(): %p %" PRIu32 "\n", static_cast<void *>(network_buffer_ptr), length)
    }
#endif /* 0 */
  }
  return network_buffer_ptr;
}

int32_t EmwCoreHci::send(const class EmwApiCore &core, const uint8_t *payloadPtr, uint16_t payloadLength)
{
  int32_t status = 0;

#if defined(EMW_USE_SPI_DMA)
  {
    const uint16_t sent = core.ioPtr->send(core, payloadPtr, payloadLength);

    if (payloadLength != sent) {
      DEBUG_HCI_LOG("EmwCoreHci::send(): ERROR sent= %" PRIu32 " !\n", static_cast<uint32_t>(sent))
      DRIVER_ERROR_VERBOSE("HCI output (SPI) error!\n")
      status = -1;
    }
  }
#endif /* EMW_USE_SPI_DMA */
  return status;
}

int32_t EmwCoreHci::unInitialize(void)
{
  int32_t status = -1;
  if (EmwOsInterface::eOK == EmwOsInterface::deleteMessageQueue(&EmwCoreHci::NetworkPacketFifo)) {
    status = 0;
  }
  return status;
}

EmwOsInterface::Queue_t EmwCoreHci::NetworkPacketFifo;
