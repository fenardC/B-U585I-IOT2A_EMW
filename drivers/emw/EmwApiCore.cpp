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
#include "EmwAddress.hpp"
#include "EmwCoreIpc.hpp"
#include "EmwOsInterface.hpp"
#include "EmwNetworkStack.hpp"
#include <cinttypes>
#include <cstring>
#include <memory>

#if !defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)
#endif /* EMW_API_DEBUG */

template<typename T> void copyStringToArray(T destination[], std::size_t destinationCount,
    const char *sourceStringPtr);

#define STRING_COPY_TO_ARRAY_CHAR(DEST, SRC)                          \
  do {                                                                \
    copyStringToArray<char>((DEST), sizeof((DEST)), (SRC));           \
  } while(false)

#define STRING_COPY_TO_ARRAY_INT8(DEST, SRC)                          \
  do {                                                                \
    copyStringToArray<std::int8_t>((DEST), sizeof((DEST)), (SRC));    \
  } while(false)

#define STRING_COPY_TO_ARRAY_UINT8(DEST, SRC)                         \
  do {                                                                \
    copyStringToArray<std::uint8_t>((DEST), sizeof((DEST)), (SRC));   \
  } while(false)

#define VOID_MEMSET_ARRAY(DEST, VAL)  (void) std::memset(static_cast<void *>(DEST), (VAL), sizeof((DEST)))

EMW_STATS_DECLARE()

static EmwOsInterface::Mutex_t DeviceAvailableLock;

EmwApiCore::EmwApiCore() noexcept
  : systemInformations()
  , stationSettings()
  , softAccessPointSettings()
  , runtime()
{
  DEBUG_API_LOG("\n EmwApiCore::EmwApiCore()>\n")
  DEBUG_API_LOG("\n EmwApiCore::EmwApiCore()< %p\n\n", static_cast<const void*>(this))
}

EmwApiCore::~EmwApiCore(void) noexcept
{
  DEBUG_API_LOG("\n EmwApiCore::~EmwApiCore()>\n")
  DEBUG_API_LOG("\n EmwApiCore::~EmwApiCore()<%p\n\n", static_cast<const void*>(this))
}

EmwApiBase::Status EmwApiCore::checkNotified(std::uint32_t timeoutInMs) const noexcept
{
  DEBUG_API_LOG(" EmwApiCore::checkNotified()>\n")

  if (0U < this->runtime.interfaces) {
    EmwCoreIpc::Poll(nullptr, this, timeoutInMs);
  }
  DEBUG_API_LOG(" EmwApiCore::checkNotified()<\n")

  return EmwApiBase::eEMW_STATUS_OK;
}

