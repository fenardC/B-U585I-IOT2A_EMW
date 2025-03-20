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
#include "EmwApiEmw.hpp"
#include <cstdint>
#include <cinttypes>
#include <cstdbool>
#include <cstring>
#include <memory>

#if !defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)
#endif /* EMW_API_DEBUG */

template<typename T> void copyStringToArray(T destination[], size_t destinationCount,
    const char *sourceStringPtr);

#define STRING_COPY_TO_ARRAY_CHAR(DEST, SRC)                          \
  do {                                                                \
    copyStringToArray<char>((DEST), sizeof((DEST)), (SRC));           \
  } while(false)

#define VOID_MEMSET_ARRAY(DEST, VAL)  (void) memset((void *)(DEST), (int)(VAL), sizeof((DEST)))

int32_t EmwApiEmw::socketClose(int32_t socketFd)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketClose()>\n")

  if ((0 <= socketFd)) {
    EmwCoreIpc::IpcSocketCloseParams_t command_data;
    EmwCoreIpc::SocketCloseResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);
    status = -1;
    command_data.closeParams.filedes = socketFd;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.closeParams), sizeof(command_data.closeParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketClose()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::socketCreate(int32_t domain, int32_t type, int32_t protocol)
{
  int32_t ret_fd = -1;
  EmwCoreIpc::EmwCoreIpc::IpcSocketCreateParams_t command_data(domain, type, protocol);
  EmwCoreIpc::SocketCreateResponseParams_t response_buffer;
  uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\nEmwApiEmw::socketCreate()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.createParams), sizeof(command_data.createParams),
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    ret_fd = response_buffer.fd;
  }
  DEBUG_API_LOG("EmwApiEmw::socketCreate()< %" PRIi32 "\n\n", ret_fd)
  return ret_fd;
}

int32_t EmwApiEmw::socketConnect(int32_t socketFd,
                                 const EmwAddress::SockAddr_t *socketAddressPtr, int32_t socketAddressLength)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketConnect()>\n")

  if ((0 <= socketFd) && (nullptr != socketAddressPtr) && (0 < socketAddressLength)) {
    EmwCoreIpc::IpcSocketConnectParams_t command_data;
    bool is_to_do_ipc_request = true;

    status = -1;
    if ((EMW_AF_INET == socketAddressPtr->family) \
        && (socketAddressLength == sizeof(EmwAddress::SockAddrIn_t))) {
      command_data.connectParams.addr = socketAddressIn_ToPacked(socketAddressPtr);
    }
    else if ((EMW_AF_INET6 == socketAddressPtr->family) \
             && (socketAddressLength == sizeof(EmwAddress::SockAddrIn6_t))) {
      command_data.connectParams.addr = socketAddressIn6_ToPacked(socketAddressPtr);
    }
    else {
      is_to_do_ipc_request = false;
    }
    if (is_to_do_ipc_request) {
      EmwCoreIpc::SocketConnectResponseParams_t response_buffer;
      uint16_t response_buffer_size = sizeof(response_buffer);

      command_data.connectParams.socket = socketFd;
      command_data.connectParams.length = static_cast<EmwAddress::SockLen_t>(socketAddressLength);
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(&command_data.connectParams), sizeof(command_data.connectParams),
          reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if (0 == response_buffer.status) {
          status = 0;
        }
        else {
          DEBUG_API_LOG("EmwApiEmw::socketConnect(): %" PRIi32 "\n\n", static_cast<int32_t>(response_buffer.status))
        }
      }
    }
  }
  return status;
}

