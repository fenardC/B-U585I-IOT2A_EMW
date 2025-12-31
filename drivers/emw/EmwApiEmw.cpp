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
#include "emw_conf.hpp"
#include "EmwApiEmw.hpp"
#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <memory>

#if !defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)
#endif /* EMW_API_DEBUG */

#define BYTES_REF_CAST(data) reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(data))

template<typename T> void copyStringToArray(T destination[], std::size_t destinationCount,
    const char *sourceStringPtr);

#define STRING_COPY_TO_ARRAY_CHAR(DEST, SRC)                          \
  do {                                                                \
    copyStringToArray<char>((DEST), sizeof((DEST)), (SRC));           \
  } while(false)


EmwApiEmw::EmwApiEmw(void) noexcept
  : EmwApiCore()
{
  DEBUG_API_LOG("\n EmwApiEmw::EmwApiEmw()>\n")
  DEBUG_API_LOG("\n EmwApiEmw::EmwApiEmw()< %p\n\n", static_cast<const void*>(this))
}

EmwApiEmw::~EmwApiEmw(void)
{
  DEBUG_API_LOG("\n EmwApiEmw::~EmwApiEmw()> %p\n\n", static_cast<const void*>(this))
  DEBUG_API_LOG("\n EmwApiEmw::~EmwApiEmw()<\n\n")
}

std::int32_t EmwApiEmw::socketClose(std::int32_t socketFd) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::socketClose()>\n")

  if ((0 <= socketFd)) {
    EmwCoreIpc::IpcSocketCloseParams_t command_data;
    EmwCoreIpc::SocketCloseResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    command_data.closeParams.filedes = socketFd;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketClose()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::socketCreate(std::int32_t domain, std::int32_t type, std::int32_t protocol) noexcept
{
  std::int32_t ret_fd = -1;
  EmwCoreIpc::EmwCoreIpc::IpcSocketCreateParams_t command_data(domain, type, protocol);
  EmwCoreIpc::SocketCreateResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n EmwApiEmw::socketCreate()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
      BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
    ret_fd = response_buffer.fd;
  }
  DEBUG_API_LOG(" EmwApiEmw::socketCreate()< %" PRIi32 "\n\n", ret_fd)
  return ret_fd;
}

std::int32_t EmwApiEmw::socketConnect(std::int32_t socketFd,
                                      const EmwAddress::SockAddr_t &socketAddress, std::int32_t socketAddressSize) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::socketConnect()>\n")

  if ((0 <= socketFd) && (0 < socketAddressSize)) {
    EmwCoreIpc::IpcSocketConnectParams_t command_data;
    bool is_to_do_ipc_request = true;

    status = -1;
    if ((EMW_AF_INET == socketAddress.family) && (socketAddressSize == sizeof(EmwAddress::SockAddrIn_t))) {
      command_data.connectParams.addr = socketAddressIn_ToPacked(socketAddress);
    }
    else if ((EMW_AF_INET6 == socketAddress.family) \
             && (socketAddressSize == sizeof(EmwAddress::SockAddrIn6_t))) {
      command_data.connectParams.addr = socketAddressIn6_ToPacked(socketAddress);
    }
    else {
      is_to_do_ipc_request = false;
    }
    if (is_to_do_ipc_request) {
      EmwCoreIpc::SocketConnectResponseParams_t response_buffer;
      std::uint16_t response_buffer_size = sizeof(response_buffer);

      command_data.connectParams.socket = socketFd;
      command_data.connectParams.length = static_cast<EmwAddress::SockLen_t>(socketAddressSize);
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
          BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
        if (0 == response_buffer.status) {
          status = 0;
        }
        else {
          DEBUG_API_LOG(" EmwApiEmw::socketConnect(): %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
        }
      }
    }
  }
  return status;
}

