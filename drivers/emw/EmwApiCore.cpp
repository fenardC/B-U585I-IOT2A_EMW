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
#include "EmwIoInterface.hpp"
#if defined (EMW_USE_SPI_DMA)
#include "EmwIoSpi.hpp"
#endif /* EMW_USE_SPI_DMA */
#include "EmwOsInterface.hpp"
#include "EmwNetworkStack.hpp"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#if !defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)
#endif /* EMW_API_DEBUG */

template<typename T> void copyStringToArray(T destination[], size_t destinationCount,
    const char *sourceStringPtr);

#define STRING_COPY_TO_ARRAY_CHAR(DEST, SRC)                          \
  do {                                                                \
    copyStringToArray<char>((DEST), sizeof((DEST)), (SRC));           \
  } while(false)

#define STRING_COPY_TO_ARRAY_INT8(DEST, SRC)                          \
  do {                                                                \
    copyStringToArray<int8_t>((DEST), sizeof((DEST)), (SRC));         \
  } while(false)

#define STRING_COPY_TO_ARRAY_UINT8(DEST, SRC)                         \
  do {                                                                \
    copyStringToArray<uint8_t>((DEST), sizeof((DEST)), (SRC));        \
  } while(false)

#define VOID_MEMSET_ARRAY(DEST, VAL)  (void) memset(static_cast<void *>(DEST), (VAL), sizeof((DEST)))

EMW_STATS_DECLARE()

static EmwOsInterface::mutex_t DeviceAvailableLock;

EmwApiCore::EmwApiCore()
  : systemInformations()
  , stationSettings()
  , accessPointSettings()
  , runtime()
  , ioPtr(nullptr)
  , ioSPI()
{
  DEBUG_API_LOG("\nEmwApiCore::EmwApiCore()>\n")

#if defined (EMW_USE_SPI_DMA)
  this->ioPtr = (class EmwIoInterface *)&this->ioSPI;
#endif /* EMW_USE_SPI_DMA */

  DEBUG_API_LOG("EmwApiCore::EmwApiCore()<\n\n")
}

EmwApiCore::EmwApiCore(const class EmwApiCore& other)
{
  static_cast<void>(other);
  DRIVER_ERROR_VERBOSE("ERROR: The Wi-Fi driver cannot be construct by copy.\n")
  EmwOsInterface::assertAlways(false);
}

EmwApiCore::~EmwApiCore(void)
{
  DEBUG_API_LOG("\nEmwApiCore::~EmwApiCore()>\n")
  DEBUG_API_LOG("EmwApiCore::~EmwApiCore()<\n\n")
}

EmwApiBase::Status EmwApiCore::checkNotified(uint32_t timeoutInMs) const
{
  DEBUG_API_LOG("\nEmwApiCore::checkNotified()>\n")
  if (0U < this->runtime.interfaces) {
    EmwCoreIpc::poll(nullptr, this, timeoutInMs);
  }
  DEBUG_API_LOG("EmwApiCore::checkNotified()<\n\n");
  return EmwApiBase::eEMW_STATUS_OK;
}

