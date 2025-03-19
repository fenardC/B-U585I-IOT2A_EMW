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

#include "EmwApiEmwBypass.hpp"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "arch/sys_arch.h"
#include <cstdint>

class WiFiNetwork final {
  public:
    explicit WiFiNetwork(EmwApiBase::EmwInterface wiFiInterface, struct netif &networkInterface) noexcept;
  public:
    explicit WiFiNetwork(const WiFiNetwork &other) = delete;
  public:
    ~WiFiNetwork(void) noexcept;
  public:
    void initializeNetif(struct netif *netifPtr) noexcept;
  public:
    std::int32_t startDriver(void) noexcept;
  public:
    std::int32_t startSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    std::int32_t startConnectionToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    std::int32_t stopDriver(void) noexcept;
  public:
    std::int32_t unInitializeDriver(void) noexcept;
  public:
    void unInitializeNetif(void) noexcept;

  public:
    const char *deviceIdentifierStringPtr;
  public:
    const char *deviceNameStringPtr;
  public:
    char deviceVersionString[24 + 1];
  public:
    bool dhcpInformFlag;
  public:
    bool dhcpReleaseOnLinkLost;
  public:
    std::uint8_t driverAccessChannel;
  public:
    volatile bool ipAcquired;
  public:
    ip_addr_t ipAddress;
  public:
    ip_addr_t ipAddress6;
  public:
    ip_addr_t gatewayAddress;
  public:
    ip_addr_t gatewayAddress6;
  public:
    ip_addr_t maskAddress;
  public:
    ip_addr_t maskAddress6;
  public:
    ip_addr_t staticIpAddress;
  public:
    ip_addr_t staticDnsServerAddress;
  public:
    ip_addr_t staticGatewayAddress;
  public:
    ip_addr_t staticMaskAddress;

  private:
    static std::int32_t DeleteTransmitFifo(void) noexcept;
  private:
    static void InformOfDriverStatus(EmwApiBase::EmwInterface interface,
                                     enum EmwApiBase::WiFiEvent status, void *argPtr) noexcept;
  private:
    static void InputFromDriver(EmwNetworkStack::Buffer_t *bufferPtr, std::uint32_t interfaceIndex) noexcept;
  private:
    static void InputNetif(struct netif &networkInterface, EmwNetworkStack::Buffer_t *bufferPtr) noexcept;
  private:
    static void NetifStatusCallback(struct netif *netifPtr) noexcept;
  private:
    static err_t OutputToDriver(struct netif *netifPtr, struct pbuf *bufferPtr) noexcept;
  private:
    static void PushToDriver(std::uint32_t timeoutInMs) noexcept;
  private:
    static void TranmitThreadFunction(void *THIS) noexcept;

  public:
    static void CheckIoSpeed(void) noexcept;
  public:
    static class WiFiNetwork *MySelf(struct netif *netifPtr) noexcept;
  public:
    static std::int32_t Scan(void) noexcept;
  public:
    static void NetifExtCallbackFunction(struct netif *netifPtr, netif_nsc_reason_t reason,
                                         const netif_ext_callback_args_t *argsPtr) noexcept;
  public:
    static netif_ext_callback_t NetifExtCallback;

  private:
    EmwApiBase::EmwInterface driverInterface;
  private:
    volatile bool driverInterfaceUp;
  private:
    bool ipV4AddressFound;
  private:
    bool ipV6AddressFound;
  private:
    struct netif &netInterface;

  private:
    static class EmwApiEmwBypass Driver;
  private:
    static sys_mbox_t TransmitFifo;
  private:
    static volatile bool TransmitThreadQuitFlag;
  private:
    static class WiFiNetwork *WiFiNetworks[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX];
  private:
    const int TRANSMIT_THREAD_PRIORITY = 17;
  private:
    const int TRANSMIT_THREAD_STACK_SIZE = 256;
};

extern "C" err_t InitializeWiFiNetif(struct netif *netifPtr);
extern "C" void UnInitializeWiFiNetif(struct netif *netifPtr);
