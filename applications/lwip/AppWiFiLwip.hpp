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
#include "WiFiNetwork.hpp"
extern "C" {
#include "lwip/netif.h"
}

class AppWiFiLwip final {
  public:
    AppWiFiLwip(void);
  public:
    ~AppWiFiLwip(void);
  public:
    void constructSoftApNetworkInterface(void);
  public:
    void constructStationNetworkInterface(void);
  public:
    void connectAp(const char *ssidStringPtr, const char *passwordStringPtr);
  public:
    void enableSoftAp(void);
  public:
    void tearDownSoftApNetworkInterface(void);
  public:
    void tearDownStationNetworkInterface(void);
  public:
    struct netif netifSOFTAP;
  public:
    struct netif netifSTA;

  private:
    class WiFiNetwork networkSOFTAP;
  private:
    class WiFiNetwork networkSTA;

  private:
    constexpr uint32_t makeU32(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    {
      uint32_t value \
        = static_cast<uint32_t>(a << 24) \
          | static_cast<uint32_t>(b << 16) \
          | static_cast<uint32_t>(c << 8) \
          | static_cast<uint32_t>(d);
      return value;
    }
};
