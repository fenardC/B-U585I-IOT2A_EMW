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
#include "EmwCoreIpc.hpp"
#include "EmwCoreHci.hpp"
#include "EmwNetworkStack.hpp"
#include "EmwOsInterface.hpp"
#include "emw_conf.hpp"
#include <cinttypes>
#include <cstdint>
#include <cstring>

#if !defined(EMW_IPC_DEBUG)
#define DEBUG_IPC_LOG(...)
#endif /* EMW_IPC_DEBUG */

static const std::uint32_t REQ_ID_RESET_VAL = UINT32_MAX;
static const std::uint32_t PACKET_REQ_ID_OFFSET = 0U;
static const std::uint32_t PACKET_REQ_ID_SIZE = 4U;
static const std::uint32_t PACKET_API_ID_OFFSET = PACKET_REQ_ID_OFFSET + PACKET_REQ_ID_SIZE;
static const std::uint32_t PACKET_API_ID_SIZE = 2U;
static const uint32_t PACKET_PARAMS_OFFSET = PACKET_API_ID_OFFSET + PACKET_API_ID_SIZE;

static inline std::uint16_t GetApiId(const std::uint8_t buffer[]);
static inline std::uint32_t GetNewReqId(void);
static inline std::uint32_t GetReqId(const std::uint8_t buffer[]);
static inline void SetApiId(std::uint8_t (&buffer)[], std::uint16_t apiId);
static inline void SetReqId(std::uint8_t buffer[], std::uint32_t reqId);


static inline std::uint16_t GetApiId(const std::uint8_t buffer[])
{
  const std::uint16_t api_id = (static_cast<std::uint16_t>(buffer[PACKET_API_ID_OFFSET + 1U] << 8)) \
                               | static_cast<std::uint16_t>(buffer[PACKET_API_ID_OFFSET]);
  return api_id;
}

static inline std::uint32_t GetNewReqId(void)
{
  static std::uint32_t req_id = UINT32_MAX - 2;

  req_id++;
  if (REQ_ID_RESET_VAL == req_id) {
    req_id++;
  }
  return req_id;
}

static inline std::uint32_t GetReqId(const std::uint8_t buffer[])
{
  const std::uint32_t req_id \
    = static_cast<std::uint32_t>(buffer[PACKET_REQ_ID_OFFSET + 3U] << 24) \
      | static_cast<std::uint32_t>(buffer[PACKET_REQ_ID_OFFSET + 2U] << 16) \
      | static_cast<std::uint32_t>(buffer[PACKET_REQ_ID_OFFSET + 1U] << 8) \
      | static_cast<std::uint32_t>(buffer[PACKET_REQ_ID_OFFSET]);
  return req_id;
}

static void inline SetApiId(std::uint8_t (&buffer)[], std::uint16_t apiId)
{
  buffer[PACKET_API_ID_OFFSET] = static_cast<std::uint8_t>(apiId & 0x00FFU);
  buffer[PACKET_API_ID_OFFSET + 1U] = static_cast<std::uint8_t>((apiId & 0xFF00U) >> 8);
}

static inline void SetReqId(std::uint8_t buffer[], std::uint32_t reqId)
{
  buffer[PACKET_REQ_ID_OFFSET] = static_cast<std::uint8_t>(reqId & 0x000000FFU);
  buffer[PACKET_REQ_ID_OFFSET + 1U] = static_cast<std::uint8_t>((reqId & 0x0000FF00U) >> 8);
  buffer[PACKET_REQ_ID_OFFSET + 2U] = static_cast<std::uint8_t>((reqId & 0x00FF0000U) >> 16);
  buffer[PACKET_REQ_ID_OFFSET + 3U] = static_cast<std::uint8_t>((reqId & 0xFF000000U) >> 24);
}

EmwCoreIpc::EmwCoreIpc() noexcept
  : isUsable(false)
{
  DEBUG_IPC_LOG("\n EmwCoreIpc::EmwCoreIpc()>\n")
  DEBUG_IPC_LOG("\n EmwCoreIpc::EmwCoreIpc()< %p\n\n", static_cast<const void*>(this))
}