EmwApiBase::Status EmwApiCore::connect(const char (&ssidString)[33], const char (&passwordString)[65],
                                       EmwApiBase::SecurityType securityType) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  const std::size_t ssid_string_length = std::strlen(ssidString);
  const std::size_t password_string_length = std::strlen(passwordString);
  const std::size_t ssid_length_max \
    = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.ssid) - 1;
  const std::size_t password_length_max \
    = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.key) - 1;

  static_cast<void>(securityType);

  DEBUG_API_LOG("\n EmwApiCore::connect()>\n");

  if ((ssid_length_max < ssid_string_length) || (password_length_max < password_string_length)) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    EmwCoreIpc::IpcWiFiConnectParams_t command_data;
    EmwCoreIpc::SysCommonResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ssid, ssidString);
    STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.key, passwordString);
    command_data.connectParams.keyLength = static_cast<std::int32_t>(password_string_length);

    if (!this->stationSettings.dhcpIsEnabled) {
      command_data.connectParams.useIp = 1U;
      {
        EmwAddress::IpAddr_t ip_address;
        (void) memcpy(&ip_address, this->stationSettings.ipAddress, sizeof(ip_address));
        {
          char ip_addr_string[] = {"000.000.000.000"};
          EmwAddress::NetworkToAscii(ip_address, ip_addr_string, sizeof(ip_addr_string));
          STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.ipAddressLocal, ip_addr_string);
        }
      }
      {
        EmwAddress::IpAddr_t ip_mask;
        (void) memcpy(&ip_mask, this->stationSettings.ipMask, sizeof(ip_mask));
        {
          char ip_mask_string[] = {"000.000.000.000"};
          EmwAddress::NetworkToAscii(ip_mask, ip_mask_string, sizeof(ip_mask_string));
          STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.networkMask, ip_mask_string);
        }
      }
      {
        EmwAddress::IpAddr_t gateway_ip_address;
        (void) memcpy(&gateway_ip_address, this->stationSettings.gatewayAddress, sizeof(gateway_ip_address));
        {
          char gateway_ip_addrstring[] = {"000.000.000.000"};
          EmwAddress::NetworkToAscii(gateway_ip_address, gateway_ip_addrstring, sizeof(gateway_ip_addrstring));
          STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.gatewayAddress, gateway_ip_addrstring);
        }
      }
      {
        EmwAddress::IpAddr_t dns_ip_address;
        (void) memcpy(&dns_ip_address, this->stationSettings.dns1, sizeof(dns_ip_address));
        {
          char dns_ip_addr_string[] = {"000.000.000.000"};
          EmwAddress::NetworkToAscii(dns_ip_address, dns_ip_addr_string, sizeof(dns_ip_addr_string));
          STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.dnsServerAddress, dns_ip_addr_string);
        }
      }
    }
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&response_buffer)), response_buffer_size,
        EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
    DEBUG_API_LOG(" EmwApiCore::connect()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
  }
  return status;
}

EmwApiBase::Status EmwApiCore::connectAdvance(const char (&ssidString)[33], const char (&passwordString)[65],
    const EmwApiBase::ConnectAttributes_t &attributes,
    const EmwApiBase::IpAttributes_t &ipAttributes) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  const std::size_t ssid_string_length = std::strlen(ssidString);
  const std::size_t password_string_length = std::strlen(passwordString);
  const std::size_t ssid_length_max \
    = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.ssid) - 1;
  const std::size_t password_length_max \
    = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.key) - 1;

  DEBUG_API_LOG("\n EmwApiCore::connectAdvance()>\n")

  if ((ssid_length_max < ssid_string_length) || (password_length_max < password_string_length)) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    EmwCoreIpc::IpcWiFiConnectParams_t command_data;
    EmwCoreIpc::SysCommonResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ssid, ssidString);
    STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.key, passwordString);
    command_data.connectParams.keyLength = static_cast<std::int32_t>(password_string_length);

    if (0 != attributes.bssid[0]) {
      command_data.connectParams.useAttribute = 1U;
      (void) memcpy(&command_data.connectParams.attr.bssid, &attributes.bssid,
                    sizeof(command_data.connectParams.attr.bssid));
      command_data.connectParams.attr.channel = attributes.channel;
      command_data.connectParams.attr.security = attributes.security;
    }
    if (0 != ipAttributes.ipAddressLocal[0]) {
      command_data.connectParams.useIp = 1U;
      command_data.connectParams.ip = ipAttributes;
    }
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&response_buffer)), response_buffer_size,
        EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
    DEBUG_API_LOG(" EmwApiCore::connectAdvance()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
  }
  return status;
}