int32_t EmwApiEmw::socketGetAddrInfo(const char *nodeNameStringPtr, const char *serviceNameStringPtr,
                                     const EmwAddress::AddrInfo_t &hints, EmwAddress::AddrInfo_t *resultPtr)
{
  int32_t ret = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketGetAddrInfo()>\n")

  if ((nullptr != nodeNameStringPtr) && (nullptr != resultPtr)) {
    EmwCoreIpc::IpcSocketGetAddrInfoParam_t command_data;
    EmwCoreIpc::SocketGetAddrInfoResponseParam_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);

    ret = -1;
    STRING_COPY_TO_ARRAY_CHAR(command_data.getAddrInfoParams.nodeName, nodeNameStringPtr);
    if (nullptr != serviceNameStringPtr) {
      STRING_COPY_TO_ARRAY_CHAR(command_data.getAddrInfoParams.serviceName, serviceNameStringPtr);
    }
    command_data.getAddrInfoParams.hints = hints;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.getAddrInfoParams), sizeof(command_data.getAddrInfoParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        resultPtr->flags = response_buffer.res.flags;
        resultPtr->family = response_buffer.res.family;
        resultPtr->sockType = response_buffer.res.sockType;
        resultPtr->protocol = response_buffer.res.protocol;
        resultPtr->addrLen = response_buffer.res.addrLen;
        resultPtr->sAddr.length = response_buffer.res.sAddr.length;
        resultPtr->sAddr.family = response_buffer.res.sAddr.family;
        /* Keep port possibly set as it is. */
        resultPtr->sAddr.data2[0] = response_buffer.res.sAddr.data2[0];
        resultPtr->sAddr.data2[1] = response_buffer.res.sAddr.data2[1];
        resultPtr->sAddr.data2[2] = response_buffer.res.sAddr.data2[2];
        resultPtr->sAddr.data3[0] = response_buffer.res.sAddr.data3[0];
        resultPtr->sAddr.data3[1] = response_buffer.res.sAddr.data3[1];
        resultPtr->sAddr.data3[2] = response_buffer.res.sAddr.data3[2];
        (void) memcpy(resultPtr->canonName, response_buffer.res.canonName, sizeof(resultPtr->canonName));
        resultPtr->canonName[sizeof(resultPtr->canonName) - 1] = '\0';
        ret = 0;
      }
      else {
        DEBUG_API_LOG("EmwApiEmw::socketGetAddrInfo(): %" PRIi32 "\n\n", static_cast<int32_t>(response_buffer.status))
      }
    }
  }
  return ret;
}

