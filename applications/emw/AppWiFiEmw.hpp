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

#include "EmwApiEmw.hpp"

class AppWiFiEmw final {
  public:
    AppWiFiEmw(void) noexcept;
  public:
    ~AppWiFiEmw(void) noexcept;
  public:
    void checkIoSpeed(void) noexcept;
  public:
    void connectToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    void enableSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept;
  public:
    void disableSoftAp(void) noexcept;
  public:
    void disconnectFromAp(void) noexcept;

  public:
    EmwApiEmw emw;
  public:
    static void EmwInterfaceStatusChanged(EmwApiBase::EmwInterface interface,
                                          enum EmwApiBase::WiFiEvent status,
                                          void *argPtr) noexcept;
  private:
    static volatile bool EmwInterfaceSoftApUp;
  private:
    static volatile bool EmwInterfaceStationUp;
};
