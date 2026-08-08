/**
  ******************************************************************************
  * Copyright (C) 2025-2026 C.Fenard.
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
#include "WiFiNetworkCore.hpp"
#include "EmwApiEmwBypass.hpp"
#include "EmwNetworkStack.hpp"
#include "emw_conf.hpp"
#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS */
#include "WiFiNetworkSoftAp.hpp"
#include "WiFiNetworkStation.hpp"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <memory>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)
//#define ENABLE_DEBUG

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))


void WiFiNetworkCore::InitializeSystem(class EmwApiCore *wifiEmwPtr) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkCore::InitializeSystem()>\n")

  if (static_cast<err_t>(ERR_OK) == sys_mbox_new(&WiFiNetworkCore::TransmitFifo, 8U)) {
    static const char transmit_fifo_name[] = {"WiFiNetworkCore::FIFO"};
    vQueueAddToRegistry(static_cast<QueueHandle_t>(WiFiNetworkCore::TransmitFifo.mbx), transmit_fifo_name);
  }
  else {
    LWIP_ASSERT("Creation of the transmit FIFO for the Wi-Fi device failed", false);
  }
  {
    static const char transmit_fifo_thread_name[] = {"NETIF-SendThread"};

    static_cast<void>(sys_thread_new(transmit_fifo_thread_name,
                                     static_cast<lwip_thread_fn>(WiFiNetworkCore::TranmitThreadFunction), NULL,
                                     WiFiNetworkCore::TRANSMIT_THREAD_STACK_SIZE,
                                     WiFiNetworkCore::TRANSMIT_THREAD_PRIORITY));
    /* Be cooperative. */
    vTaskDelay(1U);
  }

  WiFiNetworkCore::emwPtr = reinterpret_cast<class EmwApiEmwBypass *>(wifiEmwPtr);

  static_cast<void>(WiFiNetworkCore::emwPtr->setByPass(1, &WiFiNetworkCore::InputFromEmw));

  DEBUG_STD_PRINTF("WiFiNetworkCore::InitializeSystem()<\n")
}

void WiFiNetworkCore::UnInitializeSystem(void) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkCore::UnInitializeSystem()>\n")

  if (0U == WiFiNetworkCore::emwPtr->numberOfInterfacesRunning()) {
    WiFiNetworkCore::DeleteTransmitFifo();
    sys_mbox_free(&WiFiNetworkCore::TransmitFifo);
  }
  DEBUG_STD_PRINTF("WiFiNetworkCore::UnInitializeSystem()<\n")
}


WiFiNetworkCore::WiFiNetworkCore(void) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkCore::WiFiNetworkCore()>\n")
  DEBUG_STD_PRINTF("WiFiNetworkCore::WiFiNetworkCore()<\n\n")
}

WiFiNetworkCore::~WiFiNetworkCore(void) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkCore::~WiFiNetworkCore()>\n")
  DEBUG_STD_PRINTF("WiFiNetworkCore::~WiFiNetworkCore<()\n\n")
}

std::int32_t WiFiNetworkCore::UnInitializeEmw(void) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkCore::unInitializeEmw()>\n")

  if (1U == WiFiNetworkCore::emwPtr->numberOfInterfacesRunning()) {
    static_cast<void>(WiFiNetworkCore::emwPtr->setByPass(0, nullptr));
  }
  WiFiNetworkCore::emwPtr->unInitialize();
  DEBUG_STD_PRINTF("WiFiNetworkCore::unInitializeEmw()<\n")
  return 0;
}


