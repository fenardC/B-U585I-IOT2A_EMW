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
#include "EmwApiEmwBypass.hpp"
#include "EmwCoreIpc.hpp"
#include "emw_conf.hpp"
#include <cinttypes>

#if !defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)
#endif /* EMW_API_DEBUG */

#define BYTES_ARRAY_REF(data) reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(data))

EmwApiEmwBypass::EmwApiEmwBypass(void) noexcept
  : EmwApiCore()
{
  DEBUG_API_LOG("\n EmwApiEmwBypass::EmwApiEmwBypass()>\n")
  DEBUG_API_LOG("\n EmwApiEmwBypass::EmwApiEmwBypass()< %p\n\n", static_cast<const void*>(this))
}

EmwApiEmwBypass::~EmwApiEmwBypass(void)
{
  DEBUG_API_LOG("\n EmwApiEmwBypass::~EmwApiEmwBypass()> %p\n\n", static_cast<const void*>(this))
  DEBUG_API_LOG("\n EmwApiEmwBypass::~EmwApiEmwBypass()<\n\n")
}

EmwApiBase::Status EmwApiEmwBypass::setByPass(std::int32_t enable,
    EmwApiBase::NetlinkInputCallback_t netlinkInputCallback) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcWiFiBypassSetParams_t command_data;
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n EmwApiEmwBypass::setByPass()>\n");

  command_data.bypassSetParams.mode = enable;
  if ((nullptr != netlinkInputCallback) && (1 == enable)) {
    this->EmwApiCore::callbacks.netlinkInputCallback = netlinkInputCallback;
  }
  else {
    this->EmwApiCore::callbacks.netlinkInputCallback = nullptr;
  }
  if (EmwCoreIpc::eSUCCESS == this->EmwCoreIpc::request(BYTES_ARRAY_REF(&command_data), sizeof(command_data),
      BYTES_ARRAY_REF(&response_buffer), response_buffer_size, EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("\n EmwApiEmwBypass::setByPass()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))

  return status;
}

EmwApiBase::Status EmwApiEmwBypass::output(std::uint8_t *dataPtr, std::uint16_t dataLength,
    std::uint32_t interface) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  DEBUG_API_LOG("\n EmwApiEmwBypass::output()>\n");

  if ((nullptr == dataPtr) || (dataLength == 0) \
      || ((EmwApiBase::eWIFI_INTERFACE_STATION_IDX != interface) \
          && (EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX != interface))) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    std::uint16_t command_data_size = sizeof(EmwCoreIpc::IpcWiFiBypassOutParams_t) + dataLength;

    if (EmwNetworkStack::NETWORK_BUFFER_SIZE < command_data_size) {
      command_data_size = EmwNetworkStack::NETWORK_BUFFER_SIZE;
      dataLength = static_cast<std::uint16_t>(command_data_size - sizeof(EmwCoreIpc::IpcWiFiBypassOutParams_t));
    }
    {
      EmwCoreIpc::IpcWiFiBypassOutParams_t *command_data_ptr \
        = reinterpret_cast<EmwCoreIpc::IpcWiFiBypassOutParams_t *>(dataPtr - sizeof(EmwCoreIpc::IpcWiFiBypassOutParams_t));
      const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::EmwCoreIpc::eWIFI_BYPASS_OUT_CMD);
      EmwCoreIpc::SysCommonResponseParams_t response_buffer;
      std::uint16_t response_buffer_size = sizeof(response_buffer);

      command_data_ptr->ipcParams = ipc_params;
      command_data_ptr->bypassOutParams.idx = interface;
      command_data_ptr->bypassOutParams.dataLength = dataLength;

      if (EmwCoreIpc::eSUCCESS == this->EmwCoreIpc::request(BYTES_ARRAY_REF(command_data_ptr), command_data_size,
          BYTES_ARRAY_REF(&response_buffer), response_buffer_size, EMW_CMD_TIMEOUT)) {
        if (0 == response_buffer.status) {
          status = EmwApiBase::eEMW_STATUS_OK;
        }
      }
    }
  }
  DEBUG_API_LOG("\n EmwApiEmwBypass::output()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))

  return status;
}
