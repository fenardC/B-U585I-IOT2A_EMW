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
#include "emw_conf.hpp"
#include "EmwApiCore.hpp"
#include "EmwCoreIpc.hpp"
#include "EmwCoreHci.hpp"
#include "EmwOsInterface.hpp"
#include "EmwNetworkStack.hpp"
#include <cstdint>
#include <cstdbool>
#include <cstring>
#include <cinttypes>

#if !defined(EMW_IPC_DEBUG)
#define DEBUG_IPC_LOG(...)
#endif /* EMW_IPC_DEBUG */

void EmwCoreIpc::initialize(const class EmwApiCore *corePtr)
{
  EmwCoreIpc::PendingRequest.reqId = EmwCoreIpc::REQ_ID_RESET_VAL;
  EmwCoreIpc::IsPowerSaveEnabled = false;
  {
    static const char ipc_lock_name[] = {"EMW-IpcLock"};
    const EmwOsInterface::Status os_status = EmwOsInterface::createMutex(&IpcLock, ipc_lock_name);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char response_sem_name[] = {"EMW-IpcResponseSem"};
    const EmwOsInterface::Status os_status \
      = EmwOsInterface::createSemaphore(&EmwCoreIpc::PendingRequest.responseSem, response_sem_name, 1U, 0U);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
#if defined(EMW_WITH_NO_OS)
  {
    const EmwOsInterface::Status os_status \
      = EmwOsInterface::addSemaphoreHook(&EmwCoreIpc::PendingRequest.responseSem, EmwCoreIpc::poll, nullptr, corePtr);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
#endif /* EMW_WITH_NO_OS */
  EmwCoreHci::initialize(corePtr);
}

void EmwCoreIpc::poll(void *THIS, const class EmwApiCore *corePtr, uint32_t timeoutInMs)
{
  EmwNetworkStack::Buffer_t *const network_buffer_ptr = EmwCoreHci::receive(timeoutInMs);
  static_cast<void>(THIS);

  if (nullptr != network_buffer_ptr) {
    const uint32_t length = EmwNetworkStack::getBufferPayloadSize(network_buffer_ptr);
    DEBUG_IPC_LOG("EmwCoreIpc::poll(): %p HCI received length %" PRIu32 "\n",
                  static_cast<void *>(network_buffer_ptr), length)
    if (length > 0U) {
      EmwCoreIpc::processEvent(*corePtr, network_buffer_ptr);
    }
    else {
      EmwNetworkStack::freeBuffer(network_buffer_ptr);
    }
  }
}

EmwCoreIpc::Status EmwCoreIpc::request(const class EmwApiCore &core, uint8_t *commandPtr, uint16_t commandSize,
                                       uint8_t *responseBufferPtr, uint16_t *responseBufferSizePtr,
                                       uint32_t timeoutInMs)
{
  EmwCoreIpc::Status ret = EmwCoreIpc::eERROR;
  DEBUG_IPC_LOG("EmwCoreIpc::request()>\n")

  if ((0U < core.runtime.interfaces) && (nullptr != commandPtr) && (nullptr != responseBufferSizePtr)) {
    uint8_t *const command_data_ptr = EmwCoreIpc::startWithHeader(commandPtr);
    uint16_t api_id = EmwCoreIpc::getApiId(command_data_ptr);
    bool is_command_size_allowed;

    EmwScopedLock lock(&EmwCoreIpc::IpcLock);

    if ((EmwCoreIpc::eWIFI_EAP_SET_CERT_CMD == api_id) && (commandSize < 2500U)) {
      is_command_size_allowed = true;
    }
    else if (commandSize <= EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      is_command_size_allowed = true;
    }
    else {
      is_command_size_allowed = false;
    }
    if (is_command_size_allowed) {
      const uint16_t command_data_size = EmwCoreIpc::HEADER_SIZE + commandSize;
      const uint32_t req_id = EmwCoreIpc::getNewReqId();

      EmwCoreIpc::setReqId(command_data_ptr, req_id);
      EmwOsInterface::assertAlways(EmwCoreIpc::REQ_ID_RESET_VAL == EmwCoreIpc::PendingRequest.reqId);
      EmwCoreIpc::PendingRequest.reqId = req_id;
      EmwCoreIpc::PendingRequest.responseBufferPtr = responseBufferPtr;
      EmwCoreIpc::PendingRequest.responseBufferSizePtr = responseBufferSizePtr;

      if (IsPowerSaveEnabled) {
        (void) EmwCoreHci::send(core, reinterpret_cast<const uint8_t *>("dummy"), 5U);
        (void) EmwOsInterface::delay(10U);
      }
      DEBUG_IPC_LOG("EmwCoreIpc::request(): req_id: 0x%08" PRIx32 " : %" PRIu32 "\n",
                    req_id, static_cast<uint32_t>(command_data_size))

      {
        const int32_t hci_status = EmwCoreHci::send(core, command_data_ptr, command_data_size);
        if (0 != hci_status) {
          DRIVER_ERROR_VERBOSE("IPC failed to send command to HCI\n")
        }
        EmwOsInterface::assertAlways(0 == hci_status);
      }
      if (EmwOsInterface::eOK != EmwOsInterface::takeSemaphore(&EmwCoreIpc::PendingRequest.responseSem, timeoutInMs)) {
        DEBUG_IPC_LOG("EmwCoreIpc::request(): Error: command 0x%04" PRIx32 " timeout(%" PRIu32 " ms)" \
                      " waiting answer %" PRIu32 "\n",
                      static_cast<uint32_t>(api_id), timeoutInMs, EmwCoreIpc::PendingRequest.reqId)
        DRIVER_ERROR_VERBOSE("IPC Error with waiting answer\n")
        EmwCoreIpc::PendingRequest.reqId = EmwCoreIpc::REQ_ID_RESET_VAL;
        ret = EmwCoreIpc::eERROR;
      }
      else {
        ret = EmwCoreIpc::eSUCCESS;
      }
      DEBUG_IPC_LOG("EmwCoreIpc::request()< req_id: 0x%08" PRIx32 " api_id: 0x%04" PRIx32 " "
                    "done (%" PRId32 ")\n\n",
                    req_id, static_cast<uint32_t>(api_id), static_cast<int32_t>(ret))
    }
    if (EmwCoreIpc::eWIFI_PS_ON_CMD == EmwCoreIpc::getApiId(command_data_ptr)) {
      IsPowerSaveEnabled = true;
    }
    if (EmwCoreIpc::eWIFI_PS_OFF_CMD == EmwCoreIpc::getApiId(command_data_ptr)) {
      IsPowerSaveEnabled = false;
    }
  }
  DEBUG_IPC_LOG("EmwCoreIpc::request()<\n")
  return ret;
}

void EmwCoreIpc::setApiId(uint8_t buffer[], uint16_t apiId)
{
  buffer[PACKET_API_ID_OFFSET] = static_cast<uint8_t>(apiId & 0x00FFU);
  buffer[PACKET_API_ID_OFFSET + 1U] = static_cast<uint8_t>((apiId & 0xFF00U) >> 8);
}

uint8_t *EmwCoreIpc::startWithHeader(const uint8_t *bytesPtr)
{
  return const_cast<uint8_t *>(bytesPtr - EmwCoreIpc::PACKET_PARAMS_OFFSET);
}

EmwCoreIpc::Status EmwCoreIpc::testIpcEcho(const class EmwApiCore &core,
    uint8_t *dataInPtr, uint16_t dataInLength, uint8_t *dataOutPtr, uint16_t *dataOutLengthPtr, uint32_t timeoutInMs)
{
  EmwCoreIpc::Status ret = EmwCoreIpc::eERROR;

  if ((nullptr != dataInPtr) && (EmwCoreIpc::PACKET_MIN_SIZE <= dataInLength) \
      && ((dataInLength - EmwCoreIpc::HEADER_SIZE) <= EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) \
      && (nullptr != dataOutPtr) && (nullptr != dataOutLengthPtr)) {
    setApiId(dataInPtr, EmwCoreIpc::eSYS_ECHO_CMD);

    if (EmwCoreIpc::eSUCCESS != EmwCoreIpc::request(core,
        &dataInPtr[PACKET_PARAMS_OFFSET], static_cast<uint16_t>(dataInLength - EmwCoreIpc::HEADER_SIZE),
        dataOutPtr, dataOutLengthPtr, timeoutInMs)) {
      *dataOutLengthPtr = 0U;
    }
    else {
      *dataOutLengthPtr += EmwCoreIpc::HEADER_SIZE;
      ret = EmwCoreIpc::eSUCCESS;
    }
  }
  return ret;
}

EmwCoreIpc::Status EmwCoreIpc::unInitialize(void)
{
  EmwCoreIpc::Status ret;
  {
    EmwScopedLock lock(&EmwCoreIpc::IpcLock);
    (void) EmwOsInterface::deleteSemaphore(&EmwCoreIpc::PendingRequest.responseSem);
  }
  (void) EmwOsInterface::deleteMutex(&EmwCoreIpc::IpcLock);
  if (0 == EmwCoreHci::unInitialize()) {
    ret = EmwCoreIpc::eSUCCESS;
  }
  else {
    ret = EmwCoreIpc::eERROR;
  }
  return ret;
}

uint16_t EmwCoreIpc::getApiId(const uint8_t buffer[])
{
  const uint16_t api_id = (static_cast<uint16_t>(buffer[PACKET_API_ID_OFFSET + 1U] << 8)) \
                          | static_cast<uint16_t>(buffer[PACKET_API_ID_OFFSET]);
  return api_id;
}

uint32_t EmwCoreIpc::getNewReqId(void)
{
  static uint32_t req_id = UINT32_MAX - 2;
  req_id++;
  if (EmwCoreIpc::REQ_ID_RESET_VAL == req_id) {
    req_id++;
  }
  return req_id;
}

uint32_t EmwCoreIpc::getReqId(const uint8_t buffer[])
{
  const uint32_t req_id \
    = static_cast<uint32_t>(buffer[PACKET_REQ_ID_OFFSET + 3U] << 24) \
      | static_cast<uint32_t>(buffer[PACKET_REQ_ID_OFFSET + 2U] << 16) \
      | static_cast<uint32_t>(buffer[PACKET_REQ_ID_OFFSET + 1U] << 8) \
      | static_cast<uint32_t>(buffer[PACKET_REQ_ID_OFFSET]);
  return req_id;
}

uint8_t *EmwCoreIpc::removeHeader(uint8_t *ipcBytesPtr)
{
  return static_cast<uint8_t *>(ipcBytesPtr + EmwCoreIpc::PACKET_PARAMS_OFFSET);
}

void EmwCoreIpc::setReqId(uint8_t buffer[], uint32_t reqId)
{
  buffer[PACKET_REQ_ID_OFFSET] = static_cast<uint8_t>(reqId & 0x000000FFU);
  buffer[PACKET_REQ_ID_OFFSET + 1U] = static_cast<uint8_t>((reqId & 0x0000FF00U) >> 8);
  buffer[PACKET_REQ_ID_OFFSET + 2U] = static_cast<uint8_t>((reqId & 0x00FF0000U) >> 16);
  buffer[PACKET_REQ_ID_OFFSET + 3U] = static_cast<uint8_t>((reqId & 0xFF000000U) >> 24);
}

void EmwCoreIpc::processEvent(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  static const EventItem_t event_table[] = {
    {EmwCoreIpc::eSYS_REBOOT_EVENT, EmwCoreIpc::rebootEventCallback}
    , {EmwCoreIpc::eSYS_FOTA_STATUS_EVENT, EmwCoreIpc::fotaStatusEventCallback}
    , {EmwCoreIpc::eWIFI_STATUS_EVENT, EmwCoreIpc::wiFiStatusEventCallback}
#if defined(EMW_NETWORK_BYPASS_MODE)
    , {EmwCoreIpc::eWIFI_BYPASS_INPUT_EVENT, EmwCoreIpc::wiFiNetlinkInputCallback}
#endif /* EMW_NETWORK_BYPASS_MODE */
  };
  uint8_t *const payload_ptr = EmwNetworkStack::getBufferPayload(networkBufferPtr);
  const uint32_t payload_size = EmwNetworkStack::getBufferPayloadSize(networkBufferPtr);

  if ((nullptr != payload_ptr) && (EmwCoreIpc::PACKET_MIN_SIZE <= payload_size)) {
    const uint32_t req_id = EmwCoreIpc::getReqId(payload_ptr);
    const uint16_t api_id = EmwCoreIpc::getApiId(payload_ptr);

    DEBUG_IPC_LOG("EmwCoreIpc::processEvent(): req_id: 0x%08" PRIx32 ", api_id: 0x%04" PRIx32 "\n",
                  req_id, static_cast<uint32_t>(api_id))

    if ((0U == (api_id & MIPC_API_EVENT_BASE))) {
      if (EmwCoreIpc::PendingRequest.reqId == req_id) {
        if ((nullptr != EmwCoreIpc::PendingRequest.responseBufferSizePtr) \
            && (0U < *EmwCoreIpc::PendingRequest.responseBufferSizePtr) \
            && (nullptr != EmwCoreIpc::PendingRequest.responseBufferPtr)) {
          const uint32_t response_buffer_size = *(EmwCoreIpc::PendingRequest.responseBufferSizePtr);
          const uint32_t buffer_size = payload_size - EmwCoreIpc::PACKET_MIN_SIZE;
          const uint32_t actual_size = (response_buffer_size < buffer_size) ? response_buffer_size : buffer_size;

          (void) memcpy(EmwCoreIpc::PendingRequest.responseBufferPtr,
                        static_cast<void *>(EmwCoreIpc::removeHeader(payload_ptr)), actual_size);
          *EmwCoreIpc::PendingRequest.responseBufferSizePtr = static_cast<uint16_t>(actual_size);
        }
        EmwCoreIpc::PendingRequest.reqId = EmwCoreIpc::REQ_ID_RESET_VAL;
        {
          const EmwOsInterface::Status \
          os_status = EmwOsInterface::releaseSemaphore(&EmwCoreIpc::PendingRequest.responseSem);

          if (EmwOsInterface::eOK != os_status) {
            DRIVER_ERROR_VERBOSE("IPC failed to signal command response\n")
          }
          EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
        }
        EMW_STATS_INCREMENT(cmdGetAnswer)
      }
      else {
        DEBUG_IPC_LOG("EmwCoreIpc::processEvent()  : response req_id: 0x%08" PRIx32
                      " not match pending req_id: 0x%08" PRIx32 "!\n",
                      req_id, EmwCoreIpc::PendingRequest.reqId)
      }
      EmwCoreHci::free(networkBufferPtr);
    }
    else {
      const uint32_t event_table_count = sizeof(event_table) / sizeof(event_table[0]);
      uint32_t i;
      for (i = 0U; i < event_table_count; i++) {
        if (static_cast<uint16_t>(event_table[i].eventId) == api_id) {
          const EventCallback_t callback = event_table[i].callback;
          if (nullptr != callback) {
            callback(core, networkBufferPtr);
            break;
          }
        }
      }
      if (i == event_table_count) {
        DEBUG_IPC_LOG("EmwCoreIpc::processEvent()  : Unknown event: 0x%04" PRIx32 "!\n", static_cast<uint32_t>(api_id))
        DRIVER_ERROR_VERBOSE("IPC with Unknown event!\n")
        EmwCoreHci::free(networkBufferPtr);
      }
    }
  }
  else {
    DEBUG_IPC_LOG("EmwCoreIpc::processEvent()  : Unknown buffer content\n")
    EmwCoreHci::free(networkBufferPtr);
  }
}

void EmwCoreIpc::rebootEventCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  (void) core;
  EmwCoreHci::free(networkBufferPtr);
  DEBUG_IPC_LOG("\nEmwCoreIpc::rebootEventCallback(): EVENT: reboot done.\n")
}

void EmwCoreIpc::fotaStatusEventCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  uint8_t *const payload_ptr = EmwNetworkStack::getBufferPayload(networkBufferPtr);

  if (nullptr != payload_ptr) {
    const EmwApiBase::FotaStatus status \
      = *(reinterpret_cast<EmwApiBase::FotaStatus *>(EmwCoreIpc::removeHeader(payload_ptr)));

    DEBUG_IPC_LOG("\nEmwCoreIpc::fotaStatusEventCallback(): EVENT: FOTA status: %02x\n", status)

    EmwCoreHci::free(networkBufferPtr);
    {
      EmwApiBase::FotaStatusCallback_t const status_callback = core.runtime.fotaStatusCallback;
      const uint32_t callback_arg = core.runtime.fotaStatusCallbackArg;

      if (nullptr != status_callback) {
        status_callback(status, callback_arg);
      }
    }
  }
}

