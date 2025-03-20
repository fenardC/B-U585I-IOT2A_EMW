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
#include "EmwIoInterface.hpp"
#if defined (EMW_USE_SPI_DMA)
#include "EmwIoSpi.hpp"
#endif /* EMW_USE_SPI_DMA) */
#include "EmwOsInterface.hpp"
#include <cstdint>

class EmwApiCore {
  public:
    EmwApiCore(void);
  public:
    explicit EmwApiCore(const class EmwApiCore &other);
  public:
    ~EmwApiCore(void);
  public:
    typedef struct MacAddress_s {
      MacAddress_s(void) : bytes{0U, 0U, 0U, 0U, 0U, 0U} {}
      uint8_t bytes[6];
    } MacAddress_t;
  public:
    EmwApiBase::Status checkNotified(uint32_t timeoutInMs) const;
  public:
    EmwApiBase::Status connect(const char *ssidStringPtr,
                               const char *passwordStringPtr, EmwApiBase::SecurityType securityType) const;
  public:
    EmwApiBase::Status connectAdvance(const char *ssidStringPtr,
                                      const char *passwordStringPtr,
                                      const EmwApiBase::ConnectAttributes_t *attributesPtr,
                                      const EmwApiBase::IpAttributes_t *ipAttributesPtr);
  public:
    EmwApiBase::Status connectEAP(const char *ssidStringPtr,
                                  const char *identityStringPtr, const char *passwordStringPtr,
                                  const EmwApiBase::EapAttributes_t *eapAttributesPtr,
                                  const EmwApiBase::IpAttributes_t *ipAttributesPtr) const;
  public:
    EmwApiBase::Status connectWPS(void) const;
  public:
    EmwApiBase::Status disconnect(void)const;
  public:
    const char *getConfigurationString(void) const;
  public:
    void getStatistics(void) const;
  public:
    EmwApiBase::Status getIPAddress(uint8_t *ipAddressPtr, EmwApiBase::EmwInterface interface);
  public:
    EmwApiBase::Status getIP6Address(uint8_t *ip6AddressPtr, uint8_t addressSlot, EmwApiBase::EmwInterface interface);
  public:
    int32_t getIP6AddressState(uint8_t addressSlot, EmwApiBase::EmwInterface interface) const;
  public:
    EmwApiBase::Status getStationMacAddress(EmwApiCore::MacAddress_t &mac) const;
  public:
    EmwApiBase::Status getSoftApMacAddress(EmwApiCore::MacAddress_t &mac);
  public:
    int8_t getScanResults(uint8_t *resultsPtr, uint8_t number) const;
  public:
    EmwApiBase::Status getVersion(char version[], uint32_t versionSize) const;
  public:
    EmwApiBase::Status initialize(void);
  public:
    int8_t isConnected(void);
  public:
    EmwApiBase::Status registerStatusCallback(const EmwApiBase::WiFiStatusCallback_t statusCallbackFunctionPtr,
        void *argPtr, EmwApiBase::EmwInterface interface);
  public:
    EmwApiBase::Status resetHardware(void) const;
  public:
    EmwApiBase::Status resetModule(void) const;
  public:
    EmwApiBase::Status resetToFactoryDefault(void) const;
  public:
    EmwApiBase::Status scan(EmwApiBase::ScanMode scanMode, const char *ssidStringPtr, int32_t ssidStringLength);
  public:
    EmwApiBase::Status setTimeout(uint32_t timeoutInMs);
  public:
    EmwApiBase::Status startSoftAp(const EmwApiBase::SoftApSettings_t *accessPointSettingsPtr) const;
  public:
    EmwApiBase::Status stopSoftAp(void) const;
  public:
    EmwApiBase::Status stopWPS(void) const;
  public:
    int32_t stationPowerSave(int32_t onOff);
  public:
    EmwApiBase::Status testIpcEcho(uint8_t *dataInPtr, uint16_t dataInLength,
                                   uint8_t *dataOutPtr, uint16_t *dataOutLengthPtr,
                                   uint32_t timeoutInMs) const;
  public:
    void unInitialize(void);
  public:
    EmwApiBase::Status unRegisterStatusCallback(EmwApiBase::EmwInterface interface);