EmwApiBase::Status EmwApiCore::connect(const char *ssidStringPtr, const char *passwordStringPtr,
                                       EmwApiBase::SecurityType securityType) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::connect()>\n");

  static_cast<void>(securityType);

  if ((nullptr != ssidStringPtr) && (nullptr != passwordStringPtr)) {
    const size_t ssid_length = strlen(ssidStringPtr);
    const size_t password_length = strlen(passwordStringPtr);
    const size_t ssid_length_max \
      = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.ssid) - 1;
    const size_t password_length_max \
      = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.key) - 1;

    if ((ssid_length_max < ssid_length) || (password_length_max < password_length)) {
      status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
    }
    else {
      EmwCoreIpc::IpcWiFiConnectParams_t command_data;
      EmwCoreIpc::SysCommonResponseParams_t response_buffer;
      uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

      STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ssid, ssidStringPtr);
      STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.key, passwordStringPtr);
      command_data.connectParams.keyLength = (int32_t)password_length;

      if (!this->stationSettings.dhcpIsEnabled) {
        command_data.connectParams.useIp = 1U;
        {
          EmwAddress::IpAddr_t ip_addr;
          (void) memcpy(&ip_addr, this->stationSettings.ipAddress, sizeof(ip_addr));
          {
            char ip_addr_string[] = {"000.000.000.000"};
            EmwAddress::networkToAscii(ip_addr, ip_addr_string, sizeof(ip_addr_string));
            STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.ipAddressLocal, ip_addr_string);
          }
        }
        {
          EmwAddress::IpAddr_t ip_mask;
          (void) memcpy(&ip_mask, this->stationSettings.ipMask, sizeof(ip_mask));
          {
            char ip_mask_string[] = {"000.000.000.000"};
            EmwAddress::networkToAscii(ip_mask, ip_mask_string, sizeof(ip_mask_string));
            STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.networkMask, ip_mask_string);
          }
        }
        {
          EmwAddress::IpAddr_t gateway_ip_addr;
          (void) memcpy(&gateway_ip_addr, this->stationSettings.gatewayAddress, sizeof(gateway_ip_addr));
          {
            char gateway_ip_addrstring[] = {"000.000.000.000"};
            EmwAddress::networkToAscii(gateway_ip_addr, gateway_ip_addrstring, sizeof(gateway_ip_addrstring));
            STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.gatewayAddress, gateway_ip_addrstring);
          }
        }
        {
          EmwAddress::IpAddr_t dns_ip_addr;
          (void) memcpy(&dns_ip_addr, this->stationSettings.dns1, sizeof(dns_ip_addr));
          {
            char dns_ip_addr_string[] = {"000.000.000.000"};
            EmwAddress::networkToAscii(dns_ip_addr, dns_ip_addr_string, sizeof(dns_ip_addr_string));
            STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ip.dnsServerAddress, dns_ip_addr_string);
          }
        }
      }
      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(&command_data.connectParams),
          (uint16_t)sizeof(command_data.connectParams),
          reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
          status = EmwApiBase::eEMW_STATUS_OK;
        }
      }
      DEBUG_API_LOG("EmwApiCore::connect()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
    }
  }
  return status;
}

EmwApiBase::Status EmwApiCore::connectAdvance(const char *ssidStringPtr, const char *passwordStringPtr,
    const EmwApiBase::ConnectAttributes_t *attributesPtr,
    const EmwApiBase::IpAttributes_t *ipAttributesPtr)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::connectAdvance()>\n")

  if ((nullptr != ssidStringPtr) && (nullptr != passwordStringPtr)) {
    const size_t ssid_length = strlen(ssidStringPtr);
    const size_t password_length = strlen(passwordStringPtr);
    const size_t ssid_length_max \
      = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.ssid) - 1;
    const size_t password_length_max \
      = sizeof((static_cast<EmwCoreIpc::IpcWiFiConnectParams_t *>(nullptr)) ->connectParams.key) - 1;

    if ((ssid_length_max < ssid_length) || (password_length_max < password_length)) {
      status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
    }
    else {
      EmwCoreIpc::IpcWiFiConnectParams_t command_data;
      EmwCoreIpc::SysCommonResponseParams_t response_buffer;
      uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

      STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.ssid, ssidStringPtr);
      STRING_COPY_TO_ARRAY_CHAR(command_data.connectParams.key, passwordStringPtr);
      command_data.connectParams.keyLength = (int32_t)password_length;

      if (nullptr != attributesPtr) {
        command_data.connectParams.useAttribute = 1U;
        (void) memcpy(&command_data.connectParams.attr.bssid, &attributesPtr->bssid,
                      sizeof(command_data.connectParams.attr.bssid));
        command_data.connectParams.attr.channel = attributesPtr->channel;
        command_data.connectParams.attr.security = attributesPtr->security;
      }
      if (nullptr != ipAttributesPtr) {
        command_data.connectParams.useIp = 1U;
        command_data.connectParams.ip = *ipAttributesPtr;
      }

      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t * >(&command_data.connectParams),
          (uint16_t)sizeof(command_data.connectParams),
          reinterpret_cast<uint8_t * >(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
          status = EmwApiBase::eEMW_STATUS_OK;
        }
      }
      DEBUG_API_LOG("EmwApiCore::connectAdvance()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
    }
  }
  return status;
}