EmwApiBase::Status EmwApiCore::connectEAP(const char (&ssidString)[33], const char (&identityString)[33],
    const char (&passwordString)[65], const EmwApiBase::EapAttributes_t &eapAttributes,
    const EmwApiBase::IpAttributes_t &ipAttributes) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  const std::size_t ssid_string_length = std::strlen(ssidString);
  const std::size_t identity_string_length = std::strlen(identityString);
  const std::size_t password_string_length = std::strlen(passwordString);
  const std::size_t ssid_length_max \
    = sizeof((static_cast<EmwCoreIpc::WiFiEapConnectParams_t *>(nullptr)) ->ssid) - 1;
  const std::size_t identity_length_max \
    = sizeof((static_cast<EmwCoreIpc::WiFiEapConnectParams_t *>(nullptr)) ->identity) - 1;
  const std::size_t password_length_max \
    = sizeof((static_cast<EmwCoreIpc::WiFiEapConnectParams_t *>(nullptr)) ->password) - 1;

  DEBUG_API_LOG("\n EmwApiCore::connectEAP()>\n")

  if ((ssid_length_max < ssid_string_length) || (identity_length_max < identity_string_length) \
      || (password_length_max < password_string_length)) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    EmwCoreIpc::IpcWiFiEapConnectParams_t command_data;

    status = EmwApiBase::eEMW_STATUS_OK;
    STRING_COPY_TO_ARRAY_CHAR(command_data.eapConnectParams.ssid, ssidString);
    STRING_COPY_TO_ARRAY_CHAR(command_data.eapConnectParams.identity, identityString);
    STRING_COPY_TO_ARRAY_CHAR(command_data.eapConnectParams.password, passwordString);

    if (0 != eapAttributes.eapType) {
      if ((EmwApiBase::eEAP_TYPE_TLS != eapAttributes.eapType) \
          && (EmwApiBase::eEAP_TYPE_TTLS != eapAttributes.eapType) \
          && (EmwApiBase::eEAP_TYPE_PEAP != eapAttributes.eapType)) {
        status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
      }
      else {
        command_data.eapConnectParams.attr.eapType = eapAttributes.eapType;
        if (nullptr != eapAttributes.rootCaPtr) {
          status = this->setEapCert(EmwCoreIpc::EAP_ROOTCA,
                                    eapAttributes.rootCaPtr, std::strlen(eapAttributes.rootCaPtr));

          if (EmwApiBase::eEMW_STATUS_OK == status) {
            command_data.eapConnectParams.attrUsed = 1U;
          }
        }
        if ((EmwApiBase::eEMW_STATUS_OK == status) && (nullptr != eapAttributes.clientCertificatePtr)) {
          status = this->setEapCert(EmwCoreIpc::EAP_CLIENT_CERT,
                                    eapAttributes.clientCertificatePtr, std::strlen(eapAttributes.clientCertificatePtr));

          if (EmwApiBase::eEMW_STATUS_OK == status) {
            command_data.eapConnectParams.attrUsed = 1U;
          }
        }
        if ((EmwApiBase::eEMW_STATUS_OK == status) && (nullptr != eapAttributes.clientKeyPtr)) {
          status = this->setEapCert(EmwCoreIpc::EAP_CLIENT_KEY,
                                    eapAttributes.clientKeyPtr, std::strlen(eapAttributes.clientKeyPtr));

          if (EmwApiBase::eEMW_STATUS_OK == status) {
            command_data.eapConnectParams.attrUsed = 1U;
          }
        }
      }
    }
    else {
      command_data.eapConnectParams.attr.eapType = EmwApiBase::eEAP_TYPE_PEAP;
    }
    if (EmwApiBase::eEMW_STATUS_OK == status) {
      EmwCoreIpc::SysCommonResponseParams_t response_buffer;
      std::uint16_t response_buffer_size = sizeof(response_buffer);

      if (0 != ipAttributes.ipAddressLocal[0]) {
        command_data.eapConnectParams.ip = ipAttributes;
        command_data.eapConnectParams.ipUsed = 1U;
      }
      status = EmwApiBase::eEMW_STATUS_ERROR;
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
          reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
          reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&response_buffer)), response_buffer_size,
          EMW_CMD_TIMEOUT)) {
        if (0 == response_buffer.status) {
          status = EmwApiBase::eEMW_STATUS_OK;
        }
      }
      DEBUG_API_LOG(" EmwApiCore::connectEAP()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
    }
  }
  return status;
}

EmwApiBase::Status EmwApiCore::connectWPS(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_WPS_CONNECT_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n EmwApiCore::connectWPS()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&response_buffer)), response_buffer_size,
      15000U)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::connectWPS()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
  return status;
}