void EmwCoreIpc::wiFiStatusEventCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  EmwApiBase::EmwInterface wifi_interface;
  EmwApiBase::WiFiStatusCallback_t status_callback_function = nullptr;
  void *callback_arg = nullptr;
  uint8_t *const payload_ptr = EmwNetworkStack::getBufferPayload(networkBufferPtr);
  const EmwApiBase::WiFiStatus_t status \
    = *(static_cast<EmwApiBase::WiFiStatus_t *>(EmwCoreIpc::removeHeader(payload_ptr)));
  const enum EmwApiBase::WiFiEvent event = static_cast<enum EmwApiBase::WiFiEvent>(status);

  DEBUG_IPC_LOG("\nEmwCoreIpc::wiFiStatusEventCallback(): EVENT: Wi-Fi status: %02x\n", status)

  EmwCoreHci::free(networkBufferPtr);
  switch (event) {
    case EmwApiBase::eWIFI_EVENT_STA_UP:
    case EmwApiBase::eWIFI_EVENT_STA_DOWN:
    case EmwApiBase::eWIFI_EVENT_STA_GOT_IP: {
        wifi_interface = EmwApiBase::eSTATION;
        status_callback_function = core.runtime.wiFiStatusCallbacks[EmwApiBase::eWIFI_INTERFACE_STATION_IDX];
        callback_arg = core.runtime.wiFiStatusCallbackArgPtrs[EmwApiBase::eWIFI_INTERFACE_STATION_IDX];
        break;
      }
    case EmwApiBase::eWIFI_EVENT_AP_UP:
    case EmwApiBase::eWIFI_EVENT_AP_DOWN: {
        wifi_interface = EmwApiBase::eSOFTAP;
        status_callback_function = core.runtime.wiFiStatusCallbacks[EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX];
        callback_arg = core.runtime.wiFiStatusCallbackArgPtrs[EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX];
        break;
      }
    case EmwApiBase::eWIFI_EVENT_NONE:
    default: {
        wifi_interface = EmwApiBase::eSOFTAP;
        EmwOsInterface::assertAlways(false);
        break;
      }
  }
  if (nullptr != status_callback_function) {
    EMW_STATS_INCREMENT(callback)
    status_callback_function(wifi_interface, event, callback_arg);
  }
}