EmwCoreIpc::~EmwCoreIpc(void) noexcept
{
  DEBUG_IPC_LOG("\n  EmwCoreIpc::~EmwCoreIpc(%p)>\n", static_cast<const void*>(this))
  DEBUG_IPC_LOG("\n  EmwCoreIpc::~EmwCoreIpc()<\n\n")
}

void EmwCoreIpc::construct(void) noexcept
{
  DEBUG_IPC_LOG("\n  EmwCoreIpc::construct()>\n")

  this->isUsable = true;
  EmwCoreIpc::PendingRequest.reqId = REQ_ID_RESET_VAL;
  EmwCoreIpc::IsPowerSaveEnabled = false;
  {
    static const char ipc_lock_name[] = {"EMW-IpcLock"};
    const EmwOsInterface::Status os_status = EmwOsInterface::CreateMutex(EmwCoreIpc::IpcLock, ipc_lock_name);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char response_sem_name[] = {"EMW-IpcResponseSem"};
    const EmwOsInterface::Status os_status \
      = EmwOsInterface::CreateSemaphore(EmwCoreIpc::PendingRequest.sem, response_sem_name, 1U, 0U);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
#if defined(EMW_WITH_NO_OS)
  {
    const EmwOsInterface::Status os_status \
      = EmwOsInterface::AddSemaphoreHook(EmwCoreIpc::PendingRequest.sem, EmwCoreIpc::Poll, this, nullptr);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
#endif /* EMW_WITH_NO_OS */
  EmwCoreHci::Initialize();
  DEBUG_IPC_LOG("  EmwCoreIpc::construct()<\n\n")
}

void EmwCoreIpc::destruct(void) noexcept
{
  DEBUG_IPC_LOG("  EmwCoreIpc::destruct()>\n")

  this->isUsable = false;
  {
    EmwScopedLock lock(EmwCoreIpc::IpcLock);
    EmwOsInterface::DeleteSemaphore(EmwCoreIpc::PendingRequest.sem);
  }
  EmwOsInterface::DeleteMutex(EmwCoreIpc::IpcLock);
  EmwCoreHci::UnInitialize();

  DEBUG_IPC_LOG("  EmwCoreIpc::destruct()<\n")
}

EmwCoreIpc::Status EmwCoreIpc::request(std::uint8_t (&commandData)[], std::uint16_t commandDataSize,
                                       std::uint8_t (&responseBuffer)[], std::uint16_t &responseBufferSize,
                                       std::uint32_t timeoutInMs) noexcept
{
  EmwCoreIpc::Status status = EmwCoreIpc::eERROR;

  DEBUG_IPC_LOG("  EmwCoreIpc::request()> %" PRIu32 "\n", static_cast<std::uint32_t>(commandDataSize))

  if (this->isUsable) {
    const std::uint16_t api_id = GetApiId(commandData);
    bool is_command_size_allowed;

    EmwScopedLock lock(EmwCoreIpc::IpcLock);

    if ((api_id == EmwCoreIpc::eWIFI_EAP_SET_CERT_CMD) && (commandDataSize < 2500U)) {
      is_command_size_allowed = true;
    }
    else if (commandDataSize <= EmwNetworkStack::NETWORK_BUFFER_SIZE) {
      is_command_size_allowed = true;
    }
    else {
      is_command_size_allowed = false;
    }
    if (is_command_size_allowed) {
      const std::uint32_t req_id = GetNewReqId();

      SetReqId(commandData, req_id);
      EmwOsInterface::AssertAlways(REQ_ID_RESET_VAL == EmwCoreIpc::PendingRequest.reqId);
      EmwCoreIpc::PendingRequest.reqId = req_id;
      EmwCoreIpc::PendingRequest.responsePtr = responseBuffer;
      EmwCoreIpc::PendingRequest.responseSizePtr = &responseBufferSize;

      if (EmwCoreIpc::IsPowerSaveEnabled) {
        static_cast<void>(EmwCoreHci::Send(reinterpret_cast<const std::uint8_t *>("dummy"), 5U));
        EmwOsInterface::Delay(10U);
      }
      DEBUG_IPC_LOG("  EmwCoreIpc::request(): req_id: 0x%08" PRIx32 ", api_id: 0x%08" PRIx32 "\n",
                    req_id, static_cast<std::uint32_t>(api_id))

      {
        const std::int32_t hci_status = EmwCoreHci::Send(commandData, commandDataSize);
        if (hci_status != 0) {
          DRIVER_ERROR_VERBOSE("IPC failed to send command to HCI\n")
        }
        EmwOsInterface::AssertAlways(0 == hci_status);
      }
      if (EmwOsInterface::TakeSemaphore(EmwCoreIpc::PendingRequest.sem, timeoutInMs) != EmwOsInterface::eOK) {
        DEBUG_IPC_LOG("  EmwCoreIpc::Request(): Error: command 0x%04" PRIx32 " timeout(%" PRIu32 " ms)" \
                      " waiting answer %" PRIu32 "\n",
                      static_cast<std::uint32_t>(api_id), timeoutInMs, EmwCoreIpc::PendingRequest.reqId)

        DRIVER_ERROR_VERBOSE("IPC Error with waiting answer\n")
        EmwCoreIpc::PendingRequest.reqId = REQ_ID_RESET_VAL;
        status = EmwCoreIpc::eERROR;
      }
      else {
        status = EmwCoreIpc::eSUCCESS;
      }
      DEBUG_IPC_LOG("  EmwCoreIpc::Request(): req_id: 0x%08" PRIx32 " api_id: 0x%04" PRIx32 " "
                    "done (%" PRId32 ")\n",
                    req_id, static_cast<std::uint32_t>(api_id), static_cast<std::int32_t>(status))
    }
    if (api_id == EmwCoreIpc::eWIFI_PS_ON_CMD) {
      EmwCoreIpc::IsPowerSaveEnabled = true;
    }
    else if (api_id == EmwCoreIpc::eWIFI_PS_OFF_CMD) {
      EmwCoreIpc::IsPowerSaveEnabled = false;
    }
  }
  DEBUG_IPC_LOG("  EmwCoreIpc::Request()<\n\n")
  return status;
}

void EmwCoreIpc::resetIo(void) noexcept
{
  DEBUG_IPC_LOG("\n[%6" PRIu32 "] EmwCoreIpc::resetIo()>\n", HAL_GetTick())

  if (!this->isUsable) {
    EmwCoreHci::ResetIo();
  }

  DEBUG_IPC_LOG("\n[%6" PRIu32 "] EmwCoreIpc::resetIo()<\n", HAL_GetTick())
}

EmwCoreIpc::Status EmwCoreIpc::testEcho(std::uint8_t (&dataIn)[], std::uint16_t dataInLength,
                                        std::uint8_t (&dataOut)[], std::uint16_t &dataOutLength) noexcept
{
  EmwCoreIpc::Status status;

  if ((dataInLength >= EmwCoreIpc::PACKET_MIN_SIZE) \
      && ((dataInLength - EmwCoreIpc::HEADER_SIZE) <= EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE)) {
    SetApiId(dataIn, EmwCoreIpc::eSYS_ECHO_CMD);

    if (this->request(dataIn, dataInLength, dataOut, dataOutLength,
                      5000U) != EmwCoreIpc::eSUCCESS) {
      dataOutLength = 0U;
      status = EmwCoreIpc::eERROR;
    }
    else {
      dataOutLength += EmwCoreIpc::HEADER_SIZE;
      status = EmwCoreIpc::eSUCCESS;
    }
  }
  else {
    status = EmwCoreIpc::eERROR;
  }
  return status;
}

void EmwCoreIpc::poll(const void *argumentPtr, std::uint32_t timeoutInMs) noexcept
{
  EmwNetworkStack::Buffer_t *const network_buffer_ptr = EmwCoreHci::Receive(timeoutInMs);

  static_cast<void>(argumentPtr);
  DEBUG_IPC_LOG("   EmwCoreIpc::poll(%p)>\n", argumentPtr)

  if (network_buffer_ptr != nullptr) {
    uint32_t buffer_payload_size;
    std::uint8_t *const buffer_payload_ptr = EmwNetworkStack::GetBufferPayload(network_buffer_ptr, buffer_payload_size);

    DEBUG_IPC_LOG("   EmwCoreIpc::poll(): %p HCI received length %" PRIu32 "\n",
                  static_cast<const void *>(network_buffer_ptr), buffer_payload_size)

    if (buffer_payload_size >= EmwCoreIpc::PACKET_MIN_SIZE) {
      const std::uint16_t api_id = GetApiId(buffer_payload_ptr);

      DEBUG_IPC_LOG("   EmwCoreIpc::poll(): api_id: 0x%04" PRIx32 "\n", static_cast<std::uint32_t>(api_id))

      if ((api_id & MIPC_API_EVENT_BASE) == 0U) {
        EmwCoreIpc::processResponse(network_buffer_ptr);
      }
      else {
        this->processEvent(network_buffer_ptr, api_id);
      }
    }
    else {
      DEBUG_IPC_LOG("   EmwCoreIpc::poll(): Unknown buffer content\n")
      EmwNetworkStack::FreeBuffer(network_buffer_ptr);
    }
  }
  DEBUG_IPC_LOG("   EmwCoreIpc::poll()<\n\n")
}

void EmwCoreIpc::Poll(void *THIS, const void *argumentPtr, std::uint32_t timeoutInMs) noexcept
{
  (static_cast<EmwCoreIpc *>(THIS))->poll(argumentPtr, timeoutInMs);
}

std::uint8_t *EmwCoreIpc::SkipHeader(std::uint8_t buffer[])
{
  return &buffer[PACKET_PARAMS_OFFSET];
}

void EmwCoreIpc::processResponse(EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept
{
  uint32_t buffer_payload_size;
  std::uint8_t *const buffer_payload_ptr = EmwNetworkStack::GetBufferPayload(networkBufferPtr, buffer_payload_size);
  const std::uint32_t req_id = GetReqId(buffer_payload_ptr);

  DEBUG_IPC_LOG("    EmwCoreIpc::processResponse()> req_id: 0x%04" PRIx32 "\n",
                static_cast<std::uint32_t>(req_id))

  if (req_id == EmwCoreIpc::PendingRequest.reqId) {
    if ((EmwCoreIpc::PendingRequest.responseSizePtr != nullptr) \
        && (*EmwCoreIpc::PendingRequest.responseSizePtr > 0U) \
        && (EmwCoreIpc::PendingRequest.responsePtr != nullptr)) {
      const std::uint32_t response_buffer_size = *EmwCoreIpc::PendingRequest.responseSizePtr;
      const std::uint32_t buffer_size = buffer_payload_size - EmwCoreIpc::PACKET_MIN_SIZE;
      const std::uint32_t actual_size = (response_buffer_size < buffer_size) ? response_buffer_size : buffer_size;

      static_cast<void>(std::memcpy(EmwCoreIpc::PendingRequest.responsePtr,
                                    static_cast<void *>(SkipHeader(buffer_payload_ptr)), actual_size));
      *EmwCoreIpc::PendingRequest.responseSizePtr = static_cast<std::uint16_t>(actual_size);
    }
    EmwCoreIpc::PendingRequest.reqId = REQ_ID_RESET_VAL;
    {
      const EmwOsInterface::Status \
      os_status = EmwOsInterface::ReleaseSemaphore(EmwCoreIpc::PendingRequest.sem);

      if (os_status != EmwOsInterface::eOK) {
        DRIVER_ERROR_VERBOSE("IPC failed to signal command response\n")
      }
      EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
    }
    EMW_STATS_INCREMENT(cmdGetAnswer)
  }
  else {
    DEBUG_IPC_LOG("   EmwCoreIpc::processResponse(): response req_id: 0x%08" PRIx32
                  " not match pending reqId: 0x%08" PRIx32 "!\n",
                  reqId, EmwCoreIpc::PendingRequest.reqId)
  }
  EmwCoreHci::Free(networkBufferPtr);
}


EmwOsInterface::Mutex_t EmwCoreIpc::IpcLock;
bool EmwCoreIpc::IsPowerSaveEnabled = false;
EmwCoreIpc::HciResponse_t EmwCoreIpc::PendingRequest;