EmwApiBase::Status EmwApiCore::disconnect(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_DISCONNECT_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer.status);

  DEBUG_API_LOG("\n EmwApiCore::disconnect()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&response_buffer)), response_buffer_size,
      15000U)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::disconnect()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))

  return status;
}

const char *EmwApiCore::getConfigurationString(void) const noexcept
{
  static const char configuration_string[] \
    = { EMW_IO_NAME_STRING ", " RTOS_NAME_STRING ", " NETWORK_NAME_STRING };
  return configuration_string;
}

void EmwApiCore::getStatistics(void) const noexcept
{
  EMW_STATS_LOG()
}

EmwApiBase::Status EmwApiCore::getIPAddress(std::uint8_t (&ipAddressBytes)[4],
    EmwApiBase::EmwInterface interface) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::WiFiGetIpResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);
  EmwCoreIpc::IpcInterfaceParams_t command_data(this->toIpcInterface(interface));

  DEBUG_API_LOG("\n EmwApiCore::getIPAddress()>\n");

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&response_buffer)), response_buffer_size,
      EmwApiCore::GET_IP_ADDRESS_TIMEOUT)) {
    if (0 == response_buffer.status) {
      {
        EmwAddress::IpAddr_t ip;
        EmwAddress::AsciiToNetwork(&response_buffer.ip.ipAddressLocal[0], ip);
        (void) std::memcpy(&this->stationSettings.ipAddress[0], &ip.addr, sizeof(this->stationSettings.ipAddress));
      }
      {
        EmwAddress::IpAddr_t netmask;
        EmwAddress::AsciiToNetwork(&response_buffer.ip.networkMask[0], netmask);
        (void) std::memcpy(&this->stationSettings.ipMask[0], &netmask.addr, sizeof(this->stationSettings.ipMask));
      }
      {
        EmwAddress::IpAddr_t gw;
        EmwAddress::AsciiToNetwork(&response_buffer.ip.gatewayAddress[0], gw);
        (void) std::memcpy(&this->stationSettings.gatewayAddress[0], &gw.addr, sizeof(this->stationSettings.gatewayAddress));
      }
      {
        EmwAddress::IpAddr_t dns;
        EmwAddress::AsciiToNetwork(&response_buffer.ip.dnsServerAddress[0], dns);
        (void) std::memcpy(&this->stationSettings.dns1[0], &dns.addr, sizeof(this->stationSettings.dns1));
      }
      (void) std::memcpy(ipAddressBytes, this->stationSettings.ipAddress, sizeof(this->stationSettings.ipAddress));
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::getIPAddress< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
  return status;
}

EmwApiBase::Status EmwApiCore::getIP6Address(std::uint8_t (&ip6AddressBytes)[16], std::uint8_t addressSlot,
    EmwApiBase::EmwInterface interface) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  DEBUG_API_LOG("\n EmwApiCore::getIP6Address()>\n")

  if (addressSlot < 3U) {
    EmwCoreIpc::IpcWiFiGetIp6AddrParams_t command_data(addressSlot, this->toIpcInterface(interface));
    EmwCoreIpc::WiFiGetIp6AddrResponseParams_t response_buffer;
    std::uint16_t response_buffer_size = sizeof(response_buffer);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
        EmwApiCore::GET_IP_ADDRESS_TIMEOUT)) {
      if (0 == response_buffer.status) {
        (void) std::memcpy(this->stationSettings.ipv6Address[addressSlot], response_buffer.ip6,
                           sizeof(this->stationSettings.ipv6Address[addressSlot])); /* size of an array of 16 bytes. */
        (void) std::memcpy(ip6AddressBytes, response_buffer.ip6, sizeof(response_buffer.ip6));
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
    DEBUG_API_LOG(" EmwApiCore::getIP6Address()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
  }
  return status;
}

int32_t EmwApiCore::getIP6AddressState(std::uint8_t addressSlot, EmwApiBase::EmwInterface interface) const noexcept
{
  std::int32_t state = -1;
  EmwCoreIpc::IpcAddressSlotInterfaceParams_t command_data;
  EmwCoreIpc::WiFiGetIp6StateResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n EmwApiCore::getIP6AddressState()>\n")

  command_data.addressSlotInterfaceNum.addressSlot = addressSlot;
  command_data.addressSlotInterfaceNum.interfaceNum = this->toIpcInterface(interface);
  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    state = static_cast<std::int32_t>(response_buffer.state);
  }
  DEBUG_API_LOG(" EmwApiCore::getIP6AddressState()< %" PRIi32 "\n\n", state)
  return state;
}

