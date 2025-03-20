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
#pragma once
#include "EmwIoInterface.hpp"
#include <cstdint>

#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif /* __PACKED_STRUCT */

namespace EmwApiBase {
  enum Status {
    eEMW_STATUS_OK = (0),
    eEMW_STATUS_ERROR = (-1),
    eEMW_STATUS_TIMEOUT = (-2),
    eEMW_STATUS_IO_ERROR = (-3),
    eEMW_STATUS_PARAM_ERROR = (-4)
  };
  enum EmwInterface {
    eSOFTAP,
    eSTATION
  };
  enum WiFiInterfaceIndex {
    eWIFI_INTERFACE_STATION_IDX = 0U,
    eWIFI_INTERFACE_SOFTAP_IDX = 1U,
    eWIFI_INTERFACE_IDX_MAX = 2U,
    eWIFI_INTERFACE_COUNT_MAX = eWIFI_INTERFACE_IDX_MAX
  };

  typedef __PACKED_STRUCT IpAttributes_s {
    constexpr IpAttributes_s(void) noexcept
      : ipAddressLocal{0}, networkMask{0}, gatewayAddress{0}, dnsServerAddress{0} {}
    char ipAddressLocal[16];
    char networkMask[16];
    char gatewayAddress[16];
    char dnsServerAddress[16];
  } IpAttributes_t;

  typedef std::uint8_t Security_t;

  typedef __PACKED_STRUCT ConnectAttributes_s {
    constexpr ConnectAttributes_s(void) noexcept
      : bssid{0}, channel(0U), security(0U) {}
    std::uint8_t bssid[6];
    std::uint8_t channel;
    EmwApiBase::Security_t security;
  } ConnectAttributes_t;

  enum EapType {
    eEAP_TYPE_TLS = 13,
    eEAP_TYPE_TTLS = 21,
    eEAP_TYPE_PEAP = 25
  };

  typedef __PACKED_STRUCT EapAttributes_s {
    constexpr EapAttributes_s(void) noexcept
      : eapType(0U), rootCaPtr(nullptr), clientCertificatePtr(nullptr), clientKeyPtr(nullptr) {}
    std::uint8_t eapType;
    const char *rootCaPtr;
    const char *clientCertificatePtr;
    const char *clientKeyPtr;
  } EapAttributes_t;

  enum SecurityType {
    eSEC_NONE = 0,
    eSEC_WEP,
    eSEC_WPA_TKIP,
    eSEC_WPA_AES,
    eSEC_WPA2_TKIP,
    eSEC_WPA2_AES,
    eSEC_WPA2_MIXED,
    eSEC_WPA3,
    eSEC_AUTO
  };

  enum ScanMode {
    ePASSIVE = 0,
    eSCAN_ACTIVE = 1
  };

  typedef __PACKED_STRUCT ApInfo_s{
    constexpr ApInfo_s(void) noexcept : rssi(0), ssid{0}, bssid{0}, channel(0), security(0U) {}
    std::int32_t rssi;
    char ssid[32U + 1U];
    std::uint8_t bssid[6];
    std::int32_t channel;
    EmwApiBase::Security_t security;
  } ApInfo_t;

  typedef __PACKED_STRUCT ScanResults_s{
    constexpr ScanResults_s(void) noexcept : count(0U), accessPoints{} {}
    std::uint8_t count;
    EmwApiBase::ApInfo_t accessPoints[10U];
  } ScanResults_t;

  typedef struct SoftApSettings_s {
    constexpr SoftApSettings_s(void) noexcept : ssidString{0}, passwordString{0}, channel(0U), ip() {}
    char ssidString[32U + 1U];
    char passwordString[64U + 1U];
    std::uint8_t channel;
    EmwApiBase::IpAttributes_t ip;
  } SoftApSettings_t;

  enum WiFiEvent {
    eWIFI_EVENT_NONE = 0x00,
    eWIFI_EVENT_STA_DOWN = 0x01,
    eWIFI_EVENT_STA_UP = 0x02,
    eWIFI_EVENT_STA_GOT_IP = 0X03,
    eWIFI_EVENT_AP_DOWN = 0x04,
    eWIFI_EVENT_AP_UP = 0x05
  };
  typedef std::uint8_t WiFiStatus_t;
  typedef void (*WiFiStatusCallback_t)(EmwApiBase::EmwInterface interface, enum EmwApiBase::WiFiEvent event,
                                       void *argPtr);
  enum FotaStatus {
    eFOTA_SUCCESS,
    eFOTA_FAILED
  };
  typedef void (*FotaStatusCallback_t)(EmwApiBase::FotaStatus status, std::uint32_t arg);
  typedef void (*NetlinkInputCallback_t)(void *networkBufferPtr, std::uint32_t arg);

  typedef __PACKED_STRUCT LinkInformation_s{
    constexpr LinkInformation_s(void) noexcept
      : isConnected(0), ssid{0}, bssid{0}, security(0), channel(0), rssi(0) {}
    std::uint8_t isConnected;
    char ssid[32];
    std::uint8_t bssid[6];
    std::uint8_t security;
    std::uint8_t channel;
    std::int32_t rssi;
  } LinkInformation_t;

  typedef void *Mtls_t;

  typedef __PACKED_STRUCT MdnsService_s{
    constexpr MdnsService_s(void) noexcept
      : servName{0}, servType{0}, domain{0}, port(0U), proto(0U), keyVals{0}, ipAddr(0U), ip6Addr{0U}, separator('\0') {}
    char servName[63 + 1];
    char servType[63 + 1];
    char domain[63 + 1];
    std::uint16_t port;
    std::int32_t proto;
    char keyVals[255 + 1];
    std::uint32_t ipAddr;
    std::uint32_t ip6Addr[4];
    char separator;
  } MdnsService_t;
}