#if defined(EMW_NETWORK_BYPASS_MODE)
void EmwCoreIpc::wiFiNetlinkInputCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr)
{
  uint8_t *const payload_ptr = EmwNetworkStack::getBufferPayload(networkBufferPtr);
  const WiFiBypassInParams_t *const parameters_ptr \
    = reinterpret_cast<const WiFiBypassInParams_t *>(EmwCoreIpc::removeHeader(payload_ptr));

  EMW_STATS_INCREMENT(callback)
  DEBUG_IPC_LOG("\nEmwCoreIpc::wiFiNetlinkInputCallback() %p>\n", static_cast<void *>(networkBufferPtr));

  if ((nullptr != core.runtime.netlinkInputCallback) && (0U < parameters_ptr->totalLength)) {
    uint32_t low_level_netif_idx = static_cast<uint32_t>(parameters_ptr->idx);
    EmwNetworkStack::hideBufferHeader(networkBufferPtr);
    core.runtime.netlinkInputCallback(networkBufferPtr, low_level_netif_idx);
  }
  else {
    EmwNetworkStack::freeBuffer(networkBufferPtr);
  }
}
#endif /* EMW_NETWORK_BYPASS_MODE */

EmwOsInterface::mutex_t EmwCoreIpc::IpcLock;
bool EmwCoreIpc::IsPowerSaveEnabled = false;
EmwCoreIpc::HciResponse_t EmwCoreIpc::PendingRequest;