EmwApiBase::Status EmwApiCore::getStationMacAddress(EmwApiCore::MacAddress_t &mac) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  DEBUG_API_LOG("\n EmwApiCore::getStationMacAddress()>\n")

  if (0U < this->runtime.interfaces) {
    (void) std::memcpy(mac.bytes, &this->systemInformations.mac48bitsStation[0], sizeof(mac.bytes));
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG(" EmwApiCore::getStationMacAddress()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::getSoftApMacAddress(EmwApiCore::MacAddress_t &mac) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_GET_SOFT_MAC_CMD);
  std::uint16_t response_buffer_size = sizeof(mac.bytes);

  DEBUG_API_LOG("\nEmwApiCore::getSoftApMacAddress()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(* &this->systemInformations.mac48bitsSoftAp[0]), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    (void) std::memcpy(mac.bytes, &this->systemInformations.mac48bitsSoftAp[0], sizeof(mac.bytes));
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG("EmwApiCore::getSoftApMacAddress()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::getVersion(char (&version)[25], std::uint32_t versionSize) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if ((0U < versionSize) && (25U >= versionSize)) {
    EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_VERSION_CMD);
    std::uint8_t firmware_revision[24];
    std::uint16_t response_buffer_size = sizeof(firmware_revision);

    VOID_MEMSET_ARRAY(firmware_revision, 0);
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
        reinterpret_cast<std::uint8_t (&)[]>(* &firmware_revision[0]), response_buffer_size, EMW_CMD_TIMEOUT)) {
      (void) std::memcpy(version, &firmware_revision[0], versionSize);
      version[versionSize - 1U] = '\0';
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  return status;
}

int8_t EmwApiCore::getScanResults(std::uint8_t (&results)[480], std::uint8_t number) const noexcept
{
  std::uint8_t copy_number = 0U;

  if ((0U != number)) {
    copy_number = (this->runtime.scanResults.count < number) ? this->runtime.scanResults.count : number;
    (void) std::memcpy(results, this->runtime.scanResults.accessPoints,
                       copy_number * sizeof(this->runtime.scanResults.accessPoints[0]));
  }
  DEBUG_API_LOG(" EmwApiCore::getScanResults()< %" PRIi32 "\n\n", static_cast<std::int32_t>(copy_number))
  return static_cast<std::int8_t>(copy_number);
}

EmwApiBase::Status EmwApiCore::initialize(void) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  DEBUG_API_LOG("\n EmwApiCore::initialize()>\n")

  {
    EmwScopedLock lock(DeviceAvailableLock);

    if (0U == this->runtime.interfaces) {
      EmwCoreIpc::Initialize(this);
#if defined(EMW_WITH_RTOS)
      {
        static const char receive_thread_name[] = {"EMW-ReceiveThread"};
        const EmwOsInterface::Status os_status = EmwOsInterface::CreateThread(EmwApiCore::ReceiveThread,
          receive_thread_name,
          EmwApiCore::ReceiveThreadFunction, static_cast<EmwOsInterface::ThreadFunctionArgument_t>(this),
          EMW_RECEIVED_THREAD_STACK_SIZE, EMW_RECEIVED_THREAD_PRIORITY);
        EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
      }
      EmwOsInterface::DelayTicks(1U);
#endif /* EMW_WITH_RTOS */
      this->runtime.interfaces = 1U;
    }
    else {
      this->runtime.interfaces = this->runtime.interfaces + 1;
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  {
    EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_VERSION_CMD);
    std::uint16_t response_buffer_size = sizeof(this->systemInformations.firmwareRevision);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&this->systemInformations.firmwareRevision[0])),
        response_buffer_size, EMW_CMD_TIMEOUT)) {
      {
        const char firmware_revision_required_string[] = "V2.3.4";
        const bool out_of_date = (0 < std::strncmp(firmware_revision_required_string,
                                  static_cast<const char *>(this->systemInformations.firmwareRevision),
                                  sizeof(firmware_revision_required_string)));
        if (out_of_date) {
          DRIVER_ERROR_VERBOSE("ERROR: The Wi-Fi firmware is out of date\n")
        }
        EmwOsInterface::AssertAlways(!out_of_date);
      }
    }
    else {
      this->unInitialize();
    }
  }
  {
    EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_GET_MAC_CMD);
    std::uint16_t response_buffer_size = sizeof(this->systemInformations.mac48bitsStation);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
        reinterpret_cast<uint8_t (&)[]>(* &this->systemInformations.mac48bitsStation[0]), response_buffer_size,
        EMW_CMD_TIMEOUT)) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
    else {
      this->unInitialize();
    }
  }
  DEBUG_API_LOG(" EmwApiCore::initialize()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

std::int8_t EmwApiCore::isConnected(void) noexcept
{
  std::int8_t state = 0;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_GET_LINKINFO_CMD);
  EmwCoreIpc::WiFiGetLinkInfoResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n EmwApiCore::isConnected()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      this->stationSettings.isConnected = static_cast<std::int8_t>(response_buffer.info.isConnected);
      if (0 < this->stationSettings.isConnected) {
        state = 1;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiCore::isConnected()< %" PRIi32 "\n\n", static_cast<std::int32_t>(response_buffer.status))
  return state;
}

EmwApiBase::Status EmwApiCore::registerStatusCallback(const EmwApiBase::WiFiStatusCallback_t statusCallbackFunctionPtr,
    void *argPtr, EmwApiBase::EmwInterface interface) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  DEBUG_API_LOG("\n EmwApiCore::registerStatusCallback()>\n")

  if (0U < this->runtime.interfaces) {
    const uint8_t interface_index = (EmwApiBase::eSTATION == interface) \
                                    ? EmwApiBase::eWIFI_INTERFACE_STATION_IDX \
                                    : EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX;
    this->runtime.wiFiStatusCallbacks[interface_index] = statusCallbackFunctionPtr;
    this->runtime.wiFiStatusCallbackArgPtrs[interface_index] = argPtr;
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG(" EmwApiCore::registerStatusCallback()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::resetHardware(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_OK;

  EmwOsInterface::Lock();

  DEBUG_API_LOG("\n[%6" PRIu32 "] EmwApiCore::resetHardware()>\n", HAL_GetTick())

  if (0U == this->runtime.interfaces) {
    EmwCoreIpc::ResetIo();
  }
  {
    static bool done_once = false;

    if (!done_once) {
      static const char device_available_lock_name[] = {"EMW-DeviceAvailableLock"};
      const EmwOsInterface::Status os_status = EmwOsInterface::CreateMutex(DeviceAvailableLock, device_available_lock_name);
      EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
      done_once = true;
    }
  }
  DEBUG_API_LOG("[%6" PRIu32 "] EmwApiCore::resetHardware()< %" PRIi32 "\n\n", HAL_GetTick(),
                static_cast<std::int32_t>(status))
  EmwOsInterface::UnLock();
  return status;
}

EmwApiBase::Status EmwApiCore::resetModule(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_REBOOT_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  DEBUG_API_LOG("\n[%" PRIu32 "] EmwApiCore::resetModule()>\n", HAL_GetTick())

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG(" EmwApiCore::resetModule()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::resetToFactoryDefault(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_RESET_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG(" EmwApiCore::resetToFactoryDefault()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::scan(EmwApiBase::ScanMode scanMode,
                                    const char (&ssidString)[33], std::int32_t ssidStringLength) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  DEBUG_API_LOG("\n[%" PRIu32 "] EmwApiCore::scan()>\n", HAL_GetTick())

  this->runtime.scanResults.count = 0U;
  VOID_MEMSET_ARRAY(&this->runtime.scanResults.accessPoints, 0);
  if (((EmwApiBase::eSCAN_ACTIVE == scanMode) && ((ssidStringLength <= 0) || (32 < ssidStringLength)))) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    EmwCoreIpc::IpcWiFiScanParams_t command_data;
    std::uint16_t scan_results_size = sizeof(this->runtime.scanResults);

    if (EmwApiBase::eSCAN_ACTIVE == scanMode) {
      STRING_COPY_TO_ARRAY_INT8(command_data.scanParams.ssid, ssidString);
    }
    else {
      STRING_COPY_TO_ARRAY_INT8(command_data.scanParams.ssid, "");
    }
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&this->runtime.scanResults)), scan_results_size,
        EmwApiCore::SCAN_TIMEOUT_IN_MS)) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("[%6" PRIu32 "] EmwApiCore::scan()< %" PRIi32 "\n\n", HAL_GetTick(), static_cast<std::int32_t>(status))

  return status;
}

