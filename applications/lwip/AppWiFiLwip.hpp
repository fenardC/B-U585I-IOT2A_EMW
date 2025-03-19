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

#include "WiFiNetwork.hpp"
extern "C" {
#include "lwip/netif.h"
}

class AppWiFiLwip final {
  public:
    AppWiFiLwip(void) noexcept;
  public:
    ~AppWiFiLwip(void) noexcept;
  public:
    void constructSoftApNetworkInterface(void) noexcept;
  public:
    void constructStationNetworkInterface(void) noexcept;
  public:
    void connectToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    void enableSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    void tearDownSoftApNetworkInterface(void) noexcept;
  public:
    void tearDownStationNetworkInterface(void) noexcept;
  public:
    struct netif netifSOFTAP;
  public:
    struct netif netifSTA;

  private:
    class WiFiNetwork networkSOFTAP;
  private:
    class WiFiNetwork networkSTA;
};