EmwApiBase::Status EmwApiCore::connectEAP(const char *ssidStringPtr, const char *identityStringPtr,
    const char *passwordStringPtr, const EmwApiBase::EapAttributes_t *eapAttributesPtr,
    const EmwApiBase::IpAttributes_t *ipAttributesPtr) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::connectEAP()>\n")

  if ((nullptr != ssidStringPtr) && (nullptr != identityStringPtr) && (nullptr != passwordStringPtr)) {
    const size_t ssid_length = strlen(ssidStringPtr);
    const size_t identity_length = strlen(identityStringPtr);
    const size_t password_length = strlen(passwordStringPtr);
    const size_t ssid_length_max \
      = sizeof((static_cast<EmwCoreIpc::WiFiEapConnectParams_t *>(nullptr)) ->ssid) - 1;
    const size_t identity_length_max \
      = sizeof((static_cast<EmwCoreIpc::WiFiEapConnectParams_t *>(nullptr)) ->identity) - 1;
    const size_t password_length_max \
      = sizeof((static_cast<EmwCoreIpc::WiFiEapConnectParams_t *>(nullptr)) ->password) - 1;

    if ((ssid_length_max < ssid_length) || (identity_length_max < identity_length) \
        || (password_length_max < password_length)) {
      status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
    }
    else {
      EmwCoreIpc::IpcWiFiEapConnectParams_t command_data;

      status = EmwApiBase::eEMW_STATUS_OK;

      STRING_COPY_TO_ARRAY_CHAR(command_data.eapConnectParams.ssid, ssidStringPtr);
      STRING_COPY_TO_ARRAY_CHAR(command_data.eapConnectParams.identity, identityStringPtr);
      STRING_COPY_TO_ARRAY_CHAR(command_data.eapConnectParams.password, passwordStringPtr);

      if (nullptr != eapAttributesPtr) {
        if (((uint8_t)EmwApiBase::eEAP_TYPE_TLS != eapAttributesPtr->eapType) \
            && ((uint8_t)EmwApiBase::eEAP_TYPE_TTLS != eapAttributesPtr->eapType) \
            && ((uint8_t)EmwApiBase::eEAP_TYPE_PEAP != eapAttributesPtr->eapType)) {
          status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
        }
        else {
          command_data.eapConnectParams.attr.eapType = eapAttributesPtr->eapType;

          if (nullptr != eapAttributesPtr->rootCaPtr) {
            status = this->setEapCert((uint8_t)EmwCoreIpc::EAP_ROOTCA,
                                      eapAttributesPtr->rootCaPtr, strlen(eapAttributesPtr->rootCaPtr));

            if (EmwApiBase::eEMW_STATUS_OK == status) {
              command_data.eapConnectParams.attrUsed = 1U;
            }
          }
          if ((EmwApiBase::eEMW_STATUS_OK == status) && (nullptr != eapAttributesPtr->clientCertificatePtr)) {
            status = this->setEapCert((uint8_t)EmwCoreIpc::EAP_CLIENT_CERT,
                                      eapAttributesPtr->clientCertificatePtr, strlen(eapAttributesPtr->clientCertificatePtr));

            if (EmwApiBase::eEMW_STATUS_OK == status) {
              command_data.eapConnectParams.attrUsed = 1U;
            }
          }
          if ((EmwApiBase::eEMW_STATUS_OK == status) && (nullptr != eapAttributesPtr->clientKeyPtr)) {
            status = this->setEapCert((uint8_t)EmwCoreIpc::EAP_CLIENT_KEY,
                                      eapAttributesPtr->clientKeyPtr, strlen(eapAttributesPtr->clientKeyPtr));

            if (EmwApiBase::eEMW_STATUS_OK == status) {
              command_data.eapConnectParams.attrUsed = 1U;
            }
          }
        }
      }
      else {
        command_data.eapConnectParams.attr.eapType = (uint8_t) EmwApiBase::eEAP_TYPE_PEAP;
      }

      if (EmwApiBase::eEMW_STATUS_OK == status) {
        EmwCoreIpc::SysCommonResponseParams_t response_buffer;
        uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

        if (nullptr != ipAttributesPtr) {
          command_data.eapConnectParams.ip = *ipAttributesPtr;
          command_data.eapConnectParams.ipUsed = 1U;
        }

        /* Start EAP connect. */
        status = EmwApiBase::eEMW_STATUS_ERROR;

        if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
            reinterpret_cast<uint8_t *>(&command_data.eapConnectParams),
            (uint16_t)sizeof(command_data.eapConnectParams),
            reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
          if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
            status = EmwApiBase::eEMW_STATUS_OK;
          }
        }
        DEBUG_API_LOG("EmwApiCore::connectEAP()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
      }
    }
  }
  return status;
}

