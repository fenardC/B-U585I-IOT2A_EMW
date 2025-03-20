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
#include "EmwApiEmwBypass.hpp"
#include "EmwCoreIpc.hpp"
#include <cinttypes>

#if !defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)
#endif /* EMW_API_DEBUG */

EmwApiBase::Status EmwApiEmwBypass::setByPass(int32_t enable, EmwApiBase::NetlinkInputCallback_t netlinkInputCallback)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcWiFiBypassSetParams_t command_data;
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  uint16_t response_buffer_size = sizeof(response_buffer.status);

  command_data.bypassSetParams.mode = enable;
  if ((nullptr != netlinkInputCallback) && (1 == enable)) {
    this->runtime.netlinkInputCallback = netlinkInputCallback;
  }
  else {
    this->runtime.netlinkInputCallback = nullptr;
  }
  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.bypassSetParams), sizeof(command_data.bypassSetParams),
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  return status;
}

EmwApiBase::Status EmwApiEmwBypass::output(uint8_t data[], uint16_t dataLength, uint32_t interface) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if ((nullptr == data) || (dataLength <= 0) \
      || ((EmwApiBase::eWIFI_INTERFACE_STATION_IDX != interface) \
          && (EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX != interface))) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    const uint16_t command_parameters_size = sizeof(EmwCoreIpc::WiFiBypassOutParams_t) + dataLength;

    if (command_parameters_size > EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      dataLength = static_cast<uint16_t>(EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE - sizeof(
                                           EmwCoreIpc::WiFiBypassOutParams_t));
    }
    {
      EmwCoreIpc::WiFiBypassOutParams_t *command_data_ptr \
        = reinterpret_cast<EmwCoreIpc::WiFiBypassOutParams_t *>(&data[- sizeof(EmwCoreIpc::WiFiBypassOutParams_t)]);
      EmwCoreIpc::SysCommonResponseParams_t response_buffer;
      uint16_t response_buffer_size = sizeof(response_buffer.status);

      EmwCoreIpc::setApiId(EmwCoreIpc::startWithHeader(reinterpret_cast<uint8_t *>(command_data_ptr)),
                           EmwCoreIpc::eWIFI_BYPASS_OUT_CMD);
      command_data_ptr->idx = interface;
      command_data_ptr->dataLength = dataLength;

      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(command_data_ptr), command_parameters_size,
          reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if (0 == response_buffer.status) {
          status = EmwApiBase::eEMW_STATUS_OK;
        }
      }
    }
  }
  return status;
}