std::int32_t EmwApiEmw::socketGetAddrInfo(const char (&nodeNameString)[255], const char (&serviceNameString)[255],
    const EmwAddress::AddrInfo_t &hints, EmwAddress::AddrInfo_t &result) noexcept
{
  std::int32_t ret = -1;
  EmwCoreIpc::IpcSocketGetAddrInfoParam_t command_data;
  EmwCoreIpc::SocketGetAddrInfoResponseParam_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\nEmwApiEmw::socketGetAddrInfo()>\n")

  STRING_COPY_TO_ARRAY_CHAR(command_data.getAddrInfoParams.nodeName, nodeNameString);
  STRING_COPY_TO_ARRAY_CHAR(command_data.getAddrInfoParams.serviceName, serviceNameString);
  command_data.getAddrInfoParams.hints = hints;

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
      BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      result.flags = response_buffer.res.flags;
      result.family = response_buffer.res.family;
      result.sockType = response_buffer.res.sockType;
      result.protocol = response_buffer.res.protocol;
      result.addrLen = response_buffer.res.addrLen;
      result.sAddr.length = response_buffer.res.sAddr.length;
      result.sAddr.family = response_buffer.res.sAddr.family;
      /* Keep port possibly set as it is. */
      result.sAddr.data2[0] = response_buffer.res.sAddr.data2[0];
      result.sAddr.data2[1] = response_buffer.res.sAddr.data2[1];
      result.sAddr.data2[2] = response_buffer.res.sAddr.data2[2];
      result.sAddr.data3[0] = response_buffer.res.sAddr.data3[0];
      result.sAddr.data3[1] = response_buffer.res.sAddr.data3[1];
      result.sAddr.data3[2] = response_buffer.res.sAddr.data3[2];
      static_cast<void>(std::memcpy(result.canonName, response_buffer.res.canonName, sizeof(result.canonName)));
      result.canonName[sizeof(result.canonName) - 1] = '\0';
      ret = 0;
    }
    else {
      DEBUG_API_LOG(" EmwApiEmw::socketGetAddrInfo(): %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
    }
  }
  return ret;
}