EmwApiBase::Status EmwApiCore::connectWPS(void) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_WPS_CONNECT_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);
  DEBUG_API_LOG("\nEmwApiCore::connectWPS()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, 15000U)) {
    if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("EmwApiCore::connectWPS()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
  return status;
}

EmwApiBase::Status EmwApiCore::disconnect(void) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_DISCONNECT_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

  DEBUG_API_LOG("\nEmwApiCore::disconnect()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, 15000U)) {
    if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("EmwApiCore::disconnect()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
  return status;
}

const char *EmwApiCore::getConfigurationString(void) const
{
  static const char configuration_string[] = {
    EMW_IO_NAME_STRING ", " RTOS_NAME_STRING ", " NETWORK_NAME_STRING
  };
  return configuration_string;
}

void EmwApiCore::getStatistics(void) const
{
  EMW_STATS_LOG()
}


EmwApiBase::Status EmwApiCore::getIPAddress(uint8_t *ipAddressPtr, EmwApiBase::EmwInterface interface)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::getIPAddress()>\n");

  if (nullptr != ipAddressPtr) {
    EmwCoreIpc::WiFiGetIpResponseParams_t response_buffer;
    uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer);
    EmwCoreIpc::IpcInterfaceParams_t command_data;

    command_data.interfaceNum = this->toIpcInterface(interface);
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.interfaceNum), (uint16_t)sizeof(command_data.interfaceNum),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EmwApiCore::GET_IP_ADDRESS_TIMEOUT)) {
      if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
        {
          EmwAddress::IpAddr_t ip;
          EmwAddress::asciiToNetwork(&response_buffer.ip.ipAddressLocal[0], ip);
          (void) memcpy(&this->stationSettings.ipAddress[0], &ip.addr,
                        sizeof(this->stationSettings.ipAddress));
        }
        {
          EmwAddress::IpAddr_t netmask;
          EmwAddress::asciiToNetwork(&response_buffer.ip.networkMask[0], netmask);
          (void) memcpy(&this->stationSettings.ipMask[0], &netmask.addr, sizeof(this->stationSettings.ipMask));
        }
        {
          EmwAddress::IpAddr_t gw;
          EmwAddress::asciiToNetwork(&response_buffer.ip.gatewayAddress[0], gw);
          (void) memcpy(&this->stationSettings.gatewayAddress[0], &gw.addr,
                        sizeof(this->stationSettings.gatewayAddress));
        }
        {
          EmwAddress::IpAddr_t dns;
          EmwAddress::asciiToNetwork(&response_buffer.ip.dnsServerAddress[0], dns);
          (void) memcpy(&this->stationSettings.dns1[0], &dns.addr, sizeof(this->stationSettings.dns1));
        }
        (void) memcpy(ipAddressPtr, this->stationSettings.ipAddress,
                      sizeof(this->stationSettings.ipAddress));
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
    DEBUG_API_LOG("EmwApiCore::getIPAddress< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
  }
  return status;
}

EmwApiBase::Status EmwApiCore::getIP6Address(uint8_t *ip6AddressPtr, uint8_t addressSlot,
    EmwApiBase::EmwInterface interface)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::getIP6Address()>\n")

  if ((nullptr != ip6AddressPtr) && (addressSlot < 3U)) {
    EmwCoreIpc::IpcWiFiGetIp6AddrParams_t command_data(addressSlot, this->toIpcInterface(interface));
    EmwCoreIpc::WiFiGetIp6AddrResponseParams_t response_buffer;
    uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.getIp6AddrParams), (uint16_t)sizeof(command_data.getIp6AddrParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EmwApiCore::GET_IP_ADDRESS_TIMEOUT)) {
      if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
        (void) memcpy(this->stationSettings.ipv6Address[addressSlot], response_buffer.ip6,
                      sizeof(this->stationSettings.ipv6Address[addressSlot])); /* size of an array of 16 bytes. */
        (void) memcpy(ip6AddressPtr, response_buffer.ip6, sizeof(response_buffer.ip6));
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
    DEBUG_API_LOG("EmwApiCore::getIP6Address()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
  }
  return status;
}

int32_t EmwApiCore::getIP6AddressState(uint8_t addressSlot, EmwApiBase::EmwInterface interface) const
{
  int32_t state = -1;
  EmwCoreIpc::IpcAddressSlotInterfaceParams_t command_data;
  EmwCoreIpc::WiFiGetIp6StateResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer);
  DEBUG_API_LOG("\nEmwApiCore::getIP6AddressState()>\n")

  command_data.addressSlotInterfaceNum.addressSlot = addressSlot;
  command_data.addressSlotInterfaceNum.interfaceNum = this->toIpcInterface(interface);
  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.addressSlotInterfaceNum),
      (uint16_t)sizeof(command_data.addressSlotInterfaceNum),
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    state = (int32_t)response_buffer.state;
  }
  DEBUG_API_LOG("EmwApiCore::getIP6AddressState()< %" PRIi32 "\n\n", state)
  return state;
}