void WiFiNetworkCore::OutputToTransmitFifo(struct pbuf *bufferPtr) noexcept
{
  pbuf_ref(bufferPtr);
  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
              ("WiFiNetworkCore::OutputToTransmitFifo(): Transmit buffer %p next=%p  tot_len=%" PRIu32 " len=%" PRIu32 "\n",
               static_cast<const void *>(bufferPtr), static_cast<const void *>(bufferPtr->next),
               static_cast<std::uint32_t>(bufferPtr->tot_len), static_cast<std::uint32_t>(bufferPtr->len)));

  if ((nullptr != bufferPtr->next) || (bufferPtr->tot_len != bufferPtr->len)) {
    struct pbuf *buf_send_ptr = pbuf_clone(PBUF_RAW, PBUF_RAM, bufferPtr);

    static_cast<void>(pbuf_free(bufferPtr));
    if (nullptr == buf_send_ptr) {
      LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                  ("WiFiNetworkCore::OutputToTransmitFifo(): No memory for chained buffer!!!"));
    }
    else {
      sys_mbox_post(&WiFiNetworkCore::TransmitFifo, buf_send_ptr);
#if defined(ENABLE_DEBUG)
      {
        DEBUG_STD_PRINTF(">\n");
        for (std::uint32_t i = 0U; i < buf_send_ptr->tot_len; i++) {
          const std::uint8_t tx_data = (static_cast<std::uint8_t *>(buf_send_ptr->payload))[i];
          DEBUG_STD_PRINTF("%x", tx_data);
        }
        DEBUG_STD_PRINTF("<\n");
      }
#endif /* ENABLE_DEBUG */
    }
  }
  else {
    sys_mbox_post(&WiFiNetworkCore::TransmitFifo, bufferPtr);
  }
}

void WiFiNetworkCore::InputFromEmw(EmwNetworkStack::Buffer_t *bufferPtr, std::uint32_t interfaceIndex) noexcept
{
  if (nullptr != bufferPtr) {
    if (0U < EmwNetworkStack::GetBufferPayloadSize(bufferPtr)) {
      const uint32_t interface_index \
        = (interfaceIndex == EmwApiBase::eWIFI_INTERFACE_STATION_IDX) ? EmwApiBase::eWIFI_INTERFACE_STATION_IDX \
          : EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX;

      if (EmwApiBase::eWIFI_INTERFACE_STATION_IDX == interface_index) {
        WiFiNetworkStationPtr->inputNetif(bufferPtr);
      }
      else if (EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX == interface_index) {
        WiFiNetworkSoftApPtr->inputNetif(bufferPtr);
      }
      else {
        EmwNetworkStack::FreeBuffer(bufferPtr);
      }
    }
    else {
      EmwNetworkStack::FreeBuffer(bufferPtr);
    }
  }
}

void WiFiNetworkCore::PushToEmw(std::uint32_t timeoutInMs) noexcept
{
  void *message_ptr = nullptr;

  if (SYS_ARCH_TIMEOUT != sys_arch_mbox_fetch(&WiFiNetworkCore::TransmitFifo, &message_ptr, timeoutInMs)) {
    if (nullptr != message_ptr) {
      struct pbuf *const packet_ptr = static_cast<struct pbuf *>(message_ptr);
      const EmwApiBase::Status ret \
        = WiFiNetworkCore::emwPtr->output(static_cast<std::uint8_t*>(packet_ptr->payload),
                                          static_cast<std::int32_t>(packet_ptr->len),
                                          static_cast<std::int32_t>(packet_ptr->if_idx));

      LWIP_ASSERT("PushToEmw() failed", EmwApiBase::eEMW_STATUS_OK == ret);
      static_cast<void>(pbuf_free(packet_ptr));
    }
  }
}

void WiFiNetworkCore::TranmitThreadFunction(void *argumentPtr) noexcept
{
  static_cast<void>(argumentPtr);

  std::setbuf(stdout, nullptr);
  lwip_socket_thread_init();

  WiFiNetworkCore::TransmitThreadQuitFlag = false;
  while (!WiFiNetworkCore::TransmitThreadQuitFlag) {
    WiFiNetworkCore::PushToEmw(500U);
  }
  WiFiNetworkCore::TransmitThreadQuitFlag = false;
  lwip_socket_thread_cleanup();
  vTaskDelete(NULL);
  for (;;);
}

void WiFiNetworkCore::DeleteTransmitFifo(void) noexcept
{
  WiFiNetworkCore::TransmitThreadQuitFlag = true;

  while (WiFiNetworkCore::TransmitThreadQuitFlag) {
    /* Be cooperative. */
    vTaskDelay(50U);
  }
}

class EmwApiEmwBypass *WiFiNetworkCore::emwPtr = nullptr;

sys_mbox_t WiFiNetworkCore::TransmitFifo = {nullptr};
volatile bool WiFiNetworkCore::TransmitThreadQuitFlag;

class WiFiNetworkInterface<WiFiNetworkSoftAp> *WiFiNetworkCore::WiFiNetworkSoftApPtr = nullptr;
class WiFiNetworkInterface<WiFiNetworkStation> *WiFiNetworkCore::WiFiNetworkStationPtr = nullptr;