std::int32_t EmwApiEmw::socketGetHostByName(EmwAddress::SockAddr_t &socketAddress,
    const char (&nameString)[255]) noexcept
{
  std::int32_t status = -4;
  EmwCoreIpc::IpcSocketGetHostByNameParams_t command_data;

  DEBUG_API_LOG("\n EmwApiEmw::socketGetHostByName()>\n")

  if (std::strlen(nameString) < sizeof(command_data.getHostByNameParams.name)) {
    EmwCoreIpc::SocketGetHostByNameResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    STRING_COPY_TO_ARRAY_CHAR(command_data.getHostByNameParams.name, nameString);
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        /* Only for IPv4 address. */
        EmwAddress::SockAddrIn_t &socket_address_in = reinterpret_cast<EmwAddress::SockAddrIn_t &>(socketAddress);
        socket_address_in.length = sizeof(socket_address_in);
        socket_address_in.family = EMW_AF_INET;
        socket_address_in.inAddr.addr = response_buffer.s_addr;
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketGetHostByName()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::socketGetSockOpt(std::int32_t socketFd, std::int32_t level,
    std::int32_t optionName, void *optionValuePtr, std::uint32_t &optionLength) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::socketGetSockOpt()>\n")

  if ((0 <= socketFd) && (nullptr != optionValuePtr)) {
    EmwCoreIpc::EmwCoreIpc::IpcSocketGetSockOptParams_t command_data(socketFd, level, optionName);
    EmwCoreIpc::SocketGetSockOptResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        optionLength = (response_buffer.length > optionLength) ? optionLength : response_buffer.length;
        static_cast<void>(std::memcpy(optionValuePtr, &response_buffer.value[0], optionLength));
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketGetSockOpt()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::socketPing(const char (&hostnameString)[16],
                                   std::int32_t count, std::int32_t delayInMs, std::int32_t (&responses)[10]) noexcept
{
  DEBUG_API_LOG("\n EmwApiEmw::socketPing()>\n")

  return this->doSocketPing(EmwCoreIpc::eWIFI_PING_CMD, hostnameString, count, delayInMs, responses);
}

std::int32_t EmwApiEmw::socketPing6(const char (&hostnameString)[40],
                                    std::int32_t count, std::int32_t delayInMs, std::int32_t (&responses)[10]) noexcept
{
  DEBUG_API_LOG("\n EmwApiEmw::socketPing6()>\n")

  return this->doSocketPing(EmwCoreIpc::eWIFI_PING6_CMD, hostnameString, count, delayInMs, responses);
}

std::int32_t EmwApiEmw::socketSend(std::int32_t socketFd, const std::uint8_t (&data)[], std::int32_t dataLength,
                                   std::int32_t flags) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\nEmwApiEmw::socketSend()> %" PRIi32 "\n", dataLength)

  if ((0 <= socketFd) && (0 < dataLength)) {
    EmwCoreIpc::SocketSendResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);
    std::size_t data_length = static_cast<std::size_t>(dataLength);

    status = -1;
    if ((data_length + sizeof(EmwCoreIpc::IpcSocketSendParams_t) - 1U) > EmwNetworkStack::NETWORK_BUFFER_SIZE) {
      data_length = EmwNetworkStack::NETWORK_BUFFER_SIZE - (sizeof(EmwCoreIpc::IpcSocketSendParams_t) - 1U);
    }
    {
      const std::uint16_t command_data_size \
        = static_cast<std::uint16_t>(sizeof(EmwCoreIpc::IpcSocketSendParams_t) - 1U + data_length);
      std::unique_ptr<EmwCoreIpc::IpcSocketSendParams_t, decltype(&EmwOsInterface::Free)> \
      command_data_ptr(static_cast<EmwCoreIpc::IpcSocketSendParams_t *>\
                       (EmwOsInterface::Malloc(command_data_size)), &EmwOsInterface::Free);
      const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eSOCKET_SEND_CMD);

      command_data_ptr->ipcParams = ipc_params;
      command_data_ptr->sendParams.socket = socketFd;
      static_cast<void>(std::memcpy(&command_data_ptr->sendParams.buffer[0], data, data_length));
      command_data_ptr->sendParams.size = data_length;
      command_data_ptr->sendParams.flags = flags;
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(command_data_ptr.get()), command_data_size,
          BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
        status = response_buffer.sent;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketSend()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::socketSetSockOpt(std::int32_t socketFd, std::int32_t level,
    std::int32_t optionName, const void *optionValuePtr, std::int32_t optionLength) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::socketSetSockOpt()>\n")

  if ((0 <= socketFd) && (nullptr != optionValuePtr) && (0 < optionLength)) {
    EmwCoreIpc::IpcSocketSetSockOptParams_t command_data(socketFd, level, optionName);
    EmwCoreIpc::SocketSetSockOptResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    command_data.setSockOptParams.length =
      (static_cast<size_t>(optionLength) > sizeof(command_data.setSockOptParams.value)) \
      ? static_cast<EmwAddress::SockLen_t>(sizeof(command_data.setSockOptParams.value)) \
      : static_cast<EmwAddress::SockLen_t>(optionLength);
    static_cast<void>(std::memcpy(&command_data.setSockOptParams.value[0], optionValuePtr,
                                  command_data.setSockOptParams.length));

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketSetSockOpt()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::socketShutDown(std::int32_t socketFd, std::int32_t mode) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::socketShutDown()>\n")

  if (0 <= socketFd) {
    EmwCoreIpc::IpcSocketShutDownParams_t command_data(socketFd, mode);
    EmwCoreIpc::SocketShutDownResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    status = -1;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketShutDown()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::socketReceive(std::int32_t socketFd, std::uint8_t (&buffer)[], std::int32_t bufferLength,
                                      std::int32_t flags) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::socketReceive()> %" PRIi32 "\n", bufferLength)

  if ((0 <= socketFd) && (0 < bufferLength)) {
    std::size_t data_length = static_cast<std::size_t>(bufferLength);
    std::uint16_t response_buffer_size;

    status = -1;
    if ((data_length + sizeof(EmwCoreIpc::SocketReceiveResponseParams_t) - 1U) >
        EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      data_length = EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE - (sizeof(EmwCoreIpc::SocketReceiveResponseParams_t) - 1U);
    }
    response_buffer_size = static_cast<std::uint16_t>(sizeof(EmwCoreIpc::SocketReceiveResponseParams_t) - 1U + data_length);
    {
      std::unique_ptr<EmwCoreIpc::SocketReceiveResponseParams_t, decltype(&EmwOsInterface::Free)> \
      response_buffer_ptr(static_cast<EmwCoreIpc::SocketReceiveResponseParams_t *> \
                          (EmwOsInterface::Malloc(response_buffer_size)), &EmwOsInterface::Free);
      EmwCoreIpc::IpcSocketReceiveParams_t command_data(socketFd, data_length, flags);

      response_buffer_ptr->received = 0;
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
          BYTES_REF_CAST(response_buffer_ptr.get()), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
        if (response_buffer_ptr->received > 0) {
          const std::size_t received_length = static_cast<std::size_t>(response_buffer_ptr->received);

          if (received_length <= data_length) {
            static_cast<void>(std::memcpy(&buffer[0], &response_buffer_ptr->buffer[0], received_length));
          }
        }
        status = response_buffer_ptr->received;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::socketReceive()< %" PRIi32 "\n\n", status)

  return status;
}

std::int32_t EmwApiEmw::tlsSetVersion(EmwApiEmw::TlsVersion version) noexcept
{
  std::int32_t status = -1;
  EmwCoreIpc::IpcTlsSetVersionParams_t command_data(version);
  EmwCoreIpc::TlsSetVersionResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n EmwApiEmw::tlsSetVersion()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
      BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      status = 0;
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsSetVersion()< %" PRIi32 "\n\n", status)
  return status;
}

std::int32_t EmwApiEmw::tlsSetClientCertificate(const std::uint8_t (&certificate)[],
    std::uint16_t certificateLength) noexcept
{
  std::int32_t status = -1;
  const std::uint16_t command_data_size = sizeof(EmwCoreIpc::TlsSetClientCertificateParams_t) - 1U + certificateLength;

  DEBUG_API_LOG("\n EmwApiEmw::tlsSetClientCertificate()>\n")

  if ((0U == certificateLength) || (command_data_size > EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE)) {
    status = -4;
  }
  else {
    EmwCoreIpc::TlsSetClientCertificateResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);
    const std::uint16_t command_ipc_data_size = sizeof(EmwCoreIpc::CmdParams_t) + command_data_size;
    std::unique_ptr<EmwCoreIpc::IpcTlsSetClientCertificateParams_t, decltype(&EmwOsInterface::Free)> \
    command_data_ptr(static_cast<EmwCoreIpc::IpcTlsSetClientCertificateParams_t *>\
                     (EmwOsInterface::Malloc(command_ipc_data_size)), &EmwOsInterface::Free);
    const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eTLS_SET_CLIENT_CERTIFICATE_CMD);

    command_data_ptr->ipcParams = ipc_params;

    command_data_ptr->setClientCertificateParams.certificatePemSize = certificateLength;
    command_data_ptr->setClientCertificateParams.privateKeyPemSize = 0U;
    static_cast<void>(std::memcpy(&command_data_ptr->setClientCertificateParams.certificateData[0], certificate,
                                  certificateLength));

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(command_data_ptr.get()), command_ipc_data_size,
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsSetClientCertificate()< %" PRIi32 "\n\n", status)

  return status;
}


std::int32_t EmwApiEmw::tlsSetClientPrivateKey(const std::uint8_t (&privateKey)[],
    std::uint16_t privateKeyLength) noexcept
{
  std::int32_t status = -1;
  const std::uint16_t command_data_size = sizeof(EmwCoreIpc::TlsSetClientCertificateParams_t) - 1U + privateKeyLength;

  DEBUG_API_LOG("\n EmwApiEmw::tlsSetClientPrivateKey()>\n")

  if ((privateKeyLength == 0U) || (command_data_size > EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE)) {
    status = -4;
  }
  else {
    EmwCoreIpc::TlsSetClientCertificateResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);
    const std::uint16_t command_ipc_data_size = sizeof(EmwCoreIpc::CmdParams_t) + command_data_size;
    std::unique_ptr<EmwCoreIpc::IpcTlsSetClientCertificateParams_t, decltype(&EmwOsInterface::Free)> \
    command_data_ptr(static_cast<EmwCoreIpc::IpcTlsSetClientCertificateParams_t *>\
                     (EmwOsInterface::Malloc(command_ipc_data_size)), &EmwOsInterface::Free);
    const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eTLS_SET_CLIENT_CERTIFICATE_CMD);

    command_data_ptr->ipcParams = ipc_params;

    command_data_ptr->setClientCertificateParams.certificatePemSize = 0U;
    command_data_ptr->setClientCertificateParams.privateKeyPemSize = privateKeyLength;
    static_cast<void>(std::memcpy(&command_data_ptr->setClientCertificateParams.certificateData[0],
                                  privateKey, privateKeyLength));

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(command_data_ptr.get()), command_ipc_data_size,
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsSetClientPrivateKey()< %" PRIi32 "\n\n", status)

  return status;
}

std::uint32_t EmwApiEmw::tlsConnect(std::int32_t domain, std::int32_t type, std::int32_t protocol,
                                    const EmwAddress::SockAddrStorage_t &socketAddress, std::int32_t socketAddressSize,
                                    const char (&caString)[2500], std::int32_t caStringLength) noexcept
{
  std::uint32_t tls_key = 0U;
  static_cast<void>(domain);
  static_cast<void>(type);
  static_cast<void>(protocol);

  if (0 < socketAddressSize) {
    const char (&empty)[128] = {'\0'};
    tls_key = this->tlsConnectSni(empty, 0, socketAddress, socketAddressSize, caString, caStringLength);
  }
  return tls_key;
}

std::uint32_t EmwApiEmw::tlsConnectSni(const char (&serverNameInformationString)[128],
                                       std::int32_t serverNameInformationLength,
                                       const EmwAddress::SockAddrStorage_t &socketAddress, std::int32_t socketAddressSize,
                                       const char (&caString)[2500], std::int32_t caStringLength) noexcept

{
  std::uint32_t tls_key = 0U;
  const std::uint16_t command_ipc_data_size \
    = static_cast<std::uint16_t>(sizeof(EmwCoreIpc::IpcTlsConnectSniParams_t) - 1U + caStringLength);

  DEBUG_API_LOG("\n EmwApiEmw::tlsConnectSni()> %" PRIi32 "\n", caStringLength)

  if ((socketAddressSize <= 0) || (command_ipc_data_size > EmwNetworkStack::NETWORK_BUFFER_SIZE)) {
    tls_key = 0U;
  }
  else {
    EmwCoreIpc::TlsConnectSniResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    std::unique_ptr<EmwCoreIpc::IpcTlsConnectSniParams_t, decltype(&EmwOsInterface::Free)> \
    command_data_ptr(static_cast<EmwCoreIpc::IpcTlsConnectSniParams_t *>\
                     (EmwOsInterface::Malloc(command_ipc_data_size)), &EmwOsInterface::Free);
    const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eTLS_CONNECT_SNI_CMD);
    const EmwAddress::SockAddrStorage_t socket_address_default;

    command_data_ptr->ipcParams = ipc_params;
    command_data_ptr->connectSniParams.addr = socket_address_default;
    (void) std::memcpy(&command_data_ptr->connectSniParams.addr, &socketAddress, socketAddressSize);
    command_data_ptr->connectSniParams.length = socketAddressSize;

    STRING_COPY_TO_ARRAY_CHAR(command_data_ptr->connectSniParams.sniServerName, "");
    command_data_ptr->connectSniParams.caLength = 0;
    STRING_COPY_TO_ARRAY_CHAR(command_data_ptr->connectSniParams.ca, "");

    if (serverNameInformationLength > 0) {
      STRING_COPY_TO_ARRAY_CHAR(command_data_ptr->connectSniParams.sniServerName, serverNameInformationString);
    }
    if (caStringLength > 0) {
      command_data_ptr->connectSniParams.caLength = caStringLength;
      static_cast<void>(std::memcpy(&command_data_ptr->connectSniParams.ca[0], caString, caStringLength));
    }
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(command_data_ptr.get()), command_ipc_data_size,
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0U == response_buffer.tlsKey) {
        tls_key = 0U;
        DEBUG_API_LOG(" EmwApiEmw::tlsConnectSni(): errno: %" PRIi32 "\n\n", response_buffer.emwErrno)
      }
      else {
        tls_key = response_buffer.tlsKey;
        DEBUG_API_LOG(" EmwApiEmw::tlsConnectSni(): tls: %p\n\n", response_buffer.tlsPtr)
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsConnectSni()< 0x%08" PRIx32 "\n\n", tls_key)

  return tls_key;
}

std::int32_t EmwApiEmw::tlsSend(EmwApiBase::Mtls_t tlsKey, const std::uint8_t (&data)[],
                                std::size_t dataLength) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::tlsSend()> tls: 0x%08" PRIx32 "\n", tlsKey)

  if ((0U == tlsKey) || (dataLength == 0)) {
    status = -1;
  }
  else {
    EmwCoreIpc::TlsSendResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);
    std::size_t data_length = dataLength;

    if ((data_length + sizeof(EmwCoreIpc::IpcTlsSendParams_t) - 1U) > EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      data_length = EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE - (sizeof(EmwCoreIpc::IpcTlsSendParams_t) - 1U);
    }

    {
      const std::uint16_t command_data_size \
        = static_cast<std::uint16_t>(sizeof(EmwCoreIpc::IpcTlsSendParams_t) - 1U + data_length);
      const std::uint16_t command_ipc_data_size = sizeof(EmwCoreIpc::CmdParams_t) + command_data_size;
      std::unique_ptr<EmwCoreIpc::IpcTlsSendParams_t, decltype(&EmwOsInterface::Free)> \
      command_data_ptr(static_cast<EmwCoreIpc::IpcTlsSendParams_t *>\
                       (EmwOsInterface::Malloc(command_ipc_data_size)), &EmwOsInterface::Free);
      const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eTLS_SEND_CMD);

      command_data_ptr->ipcParams = ipc_params;
      command_data_ptr->sendParams.tlsKey = tlsKey;
      static_cast<void>(std::memcpy(&command_data_ptr->sendParams.buffer[0], data, data_length));
      command_data_ptr->sendParams.size = data_length;

      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(command_data_ptr.get()), command_ipc_data_size,
          BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
        status = response_buffer.sent;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsSend()< %" PRIi32 "\n\n", status)

  return status;
}

std::int32_t EmwApiEmw::tlsReceive(EmwApiBase::Mtls_t tlsPtr, std::uint8_t (&data)[], std::int32_t dataLength) noexcept
{
  std::int32_t status;

  DEBUG_API_LOG("\n EmwApiEmw::tlsReceive()> tls: 0x%08" PRIx32 "\n", tlsPtr)

  if ((0U == tlsPtr) || (dataLength <= 0)) {
    status = -4;
  }
  else {
    EmwCoreIpc::IpcTlsReceiveParams_t command_data(tlsPtr);
    std::uint16_t response_buffer_size;
    std::size_t data_length = dataLength;

    status = 0;

    if ((data_length + sizeof(EmwCoreIpc::TlsReceiveParams_t) - 1U) > EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE) {
      data_length = EmwNetworkStack::NETWORK_IPC_PAYLOAD_SIZE - (sizeof(EmwCoreIpc::TlsReceiveParams_t) - 1U);
    }

    response_buffer_size = static_cast<std::uint16_t>(sizeof(EmwCoreIpc::TlsReceiveParams_t) - 1U + data_length);
    {
      std::unique_ptr<EmwCoreIpc::TlsReceiveResponseParams_t, decltype(&EmwOsInterface::Free)> \
      response_buffer_ptr(static_cast<EmwCoreIpc::TlsReceiveResponseParams_t *>\
                          (EmwOsInterface::Malloc(response_buffer_size)), &EmwOsInterface::Free);

      response_buffer_ptr->received = 0;
      command_data.receiveParams.size = dataLength;

      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
          BYTES_REF_CAST(response_buffer_ptr.get()), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
        if (response_buffer_ptr->received > 0) {
          const std::size_t received_length = response_buffer_ptr->received;
          if (received_length <= data_length) {
            static_cast<void>(std::memcpy(data, &response_buffer_ptr->buffer[0], received_length));
          }
        }
        status = response_buffer_ptr->received;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsReceive()< %" PRIi32 "\n\n", status)

  return status;
}

std::int32_t EmwApiEmw::tlsClose(EmwApiBase::Mtls_t tlsPtr) noexcept
{
  std::int32_t status = -1;

  DEBUG_API_LOG("\n EmwApiEmw::tlsClose()> tls: 0x%08" PRIx32 "\n", tlsPtr)

  if (0U == tlsPtr) {
    status = -4;
  }
  else {
    EmwCoreIpc::IpcTlsCloseParams_t command_data(tlsPtr);
    EmwCoreIpc::TlsCloseResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsClose()< %" PRIi32 "\n\n", status)

  return status;
}

std::int32_t EmwApiEmw::tlsSetNonBlocking(EmwApiBase::Mtls_t tlsPtr, std::int32_t nonblock) noexcept
{
  std::int32_t status = -1;

  DEBUG_API_LOG("\n EmwApiEmw::tlsSetNonBlocking()> tls: 0x%08" PRIx32 "\n", tlsPtr)

  if (0U == tlsPtr) {
    status = -4;
  }
  else {
    EmwCoreIpc::IpcTlsSetNonblockParams_t command_data(tlsPtr, nonblock);
    EmwCoreIpc::TlsSetNonblockResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::tlsSetNonBlocking()< %" PRIi32 "\n\n", status)

  return status;
}

std::int32_t EmwApiEmw::doSocketPing(std::uint16_t apiId,
                                     const char *hostnameStringPtr,
                                     std::int32_t count, std::int32_t delayInMs, std::int32_t (&responses)[10]) noexcept
{
  std::int32_t status = -4;

  DEBUG_API_LOG("\n EmwApiEmw::doSocketPing()>\n")

  if (0 < count) {
    EmwCoreIpc::IpcWiFiPingParams_t command_data(apiId);
    EmwCoreIpc::WiFiPingResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(EmwCoreIpc::WiFiPingResponseParams_t);
    const std::int32_t count_max = sizeof(response_buffer.delaysInMs) / sizeof(response_buffer.delaysInMs[0]);

    status = -1;
    STRING_COPY_TO_ARRAY_CHAR(command_data.pingParams.hostname, hostnameStringPtr);
    command_data.pingParams.count = (count <= count_max) ? count : count_max;
    command_data.pingParams.delayInMs = delayInMs;
    response_buffer.numberOf = 0;
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this, BYTES_REF_CAST(&command_data), sizeof(command_data),
        BYTES_REF_CAST(&response_buffer), response_buffer_size, EmwCoreIpc::EMW_CMD_TIMEOUT)) {
      if (response_buffer.numberOf > 0) {
        for (int32_t i = 0; i < response_buffer.numberOf; i++) {
          responses[i] = response_buffer.delaysInMs[i];
        }
        status = 0;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiEmw::doSocketPing()< %" PRIi32 "\n", status)
  return status;
}

EmwAddress::SockAddrIn_t EmwApiEmw::socketAddressIn_FromPacked(const EmwAddress::SockAddrStorage_t &socketAddress)
noexcept
{
  EmwAddress::SockAddrIn_t sock_address_in;

  sock_address_in.length = socketAddress.length;
  sock_address_in.family = socketAddress.family;
  {
    std::uint16_t port_in = socketAddress.data1[0];
    port_in |= socketAddress.data1[1] << 8;
    sock_address_in.port = port_in;
  }
  sock_address_in.inAddr.addr = socketAddress.data2[0];
  return sock_address_in;
}

EmwAddress::SockAddrIn6_t EmwApiEmw::socketAddressIn6_FromPacked(const EmwAddress::SockAddrStorage_t &socketAddress)
noexcept
{
  EmwAddress::SockAddrIn6_t sock_address_in6;

  sock_address_in6.length = socketAddress.length;
  sock_address_in6.family = socketAddress.family;
  {
    std::uint16_t port_in = socketAddress.data1[0];
    port_in |= socketAddress.data1[1] << 8;
    sock_address_in6.port = port_in;
  }
  sock_address_in6.flowInfo = socketAddress.data2[0];
  sock_address_in6.in6Addr.un.u32Addr[0] = socketAddress.data2[1];
  sock_address_in6.in6Addr.un.u32Addr[1] = socketAddress.data2[2];
  sock_address_in6.in6Addr.un.u32Addr[2] = socketAddress.data3[0];
  sock_address_in6.in6Addr.un.u32Addr[3] = socketAddress.data3[1];
  sock_address_in6.scopeId = socketAddress.data3[2];
  return sock_address_in6;
}

EmwAddress::SockAddrStorage_t EmwApiEmw::socketAddressIn_ToPacked(const EmwAddress::SockAddr_t &socketAddress) noexcept
{
  EmwAddress::SockAddrStorage_t sock_address_storage;
  const EmwAddress::SockAddrIn_t *const sock_address_in_ptr \
    = reinterpret_cast<const EmwAddress::SockAddrIn_t *>(&socketAddress);

  sock_address_storage.length = sock_address_in_ptr->length;
  sock_address_storage.family = sock_address_in_ptr->family;
  sock_address_storage.data1[0] = static_cast<std::uint8_t>(sock_address_in_ptr->port);
  sock_address_storage.data1[1] = static_cast<std::uint8_t>(sock_address_in_ptr->port >> 8);
  sock_address_storage.data2[0] = sock_address_in_ptr->inAddr.addr;
  return sock_address_storage;
}

EmwAddress::SockAddrStorage_t EmwApiEmw::socketAddressIn6_ToPacked(const EmwAddress::SockAddr_t &socketAddress) noexcept
{
  EmwAddress::SockAddrStorage_t sock_address_storage;
  const EmwAddress::SockAddrIn6_t *const sock_address_in6_ptr \
    = reinterpret_cast<const EmwAddress::SockAddrIn6_t *>(&socketAddress);

  sock_address_storage.length = sock_address_in6_ptr->length;
  sock_address_storage.family = sock_address_in6_ptr->family;
  sock_address_storage.data1[0] = static_cast<std::uint8_t>(sock_address_in6_ptr->port);
  sock_address_storage.data1[1] = static_cast<std::uint8_t>(sock_address_in6_ptr->port >> 8);
  sock_address_storage.data2[0] = sock_address_in6_ptr->flowInfo;
  sock_address_storage.data2[1] = sock_address_in6_ptr->in6Addr.un.u32Addr[0];
  sock_address_storage.data2[2] = sock_address_in6_ptr->in6Addr.un.u32Addr[1];
  sock_address_storage.data3[0] = sock_address_in6_ptr->in6Addr.un.u32Addr[2];
  sock_address_storage.data3[1] = sock_address_in6_ptr->in6Addr.un.u32Addr[3];
  sock_address_storage.data3[2] = sock_address_in6_ptr->scopeId;
  return sock_address_storage;
}

template<typename T> void copyStringToArray(T destination[], std::size_t destinationCount,
    const char *sourceStringPtr)
{
  const std::size_t index_max = destinationCount;
  const char *source_char_ptr = sourceStringPtr;
  std::size_t index;

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