EmwApiBase::Status EmwApiCore::getStationMacAddress(EmwApiCore::MacAddress_t &mac) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::getStationMacAddress()>\n")

  if (0U < this->runtime.interfaces) {
    (void) memcpy(mac.bytes, this->systemInformations.mac48bitsStation, 6U);
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG("EmwApiCore::getStationMacAddress()< %" PRIi32 "\n\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::getSoftApMacAddress(EmwApiCore::MacAddress_t &mac)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::getSoftApMacAddress()>\n")

  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_GET_SOFT_MAC_CMD);
  uint16_t response_buffer_size = 6U;
  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      this->systemInformations.mac48bitsAp, &response_buffer_size, EMW_CMD_TIMEOUT)) {
    (void) memcpy(mac.bytes, &this->systemInformations.mac48bitsAp[0], 6U);
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG("EmwApiCore::getSoftApMacAddress()< %" PRIi32 "\n\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::getVersion(char version[], uint32_t versionSize) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if ((nullptr != version) && (0U < versionSize)) {
    EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_VERSION_CMD);
    uint8_t firmware_revision[24];
    uint16_t response_buffer_size = (uint16_t)sizeof(firmware_revision);

    VOID_MEMSET_ARRAY(firmware_revision, 0);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
        &firmware_revision[0], &response_buffer_size, EMW_CMD_TIMEOUT)) {
      (void) memcpy(version, &firmware_revision[0], versionSize);
      version[versionSize - 1U] = (uint8_t)'\0';
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  return status;
}

int8_t EmwApiCore::getScanResults(uint8_t *resultsPtr, uint8_t number) const
{
  uint8_t copy_number = 0U;

  if ((nullptr != resultsPtr) && (0U != number)) {
    copy_number = (this->runtime.scanResults.count < number) ? this->runtime.scanResults.count : number;
    (void) memcpy(resultsPtr, this->runtime.scanResults.accessPoints,
                  (size_t)copy_number * sizeof(this->runtime.scanResults.accessPoints[0]));
  }
  DEBUG_API_LOG("EmwApiCore::getScanResults()< %" PRIi32 "\n", (int32_t)copy_number)
  return (int8_t)copy_number;
}

EmwApiBase::Status EmwApiCore::initialize(void)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::initialize()>\n")

  {
    EmwScopedLock lock(&DeviceAvailableLock);

    if (0U == this->runtime.interfaces) {
      struct SystemInformations_s system_informations_default;
      struct StationSettings_s station_settings_default;
      EmwApiBase::SoftApSettings_t access_point_settings_default;
      struct Runtime_s runtime_default;

      this->systemInformations = system_informations_default;
      this->stationSettings = station_settings_default;
      this->accessPointSettings = access_point_settings_default;
      this->runtime = runtime_default;

      this->ioPtr->initialize(*this, EmwIoInterface::eINITIALIZE);
      EmwCoreIpc::initialize(this);
#if defined(EMW_WITH_RTOS)
      {
        static const char receive_thread_name[] = {"EMW-ReceiveThread"};
        const EmwOsInterface::Status os_status = EmwOsInterface::createThread(&receiveThread,
          receive_thread_name, receiveThreadFunction, (EmwOsInterface::thread_function_argument)this,
          EMW_RECEIVED_THREAD_STACK_SIZE, EMW_RECEIVED_THREAD_PRIORITY);
        EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
      }
      (void) EmwOsInterface::delayTicks(1U);
#endif /* EMW_WITH_RTOS */
      this->runtime.interfaces = 1U;
    }
    else {
      this->runtime.interfaces++;
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  {
    EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_VERSION_CMD);
    uint16_t response_buffer_size = (uint16_t)sizeof(this->systemInformations.firmwareRevision);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
        reinterpret_cast<uint8_t *>(this->systemInformations.firmwareRevision), &response_buffer_size, EMW_CMD_TIMEOUT)) {
      {
        const char firmware_revision_required_string[] = "V2.3.4";
        const bool out_of_date = (0 < strncmp(firmware_revision_required_string,
                                              static_cast<const char *>(this->systemInformations.firmwareRevision),
                                              sizeof(firmware_revision_required_string)));
        if (out_of_date) {
          DRIVER_ERROR_VERBOSE("ERROR: The Wi-Fi firmware is out of date\n")
        }
        EmwOsInterface::assertAlways(!out_of_date);
      }
    }
    else {
      this->unInitialize();
    }
  }
  {
    EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_GET_MAC_CMD);
    uint16_t response_buffer_size = (uint16_t)sizeof(this->systemInformations.mac48bitsStation);
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
        this->systemInformations.mac48bitsStation, &response_buffer_size, EMW_CMD_TIMEOUT)) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
    else {
      this->unInitialize();
    }
  }
  DEBUG_API_LOG("EmwApiCore::initialize()< %" PRIi32 "\n\n", (int32_t)status)
  return status;
}