  public:
    struct SystemInformations_s {
      SystemInformations_s(void)
        : productName{"MXCHIP-WIFI"}, productIdentifier{"EMW3080B"}, firmwareRevision{"V0.0.0"},
          mac48bitsSoftAp{0U, 0U, 0U, 0U, 0U, 0U}, mac48bitsStation{0U, 0U, 0U, 0U, 0U, 0U} {}
      char productName[12];
      char productIdentifier[9];
      char firmwareRevision[24];
      uint8_t mac48bitsSoftAp[6];
      uint8_t mac48bitsStation[6];
    } systemInformations;

  public:
    struct StationSettings_s {
      StationSettings_s(void)
        : ssidString{0}, passwordString{0}, security(EmwApiBase::eSEC_NONE)
        , dhcpIsEnabled(true), isConnected(0)
        , ipAddress{0}, ipMask{0}, gatewayAddress{0}, dns1{0}
        , ipv6State{0}, ipv6Address{0}, ipv6Mask{{0}, {0}, {0}}, ipv6GatewayAddress{0}, ipv6Dns1{0} {}
      char ssidString[32 + 1U];
      char passwordString[64 + 1U];
      EmwApiBase::SecurityType security;
      bool dhcpIsEnabled;
      int8_t isConnected;
      uint8_t ipAddress[4];
      uint8_t ipMask[4];
      uint8_t gatewayAddress[4];
      uint8_t dns1[4];
      int32_t ipv6State[3];
      uint8_t ipv6Address[3][16];
      uint8_t ipv6Mask[16];
      uint8_t ipv6GatewayAddress[16];
      uint8_t ipv6Dns1[16];
    } stationSettings;

  private:
    EmwApiBase::SoftApSettings_t softAccessPointSettings;

  public:
    struct Runtime_s {
      Runtime_s(void)
        : timeoutInMs{10000U}, wiFiStatusCallbacks{nullptr, nullptr}, wiFiStatusCallbackArgPtrs{nullptr, nullptr}
        , fotaStatusCallback(nullptr), fotaStatusCallbackArg(0U)
        , netlinkInputCallback(nullptr), scanResults(), interfaces(0U) {}
      uint32_t timeoutInMs;
      EmwApiBase::WiFiStatusCallback_t wiFiStatusCallbacks[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX];
      void *wiFiStatusCallbackArgPtrs[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX];
      EmwApiBase::FotaStatusCallback_t fotaStatusCallback;
      uint32_t fotaStatusCallbackArg;
      EmwApiBase::NetlinkInputCallback_t netlinkInputCallback;
      EmwApiBase::ScanResults_t scanResults;
      uint8_t interfaces;
    } runtime;

  public:
    EmwIoInterface *ioPtr;
#if defined (EMW_USE_SPI_DMA)
  private:
    class EmwIoSpi ioSPI;
#endif /* EMW_USE_SPI_DMA */
#if defined(EMW_WITH_RTOS)
  private:
    static volatile bool receiveThreadQuitFlag;
  private:
    static EmwOsInterface::thread_t receiveThread;
  private:
    static void receiveThreadFunction(EmwOsInterface::thread_function_argument argument);
#endif /* EMW_WITH_RTOS */

  private:
    EmwApiBase::Status setEapCert(uint8_t certificateType,
                                  const char *certificateStringPtr, uint32_t length) const;
  private:
    uint8_t toIpcInterface(EmwApiBase::EmwInterface interface) const;
  private:
    static const uint32_t GET_IP_ADDRESS_TIMEOUT = 12000U;
  private:
    static const uint32_t SCAN_TIMEOUT_IN_MS = 5000U;
};
