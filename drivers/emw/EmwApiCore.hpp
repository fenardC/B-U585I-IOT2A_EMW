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

#include "EmwApiBase.hpp"
#include "EmwOsInterface.hpp"
#include <cstdint>

class EmwApiCore {
  public:
    EmwApiCore(void) noexcept;
  public:
    explicit EmwApiCore(const EmwApiCore &other) = delete;
  public:
    virtual ~EmwApiCore(void) noexcept;
  public:
    typedef struct MacAddress_s {
      constexpr MacAddress_s(void) noexcept
        : bytes{0U, 0U, 0U, 0U, 0U, 0U} {}
      std::uint8_t bytes[6];
    } MacAddress_t;
  public:
    EmwApiBase::Status checkNotified(std::uint32_t timeoutInMs) const noexcept;
  public:
    EmwApiBase::Status connect(const char (&ssidString)[33], const char (&passwordString)[65],
                               EmwApiBase::SecurityType securityType) const noexcept;
  public:
    EmwApiBase::Status connectAdvance(const char (&ssidString)[33], const char (&passwordString)[65],
                                      const EmwApiBase::ConnectAttributes_t &attributes,
                                      const EmwApiBase::IpAttributes_t &ipAttributes) const noexcept;
  public:
    EmwApiBase::Status connectEAP(const char (&ssidString)[33], const char (&identityString)[33],
                                  const char (&passwordString)[65], const EmwApiBase::EapAttributes_t &eapAttributes,
                                  const EmwApiBase::IpAttributes_t &ipAttributes) const noexcept;
  public:
    EmwApiBase::Status connectWPS(void) const noexcept;
  public:
    EmwApiBase::Status disconnect(void) const noexcept;
  public:
    const char *getConfigurationString(void) const noexcept;
  public:
    void getStatistics(void) const noexcept;
  public:
    EmwApiBase::Status getIPAddress(std::uint8_t (&ipAddressBytes)[4], EmwApiBase::EmwInterface interface) noexcept;
  public:
    EmwApiBase::Status getIP6Address(std::uint8_t (&ip6AddressBytes)[16], std::uint8_t addressSlot,
                                     EmwApiBase::EmwInterface interface) noexcept;
  public:
    std::int32_t getIP6AddressState(std::uint8_t addressSlot, EmwApiBase::EmwInterface interface) const noexcept;
  public:
    EmwApiBase::Status getStationMacAddress(EmwApiCore::MacAddress_t &mac) const noexcept;
  public:
    EmwApiBase::Status getSoftApMacAddress(EmwApiCore::MacAddress_t &mac) noexcept;
  public:
    std::int8_t getScanResults(std::uint8_t (&results)[480], std::uint8_t number) const noexcept;
  public:
    EmwApiBase::Status getVersion(char (&version)[25], std::uint32_t versionSize) const noexcept;
  public:
    std::uint8_t numberOfInterfacesRunning(void) noexcept
    {
      return this->runtime.interfaces;
    }
  public:
    EmwApiBase::Status initialize(void) noexcept;
  public:
    std::int8_t isConnected(void) noexcept;
  public:
    EmwApiBase::Status registerStatusCallback(const EmwApiBase::WiFiStatusCallback_t statusCallbackFunctionPtr,
        void *argPtr, EmwApiBase::EmwInterface interface) noexcept;
  public:
    EmwApiBase::Status resetHardware(void) const noexcept;
  public:
    EmwApiBase::Status resetModule(void) const noexcept;
  public:
    EmwApiBase::Status resetToFactoryDefault(void) const noexcept;
  public:
    EmwApiBase::Status scan(EmwApiBase::ScanMode scanMode,
                            const char (&ssidString)[33], std::int32_t ssidStringLength) noexcept;
  public:
    EmwApiBase::Status setTimeout(std::uint32_t timeoutInMs) noexcept;
  public:
    EmwApiBase::Status startSoftAp(const EmwApiBase::SoftApSettings_t &accessPointSettings) const noexcept;
  public:
    EmwApiBase::Status stopSoftAp(void) const noexcept;
  public:
    EmwApiBase::Status stopWPS(void) const noexcept;
  public:
    int32_t stationPowerSave(std::int32_t onOff) noexcept;
  public:
    EmwApiBase::Status testIpcEcho(std::uint8_t (&dataIn)[], std::uint16_t dataInLength,
                                   std::uint8_t (&dataOut)[], std::uint16_t &dataOutLength, std::uint32_t timeoutInMs) const noexcept;
  public:
    void unInitialize(void) noexcept;
  public:
    EmwApiBase::Status unRegisterStatusCallback(EmwApiBase::EmwInterface interface) noexcept;