int8_t EmwApiCore::isConnected(void)
{
  int8_t state = 0;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_GET_LINKINFO_CMD);
  EmwCoreIpc::WiFiGetLinkInfoResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer);
  DEBUG_API_LOG("\nEmwApiCore::isConnected()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
      this->stationSettings.isConnected = (int8_t)response_buffer.info.isConnected;

      if (0 < this->stationSettings.isConnected) {
        state = 1;
      }
    }
  }
  DEBUG_API_LOG("EmwApiCore::isConnected()< %" PRIi32 "\n\n", (int32_t)response_buffer.status)
  return state;
}

EmwApiBase::Status EmwApiCore::registerStatusCallback(const EmwApiBase::WiFiStatusCallback_t statusCallbackFunctionPtr,
    void *argPtr, EmwApiBase::EmwInterface interface)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  DEBUG_API_LOG("\nEmwApiCore::registerStatusCallback()>\n")

  if (0U < this->runtime.interfaces) {
    const uint8_t interface_index = (EmwApiBase::eSTATION == interface) \
                                    ? (uint8_t)EmwApiBase::eWIFI_INTERFACE_STATION_IDX \
                                    : (uint8_t)EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX;
    this->runtime.wiFiStatusCallbacks[interface_index] = statusCallbackFunctionPtr;
    this->runtime.wiFiStatusCallbackArgPtrs[interface_index] = argPtr;
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG("EmwApiCore::registerStatusCallback()< %" PRIi32 "\n\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::resetHardware(void) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_OK;
  EmwOsInterface::lock();
  DEBUG_API_LOG("\nEmwApiCore::resetHardware()>\n")

  if (0U == this->runtime.interfaces) {
    (void) this->ioPtr->initialize(*this, EmwIoInterface::eRESET);
  }
  {
    static bool done_once = false;

    if (!done_once) {
      static const char device_available_lock_name[] = {"EMW-DeviceAvailableLock"};
      const EmwOsInterface::Status os_status \
        = EmwOsInterface::createMutex(&DeviceAvailableLock, device_available_lock_name);
      EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
      done_once = true;
    }
  }
  DEBUG_API_LOG("EmwApiCore::resetHardware()< %" PRIi32 "\n\n", (int32_t)status)
  EmwOsInterface::unLock();
  return status;
}

EmwApiBase::Status EmwApiCore::resetModule(void) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_REBOOT_CMD);
  DEBUG_API_LOG("\nEmwApiCore::resetModule()>\n")

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U, nullptr, nullptr, EMW_CMD_TIMEOUT)) {
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG("EmwApiCore::resetModule()< %" PRIi32 "\n\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::resetToFactoryDefault(void) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eSYS_RESET_CMD);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U, nullptr, nullptr, EMW_CMD_TIMEOUT)) {
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  return status;
}

EmwApiBase::Status EmwApiCore::scan(EmwApiBase::ScanMode scanMode,
                                    const char *ssidStringPtr, int32_t ssidLength)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  this->runtime.scanResults.count = 0U;
  VOID_MEMSET_ARRAY(&this->runtime.scanResults.accessPoints, 0);

  if (((EmwApiBase::eSCAN_ACTIVE == scanMode) \
       && ((nullptr == ssidStringPtr) || (ssidLength <= 0) || (32 < ssidLength)))) {
    status = EmwApiBase::eEMW_STATUS_PARAM_ERROR;
  }
  else {
    EmwCoreIpc::IpcWiFiScanParams_t command_data;
    EmwCoreIpc::WiFiScanResponseParams_t *const response_parameters_ptr \
      = reinterpret_cast<EmwCoreIpc::WiFiScanResponseParams_t *>(&this->runtime.scanResults);
    uint16_t response_parameters_size = (uint16_t)sizeof(this->runtime.scanResults);

    if (EmwApiBase::eSCAN_ACTIVE == scanMode) {
      STRING_COPY_TO_ARRAY_INT8(command_data.scanParams.ssid, ssidStringPtr);
    }
    else {
      STRING_COPY_TO_ARRAY_INT8(command_data.scanParams.ssid, "");
    }
    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t * >(&command_data.scanParams),
        (uint16_t)sizeof(command_data.scanParams),
        reinterpret_cast<uint8_t * >(response_parameters_ptr), &response_parameters_size, EmwApiCore::SCAN_TIMEOUT_IN_MS)) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("EmwApiCore::scan()< %" PRIi32 "\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::setTimeout(uint32_t timeoutInMs)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (0U < this->runtime.interfaces) {
    this->runtime.timeoutInMs = timeoutInMs;
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  return status;
}