EmwApiBase::Status EmwApiCore::setTimeout(std::uint32_t timeoutInMs) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (0U < this->runtime.interfaces) {
    this->runtime.timeoutInMs = timeoutInMs;
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  return status;
}

EmwApiBase::Status EmwApiCore::startSoftAp(const EmwApiBase::SoftApSettings_t &accessPointSettings) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcWiFiSoftApStartParams_t command_data;
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  (void) std::memcpy(command_data.softApStartParams.ssid, accessPointSettings.ssidString,
                     sizeof(command_data.softApStartParams.ssid));
  (void) std::memcpy(command_data.softApStartParams.key, accessPointSettings.passwordString,
                     sizeof(command_data.softApStartParams.key));
  command_data.softApStartParams.channel = accessPointSettings.channel;
  STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.ipAddressLocal,
                            accessPointSettings.ip.ipAddressLocal);
  STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.networkMask,
                            accessPointSettings.ip.networkMask);
  STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.gatewayAddress,
                            accessPointSettings.ip.gatewayAddress);
  STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.dnsServerAddress,
                            accessPointSettings.ip.dnsServerAddress);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      3000U)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::startSoftAp()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::stopSoftAp(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_SOFTAP_STOP_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::stopSoftAp()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::stopWPS(void) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_WPS_STOP_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::stopWPS()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::unRegisterStatusCallback(EmwApiBase::EmwInterface interface) noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (0U < this->runtime.interfaces) {
    const std::uint8_t interface_index = (EmwApiBase::eSTATION == interface) \
                                         ? static_cast<std::uint8_t>(EmwApiBase::eWIFI_INTERFACE_STATION_IDX) \
                                         : static_cast<std::uint8_t>(EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX);
    this->runtime.wiFiStatusCallbacks[interface_index] = nullptr;
    this->runtime.wiFiStatusCallbackArgPtrs[interface_index] = nullptr;
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG(" EmwApiCore::unRegisterStatusCallback()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::setEapCert(std::uint8_t certificateType, const char *certificateStringPtr,
    std::uint32_t length) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  static_cast<void>(length);

  if (nullptr != certificateStringPtr) {
    const std::size_t certificate_length = std::strlen(certificateStringPtr);
    const std::uint16_t command_data_size \
      = static_cast<std::uint16_t>(sizeof(EmwCoreIpc::IpcWiFiEapSetCertParams_t) + certificate_length); /* len + 1 */
    std::unique_ptr<EmwCoreIpc::IpcWiFiEapSetCertParams_t, decltype(&EmwOsInterface::Free)> \
    command_data_ptr(static_cast<EmwCoreIpc::IpcWiFiEapSetCertParams_t *> \
                     (EmwOsInterface::Malloc(command_data_size)), &EmwOsInterface::Free);
    const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eWIFI_EAP_SET_CERT_CMD);
    EmwCoreIpc::SysCommonResponseParams_t response_buffer;
    uint16_t response_buffer_size = sizeof(response_buffer);

    command_data_ptr->ipcParams = ipc_params;
    command_data_ptr->eapSetCertParams.type = certificateType;
    command_data_ptr->eapSetCertParams.length = static_cast<std::uint16_t>(certificate_length);
    (void) std::memcpy(command_data_ptr->eapSetCertParams.cert, certificateStringPtr, certificate_length);
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(command_data_ptr.get())), command_data_size,
        reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
        EMW_CMD_TIMEOUT)) {
      if (0 == response_buffer.status) {
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
  }
  DEBUG_API_LOG(" EmwApiCore::setEapCert()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

std::int32_t EmwApiCore::stationPowerSave(std::int32_t onOff) noexcept
{
  std::int32_t status = -1;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_PS_OFF_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  std::uint16_t response_buffer_size = sizeof(response_buffer);

  if (0 != onOff) {
    const EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eWIFI_PS_ON_CMD);
    command_data.ipcParams = ipc_params;
  }
  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::Request(*this,
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t *>(&command_data)), sizeof(command_data),
      reinterpret_cast<std::uint8_t (&)[]>(*reinterpret_cast<std::uint8_t * >(&response_buffer)), response_buffer_size,
      EMW_CMD_TIMEOUT)) {
    if (0 == response_buffer.status) {
      status = 0;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::stationPowerSave()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

EmwApiBase::Status EmwApiCore::testIpcEcho(std::uint8_t (&dataIn)[], std::uint16_t dataInLength,
    std::uint8_t (&dataOut)[], std::uint16_t &dataOutLength, std::uint32_t timeoutInMs) const noexcept
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::TestIpcEcho(*this,
      dataIn, dataInLength, dataOut, dataOutLength, timeoutInMs)) {
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  return status;
}

void EmwApiCore::unInitialize(void) noexcept
{
  EmwScopedLock lock(DeviceAvailableLock);

  DEBUG_API_LOG("\n EmwApiCore::unInitialize()>\n")

  if (1U == this->runtime.interfaces) {
    this->runtime.interfaces = 0U;
#if defined(EMW_WITH_RTOS)
    EmwApiCore::ReceiveThreadQuitFlag = true;
    while (EmwApiCore::ReceiveThreadQuitFlag) {
      EmwOsInterface::Delay(50U);
    }
    EmwOsInterface::TerminateThread(EmwApiCore::ReceiveThread);
#endif /* EMW_WITH_RTOS */
    EmwCoreIpc::UnInitialize();
  }
  else {
    if (0U < this->runtime.interfaces) {
      this->runtime.interfaces = this->runtime.interfaces - 1;
    }
  }
  DEBUG_API_LOG(" EmwApiCore::unInitialize()<\n\n")
  EMW_STATS_LOG()
}

#if defined(EMW_WITH_RTOS)
volatile bool EmwApiCore::ReceiveThreadQuitFlag;
EmwOsInterface::Thread_t EmwApiCore::ReceiveThread;

void EmwApiCore::ReceiveThreadFunction(EmwOsInterface::ThreadFunctionArgument_t argument) noexcept
{
  const class EmwApiCore *const THIS = static_cast<const class EmwApiCore *>(argument);

#if defined(EMW_API_DEBUG)
  std::setbuf(stdout, nullptr);
#endif /* EMW_API_DEBUG */
  DEBUG_API_LOG("\n[%" PRIu32 "] EmwApiCore::ReceiveThreadFunction()>\n", HAL_GetTick())

  EmwApiCore::ReceiveThreadQuitFlag = false;
  while (EmwApiCore::ReceiveThreadQuitFlag != true) {
    EmwCoreIpc::Poll(nullptr, THIS, 500U);
  }
  EmwApiCore::ReceiveThreadQuitFlag = false;
  EmwOsInterface::ExitThread();
}
#endif /* EMW_WITH_RTOS */

std::uint8_t EmwApiCore::toIpcInterface(EmwApiBase::EmwInterface interface) const noexcept
{
  const std::uint8_t interface_num = (EmwApiBase::eSOFTAP == interface) ? 0U : 1U;
  return interface_num;
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
