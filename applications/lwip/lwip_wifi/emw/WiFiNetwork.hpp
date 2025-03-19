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
#include <cstdbool>
#include <cstdint>

class WiFiNetwork final {
  public:
    explicit WiFiNetwork(EmwApiBase::EmwInterface wiFiInterface, struct netif &networkInterface);
  public:
    ~WiFiNetwork(void);
  public:
    typedef struct {
      const char *ssidStringPtr;
      const char *pskStringPtr;
      int32_t securityMode;
    } WiFiCredentials_t;
  public:
    void initializeNetif(struct netif *netifPtr);
  public:
    int32_t startDriver(void);
  public:
    int32_t startSoftAccesPoint(void);
  public:
    int32_t startStation(void);
  public:
    int32_t stopDriver(void);
  public:
    int32_t unInitializeDriver(void);
  public:
    void unInitializeNetif(void);

  public:
    WiFiCredentials_t credentials;
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
    uint8_t driverAccessChannel;
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
    bool checkTimeout(uint32_t tickStart, uint32_t tickCount) const;
  private:
    static int32_t deleteTransmitFifo(void);
  private:
    static void informOfDriverStatus(EmwApiBase::EmwInterface interface,
                                     enum EmwApiBase::WiFiEvent status, void *argPtr);
  private:
    static void inputFromDriver(void *bufferPtr, uint32_t interfaceIndex);
  private:
    static void inputNetif(struct netif &networkInterface, struct pbuf *bufferPtr);
  private:
    static void netifStatusCallback(struct netif *netifPtr);
  private:
    static err_t outputToDriver(struct netif *netifPtr, struct pbuf *bufferPtr);
  private:
    void pushToDriver(uint32_t timeoutInMs);
  private:
    static const char *securityToString(enum EmwApiBase::SecurityType);
  private:
    static void tranmitThreadFunction(void *THIS);

  public:
    static void checkIoSpeed(void);
  public:
    static class WiFiNetwork *mySelf(struct netif *netifPtr);
  public:
    static int32_t scan(void);
  public:
    static void netifExtCallbackFunction(struct netif *netifPtr, netif_nsc_reason_t reason,
                                         const netif_ext_callback_args_t *argsPtr);
  public:
    static netif_ext_callback_t netifExtCallback;

  private:
    static const char *netifReasonToString(netif_nsc_reason_t reason);

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
    static class EmwApiEmwBypass driver;
  private:
    static sys_mbox_t transmitFifo;
  private:
    static volatile bool transmitThreadQuitFlag;
  private:
    static class WiFiNetwork *wiFiNetworks[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX];
  private:
    const int TRANSMIT_THREAD_PRIORITY = 17;
  private:
    const int TRANSMIT_THREAD_STACK_SIZE = 256;
};

extern "C" err_t InitializeWiFiNetif(struct netif *netifPtr);
extern "C" void UnInitializeWiFiNetif(struct netif *netifPtr);