EmwApiBase::Status EmwApiCore::startSoftAp(const EmwApiBase::SoftApSettings_t *accessPointSettingsPtr) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (nullptr != accessPointSettingsPtr) {
    EmwCoreIpc::IpcWiFiSoftApStartParams_t command_data;
    EmwCoreIpc::SysCommonResponseParams_t response_buffer;
    uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

    (void) memcpy(command_data.softApStartParams.ssid,
                  accessPointSettingsPtr->ssidString, sizeof(command_data.softApStartParams.ssid));
    (void) memcpy(command_data.softApStartParams.key,
                  accessPointSettingsPtr->passwordString, sizeof(command_data.softApStartParams.key));
    command_data.softApStartParams.channel = (int32_t)accessPointSettingsPtr->channel;
    STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.ipAddressLocal,
                              accessPointSettingsPtr->ip.ipAddressLocal);
    STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.networkMask,
                              accessPointSettingsPtr->ip.networkMask);
    STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.gatewayAddress,
                              accessPointSettingsPtr->ip.gatewayAddress);
    STRING_COPY_TO_ARRAY_CHAR(command_data.softApStartParams.ip.dnsServerAddress,
                              accessPointSettingsPtr->ip.dnsServerAddress);

    if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
        reinterpret_cast<uint8_t *>(&command_data.softApStartParams),
        (uint16_t)sizeof(command_data.softApStartParams),
        reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, 3000U)) {
      if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
        status = EmwApiBase::eEMW_STATUS_OK;
      }
    }
    DEBUG_API_LOG("EmwApiCore::startAP()< %" PRIi32 "\n", (int32_t)status)
  }
  return status;
}

EmwApiBase::Status EmwApiCore::stopSoftAp(void) const
{
  EmwApiBase::Status ret = EmwApiBase::eEMW_STATUS_ERROR;

  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_SOFTAP_STOP_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
      ret = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("EmwApiCore::stopAP()< %" PRIi32 "\n", (int32_t)ret)
  return ret;
}

EmwApiBase::Status EmwApiCore::stopWPS(void) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_WPS_STOP_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
      status = EmwApiBase::eEMW_STATUS_OK;
    }
  }
  DEBUG_API_LOG("EmwApiCore::stopWPS()< %" PRIi32 "\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::unRegisterStatusCallback(EmwApiBase::EmwInterface interface)
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (0U < this->runtime.interfaces) {
    const uint8_t interface_index = (EmwApiBase::eSTATION == interface) \
                                    ? (uint8_t)EmwApiBase::eWIFI_INTERFACE_STATION_IDX \
                                    : (uint8_t)EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX;
    this->runtime.wiFiStatusCallbacks[interface_index] = nullptr;
    this->runtime.wiFiStatusCallbackArgPtrs[interface_index] = nullptr;
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  DEBUG_API_LOG("EmwApiCore::unRegisterStatusCallback()< %" PRIi32 "\n", (int32_t)status)
  return status;
}

EmwApiBase::Status EmwApiCore::setEapCert(uint8_t certificateType, const char *certificateStringPtr,
    uint32_t length) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  (void) length;

  if (nullptr != certificateStringPtr) {
    const size_t certificate_length = strlen(certificateStringPtr);
    const size_t command_parameters_size = sizeof(EmwCoreIpc::WiFiEapSetCertParams_t) +
                                           certificate_length; /* len + 1 */

    EmwCoreIpc::IpcWiFiEapSetCertParams_t *const command_data \
      = static_cast<EmwCoreIpc::IpcWiFiEapSetCertParams_t *>\
        (EmwOsInterface::malloc(sizeof(EmwCoreIpc::CmdParams_t) + command_parameters_size));

    if (nullptr != command_data) {
      EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eWIFI_EAP_SET_CERT_CMD);
      EmwCoreIpc::SysCommonResponseParams_t response_buffer;
      uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

      command_data->ipcParams = ipc_params;
      command_data->eapSetCertParams.type = certificateType;
      command_data->eapSetCertParams.length = (uint16_t)certificate_length;
      (void) memcpy(command_data->eapSetCertParams.cert,
                    certificateStringPtr, (size_t)certificate_length);

      if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
          reinterpret_cast<uint8_t *>(&command_data->eapSetCertParams), (uint16_t)command_parameters_size,
          reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
        if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
          status = EmwApiBase::eEMW_STATUS_OK;
        }
      }
      EmwOsInterface::free(command_data);
    }
  }
  return status;
}