int32_t EmwApiEmw::socketGetHostByName(EmwAddress::SockAddr_t *socketAddressPtr, const char *nameStringPtr)
{
  int32_t status = -4;
  EmwCoreIpc::IpcSocketGetHostByNameParams_t command_data;
  DEBUG_API_LOG("\nEmwApiEmw::socketGetHostByName()>\n")

  if ((nullptr != socketAddressPtr) && (nullptr != nameStringPtr)) {
    if (strlen(nameStringPtr) < sizeof(command_data.getHostByNameParams.name)) {
      EmwCoreIpc::SocketGetHostByNameResponseParams_t response_buffer;
      uint16_t response_buffer_size = sizeof(response_buffer);

      status = -1;
      STRING_COPY_TO_ARRAY_CHAR(command_data.getHostByNameParams.name, nameStringPtr);
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(&command_data.getHostByNameParams), sizeof(command_data.getHostByNameParams),
          reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if (0 == response_buffer.status) {
          /* Only for IPv4 address. */
          EmwAddress::SockAddrIn_t *const socket_address_in_ptr = reinterpret_cast<EmwAddress::SockAddrIn_t *>(socketAddressPtr);
          socket_address_in_ptr->length = static_cast<uint8_t>(sizeof(*socket_address_in_ptr));
          socket_address_in_ptr->family = EMW_AF_INET;
          socket_address_in_ptr->inAddr.addr = response_buffer.s_addr;
          status = 0;
        }
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketGetHostByName()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::socketGetSockOpt(int32_t socketFd, int32_t level,
                                    int32_t optionName, void *optionValuePtr, uint32_t *optionLengthPtr)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketGetSockOpt()>\n")

  if ((0 <= socketFd) && (nullptr != optionValuePtr) && (nullptr != optionLengthPtr)) {
    EmwCoreIpc::EmwCoreIpc::IpcSocketGetSockOptParams_t command_data(socketFd, level, optionName);
    EmwCoreIpc::SocketGetSockOptResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.getSockOptParams), sizeof(command_data.getSockOptParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        *optionLengthPtr = (response_buffer.length > *optionLengthPtr) ? *optionLengthPtr : response_buffer.length;
        (void) memcpy(optionValuePtr, &response_buffer.value[0], *optionLengthPtr);
        status = 0;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketGetSockOpt()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::socketPing(const char *hostnameStringPtr,
                              int32_t count, int32_t delayInMs, int32_t responses[])
{
  DEBUG_API_LOG("\nEmwApiEmw::socketPing()>\n")
  return this->doSocketPing(EmwCoreIpc::eWIFI_PING_CMD, hostnameStringPtr, count, delayInMs, responses);
}

int32_t EmwApiEmw::socketPing6(const char *hostnameStringPtr,
                               int32_t count, int32_t delayInMs, int32_t responses[])
{
  DEBUG_API_LOG("\nEmwApiEmw::socketPing6()>\n")
  return this->doSocketPing(EmwCoreIpc::eWIFI_PING6_CMD, hostnameStringPtr, count, delayInMs, responses);
}

int32_t EmwApiEmw::socketSend(int32_t socketFd, const uint8_t *dataPtr, int32_t dataLength, int32_t flags)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketSend()>\n")
  if ((0 <= socketFd) && (nullptr != dataPtr) && (0 < dataLength)) {
    EmwCoreIpc::SocketSendResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);
    size_t data_length = static_cast<size_t>(dataLength);
    status = -1;
    if ((data_length + sizeof(EmwCoreIpc::IpcSocketSendParams_t) - 1U) > EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      data_length = EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE - (sizeof(EmwCoreIpc::IpcSocketSendParams_t) - 1U);
    }
    {
      const uint16_t command_data_size = static_cast<uint16_t>(sizeof(EmwCoreIpc::IpcSocketSendParams_t) - 1U + data_length);
      const size_t command_ipc_data_size = sizeof(EmwCoreIpc::CmdParams_t) + command_data_size;
      std::unique_ptr<EmwCoreIpc::IpcSocketSendParams_t, decltype(&EmwOsInterface::free)> \
      command_data_ptr(static_cast<EmwCoreIpc::IpcSocketSendParams_t *>\
                       (EmwOsInterface::malloc(command_ipc_data_size)), &EmwOsInterface::free);
      const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eSOCKET_SEND_CMD);

      command_data_ptr->ipcParams = ipc_params;
      command_data_ptr->sendParams.socket = socketFd;
      (void) memcpy(&command_data_ptr->sendParams.buffer[0], dataPtr, data_length);
      command_data_ptr->sendParams.size = data_length;
      command_data_ptr->sendParams.flags = flags;
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(&command_data_ptr->sendParams), command_data_size,
          reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        status = response_buffer.sent;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketSend()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::socketSetSockOpt(int32_t socketFd, int32_t level,
                                    int32_t optionName, const void *optionValuePtr, int32_t optionLength)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketSetSockOpt()>\n")
  if ((0 <= socketFd) && (nullptr != optionValuePtr) && (0 < optionLength)) {
    EmwCoreIpc::IpcSocketSetSockOptParams_t command_data;
    EmwCoreIpc::SocketSetSockOptResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    command_data.setSockOptParams.socket = socketFd;
    command_data.setSockOptParams.level = level;
    command_data.setSockOptParams.name = optionName;
    command_data.setSockOptParams.length =
      (static_cast<size_t>(optionLength) > sizeof(command_data.setSockOptParams.value)) \
      ? static_cast<EmwAddress::SockLen_t>(sizeof(command_data.setSockOptParams.value)) \
      : static_cast<EmwAddress::SockLen_t>(optionLength);
    (void)memcpy(&command_data.setSockOptParams.value[0], optionValuePtr, command_data.setSockOptParams.length);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.setSockOptParams), sizeof(command_data.setSockOptParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketSetSockOpt()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::socketShutDown(int32_t socketFd, int32_t mode)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketShutDown()>\n")

  if (0 <= socketFd) {
    EmwCoreIpc::IpcSocketShutDownParams_t command_data(socketFd, mode);
    EmwCoreIpc::SocketShutDownResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.shutDownParams), sizeof(command_data.shutDownParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketShutDown()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::socketReceive(int32_t socketFd, uint8_t *bufferPtr, int32_t bufferLength, int32_t flags)
{
  int32_t status = -4;
  DEBUG_API_LOG("\nEmwApiEmw::socketReceive()>\n")

  if ((0 <= socketFd) && (nullptr != bufferPtr) && (0 < bufferLength)) {
    size_t data_length = static_cast<size_t>(bufferLength);
    uint16_t response_buffer_size;

    status = -1;
    if ((data_length + sizeof(EmwCoreIpc::SocketReceiveResponseParams_t) - 1U) >
        EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      data_length = EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE - (sizeof(EmwCoreIpc::SocketReceiveResponseParams_t) - 1U);
    }
    response_buffer_size = static_cast<uint16_t>(sizeof(EmwCoreIpc::SocketReceiveResponseParams_t) - 1U + data_length);
    {
      std::unique_ptr<EmwCoreIpc::SocketReceiveResponseParams_t, decltype(&EmwOsInterface::free)> \
      response_buffer_ptr(static_cast<EmwCoreIpc::SocketReceiveResponseParams_t *> \
                          (EmwOsInterface::malloc(response_buffer_size)), &EmwOsInterface::free);
      EmwCoreIpc::IpcSocketReceiveParams_t command_data(socketFd, data_length, flags);

      response_buffer_ptr->received = 0;
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(&command_data.receiveParams), sizeof(command_data.receiveParams),
          reinterpret_cast<uint8_t *>(response_buffer_ptr.get()), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if (response_buffer_ptr->received > 0) {
          const size_t received_len = static_cast<size_t>(response_buffer_ptr->received);
          if (received_len <= data_length) {
            (void) memcpy(bufferPtr, &response_buffer_ptr->buffer[0], received_len);
          }
        }
        status = response_buffer_ptr->received;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::socketReceive()< %" PRIi32 "\n\n", status)
  return status;
}

int32_t EmwApiEmw::doSocketPing(EmwCoreIpc::ApiId apiId,
                                const char *hostnameStringPtr,
                                int32_t count, int32_t delayInMs, int32_t responses[])
{
  int32_t status = -4;
  if ((nullptr != hostnameStringPtr) && (0 < count)) {
    EmwCoreIpc::IpcWiFiPingParams_t command_data(apiId);
    EmwCoreIpc::WiFiPingResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(EmwCoreIpc::WiFiPingResponseParams_t);
    const int32_t count_max = sizeof(response_buffer.delaysInMs) / sizeof(response_buffer.delaysInMs[0]);

    status = -1;
    STRING_COPY_TO_ARRAY_CHAR(command_data.pingParams.hostname, hostnameStringPtr);
    command_data.pingParams.count = (count <= count_max) ? count : count_max;
    command_data.pingParams.delayInMs = delayInMs;
    response_buffer.numberOf = 0;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.pingParams), sizeof(command_data.pingParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      if (response_buffer.numberOf > 0) {
        for (int32_t i = 0; i < response_buffer.numberOf; i++) {
          responses[i] = response_buffer.delaysInMs[i];
        }
        status = 0;
      }
    }
  }
  DEBUG_API_LOG("EmwApiEmw::doSocketPing()< %" PRIi32 "\n", status)
  return status;
}

EmwAddress::SockAddrIn_t EmwApiEmw::socketAddressIn_FromPacked(const EmwAddress::SockAddrStorage_t *socketAddressPtr)
{
  EmwAddress::SockAddrIn_t s_addr_in;

  s_addr_in.length = socketAddressPtr->length;
  s_addr_in.family = socketAddressPtr->family;
  {
    uint16_t port_in = socketAddressPtr->data1[0];
    port_in |= socketAddressPtr->data1[1] << 8;
    s_addr_in.port = port_in;
  }
  s_addr_in.inAddr.addr = socketAddressPtr->data2[0];
  return s_addr_in;
}

EmwAddress::SockAddrIn6_t EmwApiEmw::socketAddressIn6_FromPacked(const EmwAddress::SockAddrStorage_t *socketAddressPtr)
{
  EmwAddress::SockAddrIn6_t s_address_in6;

  s_address_in6.length = socketAddressPtr->length;
  s_address_in6.family = socketAddressPtr->family;
  {
    uint16_t port_in = socketAddressPtr->data1[0];
    port_in |= socketAddressPtr->data1[1] << 8;
    s_address_in6.port = port_in;
  }
  s_address_in6.flowInfo = socketAddressPtr->data2[0];
  s_address_in6.in6Addr.un.u32Addr[0] = socketAddressPtr->data2[1];
  s_address_in6.in6Addr.un.u32Addr[1] = socketAddressPtr->data2[2];
  s_address_in6.in6Addr.un.u32Addr[2] = socketAddressPtr->data3[0];
  s_address_in6.in6Addr.un.u32Addr[3] = socketAddressPtr->data3[1];
  s_address_in6.scopeId = socketAddressPtr->data3[2];
  return s_address_in6;
}

EmwAddress::SockAddrStorage_t EmwApiEmw::socketAddressIn_ToPacked(const EmwAddress::SockAddr_t *socketAddressPtr)
{
  EmwAddress::SockAddrStorage_t s_address_storage;
  const EmwAddress::SockAddrIn_t *const s_address_in_ptr \
    = reinterpret_cast<const EmwAddress::SockAddrIn_t *>(socketAddressPtr);

  s_address_storage.length = s_address_in_ptr->length;
  s_address_storage.family = s_address_in_ptr->family;
  s_address_storage.data1[0] = static_cast<uint8_t>(s_address_in_ptr->port);
  s_address_storage.data1[1] = static_cast<uint8_t>(s_address_in_ptr->port >> 8);
  s_address_storage.data2[0] = s_address_in_ptr->inAddr.addr;
  return s_address_storage;
}

EmwAddress::SockAddrStorage_t EmwApiEmw::socketAddressIn6_ToPacked(const EmwAddress::SockAddr_t *socketAddressPtr)
{
  EmwAddress::SockAddrStorage_t s_address_storage;
  const EmwAddress::SockAddrIn6_t *const s_address_in6_ptr \
    = reinterpret_cast<const EmwAddress::SockAddrIn6_t *>(socketAddressPtr);

  s_address_storage.length = s_address_in6_ptr->length;
  s_address_storage.family = s_address_in6_ptr->family;
  s_address_storage.data1[0] = static_cast<uint8_t>(s_address_in6_ptr->port);
  s_address_storage.data1[1] = static_cast<uint8_t>(s_address_in6_ptr->port >> 8);
  s_address_storage.data2[0] = s_address_in6_ptr->flowInfo;
  s_address_storage.data2[1] = s_address_in6_ptr->in6Addr.un.u32Addr[0];
  s_address_storage.data2[2] = s_address_in6_ptr->in6Addr.un.u32Addr[1];
  s_address_storage.data3[0] = s_address_in6_ptr->in6Addr.un.u32Addr[2];
  s_address_storage.data3[1] = s_address_in6_ptr->in6Addr.un.u32Addr[3];
  s_address_storage.data3[2] = s_address_in6_ptr->scopeId;
  return s_address_storage;
}

template<typename T> void copyStringToArray(T destination[], size_t destinationCount,
    const char *sourceStringPtr)
{
  const size_t index_max = destinationCount;
  const char *source_char_ptr = sourceStringPtr;
  size_t index;

  for (index = 0U; index < (index_max - 1); index++) {
    const char source_char = *source_char_ptr;
    destination[index] = static_cast<T>(source_char);
    source_char_ptr++;
    if (source_char == '\0') {
      break;
    }
  }
  for (; index < index_max; index++) {
    destination[index] = '\0';
  }
}
