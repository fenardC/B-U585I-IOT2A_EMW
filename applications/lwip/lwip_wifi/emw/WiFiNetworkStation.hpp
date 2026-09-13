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
#pragma once

#include "WiFiNetworkCore.hpp"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "arch/sys_arch.h"
#include <cstdint>

class WiFiNetworkStation final : public WiFiNetworkInterface<WiFiNetworkStation>, protected WiFiNetworkCore {
  public:
    static err_t InitializeNetif(struct netif *netifPtr) noexcept;
  public:
    static void UnInitializeNetif(struct netif *netifPtr) noexcept;

  public:
    explicit WiFiNetworkStation(struct netif &netif) noexcept;
  public:
    explicit WiFiNetworkStation(const WiFiNetworkStation &other) = delete;
  public:
    ~WiFiNetworkStation(void) noexcept override;
  public:
    std::int32_t activateImp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    std::int32_t deActivateImp(void) noexcept;
  public:
    void inputNetifImp(EmwNetworkStack::Buffer_t *bufferPtr) noexcept;

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
    static void EmwStatusChanged(EmwApiBase::EmwInterface interface,
                                 enum EmwApiBase::WiFiEvent status, void *argPtr) noexcept;
  private:
    static err_t OutputToTransmitFifo(struct netif *netifPtr, struct pbuf *bufferPtr) noexcept;
  private:
    static void NetifStatusCallback(struct netif *netifPtr) noexcept;
  private:
    static class WiFiNetworkStation *MySelf(struct netif *netifPtr) noexcept;
  private:
    static void NetifExtCallbackFunction(struct netif *netifPtr, netif_nsc_reason_t reason,
                                         const netif_ext_callback_args_t *argsPtr) noexcept;
  private:
    static netif_ext_callback_t NetifExtCallback;

  private:
    volatile bool emwInterfaceUp;
  private:
    bool ipV4AddressFound;
  private:
    bool ipV6AddressFound;
  private:
    struct netif &lwipNetif;
};