int32_t EmwApiCore::stationPowerSave(int32_t onOff)
{
  int32_t status = -1;
  EmwCoreIpc::IpcNoParams_t command_data(EmwCoreIpc::eWIFI_PS_OFF_CMD);
  EmwCoreIpc::SysCommonResponseParams_t response_buffer;
  uint16_t response_buffer_size = (uint16_t)sizeof(response_buffer.status);

  if (0 != onOff) {
    EmwCoreIpc::CmdParams_s ipc_params(EmwCoreIpc::eWIFI_PS_ON_CMD);
    command_data.ipcParams = ipc_params;
  }
  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::request(*this,
      reinterpret_cast<uint8_t *>(&command_data.dataNull), 0U,
      reinterpret_cast<uint8_t *>(&response_buffer), &response_buffer_size, EMW_CMD_TIMEOUT)) {
    if ((int32_t)EmwCoreIpc::eSUCCESS == response_buffer.status) {
      status = 0;
    }
  }
  return status;
}

EmwApiBase::Status EmwApiCore::testIpcEcho(uint8_t *dataInPtr, uint16_t dataInLength,
    uint8_t *dataOutPtr, uint16_t *dataOutLength, uint32_t timeoutInMs) const
{
  EmwApiBase::Status status = EmwApiBase::eEMW_STATUS_ERROR;

  if (EmwCoreIpc::eSUCCESS == EmwCoreIpc::testIpcEcho(*this, dataInPtr, dataInLength,
      dataOutPtr, dataOutLength, timeoutInMs)) {
    status = EmwApiBase::eEMW_STATUS_OK;
  }
  return status;
}

void EmwApiCore::unInitialize(void)
{
  EmwScopedLock lock(&DeviceAvailableLock);

  if (1U == this->runtime.interfaces) {
    this->runtime.interfaces = 0U;

#if defined(EMW_WITH_RTOS)
    this->receiveThreadQuitFlag = true;
    while (receiveThreadQuitFlag) {
      (void) EmwOsInterface::delay(50U);
    }
    (void) EmwOsInterface::terminateThread(&receiveThread);
#endif /* EMW_WITH_RTOS */
    (void) EmwCoreIpc::unInitialize();
    (void) ioPtr->unInitialize(*this);
  }
  else {
    if (0U < this->runtime.interfaces) {
      this->runtime.interfaces--;
    }
  }
  DEBUG_API_LOG("EmwApiCore::unInitialize()<\n")
  EMW_STATS_LOG()
}

#if defined(EMW_WITH_RTOS)
__IO bool EmwApiCore::receiveThreadQuitFlag;
EmwOsInterface::thread_t EmwApiCore::receiveThread;

void EmwApiCore::receiveThreadFunction(EmwOsInterface::thread_function_argument argument)
{
  /*const*/ class EmwApiCore *const core_ptr = (/*const*/ class EmwApiCore *)argument;

#if defined(EMW_API_DEBUG)
  SETBUF(stdout, nullptr)
#endif /* EMW_API_DEBUG */

  EmwApiCore::receiveThreadQuitFlag = false;

  while (EmwApiCore::receiveThreadQuitFlag != true) {
    EmwCoreIpc::poll(nullptr, core_ptr, 500U);
  }

  EmwApiCore::receiveThreadQuitFlag = false;
  EmwOsInterface::exitThread();
}
#endif /* EMW_WITH_RTOS */

uint8_t EmwApiCore::toIpcInterface(EmwApiBase::EmwInterface interface) const
{
  const uint8_t interface_num = (EmwApiBase::eSOFTAP == interface) ? (uint8_t)0U : (uint8_t)1U;
  return interface_num;
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