  public:
    struct SystemInformations_s {
      constexpr SystemInformations_s(void) noexcept
        : productName{"MXCHIP-WIFI"}, productIdentifier{"EMW3080B"}, firmwareRevision{"V0.0.0"},
      mac48bitsSoftAp{0U, 0U, 0U, 0U, 0U, 0U}, mac48bitsStation{0U, 0U, 0U, 0U, 0U, 0U} {}
      char productName[12];
      char productIdentifier[9];
      char firmwareRevision[24];
      std::uint8_t mac48bitsSoftAp[6];
      std::uint8_t mac48bitsStation[6];
    } systemInformations;

  public:
    struct StationSettings_s {
      constexpr StationSettings_s(void) noexcept
        : ssidString{0}, passwordString{0}, security(EmwApiBase::eSEC_NONE)
      , dhcpIsEnabled(true), isConnected(0)
      , ipAddress{0}, ipMask{0}, gatewayAddress{0}, dns1{0}
      , ipv6State{0}, ipv6Address{0}, ipv6Mask{{0}, {0}, {0}}, ipv6GatewayAddress{0}, ipv6Dns1{0} {}
      char ssidString[32 + 1U];
      char passwordString[64 + 1U];
      EmwApiBase::SecurityType security;
      bool dhcpIsEnabled;
      std::int8_t isConnected;
      std::uint8_t ipAddress[4];
      std::uint8_t ipMask[4];
      std::uint8_t gatewayAddress[4];
      std::uint8_t dns1[4];
      std::int32_t ipv6State[3];
      std::uint8_t ipv6Address[3][16];
      std::uint8_t ipv6Mask[16];
      std::uint8_t ipv6GatewayAddress[16];
      std::uint8_t ipv6Dns1[16];
    } stationSettings;

  private:
    EmwApiBase::SoftApSettings_t softAccessPointSettings;

  public:
    struct Runtime_s {
      constexpr Runtime_s(void) noexcept
        : timeoutInMs{10000U}, wiFiStatusCallbacks{nullptr, nullptr}, wiFiStatusCallbackArgPtrs{nullptr, nullptr}
      , fotaStatusCallback(nullptr), fotaStatusCallbackArg(0U)
      , netlinkInputCallback(nullptr), scanResults(), interfaces(0U) {}
      std::uint32_t timeoutInMs;
      EmwApiBase::WiFiStatusCallback_t wiFiStatusCallbacks[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX];
      void *wiFiStatusCallbackArgPtrs[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX];
      EmwApiBase::FotaStatusCallback_t fotaStatusCallback;
      std::uint32_t fotaStatusCallbackArg;
      EmwApiBase::NetlinkInputCallback_t netlinkInputCallback;
      EmwApiBase::ScanResults_t scanResults;
      volatile std::uint8_t interfaces;
    } runtime;

  public:

#if defined(EMW_WITH_RTOS)
  private:
    static EmwOsInterface::Thread_t ReceiveThread;
  private:
    static volatile bool ReceiveThreadQuitFlag;
  private:
    static void ReceiveThreadFunction(EmwOsInterface::ThreadFunctionArgument_t argument) noexcept;
#endif /* EMW_WITH_RTOS */

  private:
    EmwApiBase::Status setEapCert(std::uint8_t certificateType,
                                  const char *certificateStringPtr, std::uint32_t length) const noexcept;
  private:
    std::uint8_t toIpcInterface(EmwApiBase::EmwInterface interface) const noexcept;
  private:
    static const std::uint32_t GET_IP_ADDRESS_TIMEOUT = 12000U;
  private:
    static const std::uint32_t SCAN_TIMEOUT_IN_MS = 5000U;
};
