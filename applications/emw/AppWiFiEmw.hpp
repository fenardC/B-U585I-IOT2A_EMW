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
    AppWiFiEmw(void);
  public:
    ~AppWiFiEmw(void);
  public:
    void checkIoSpeed(void) const;
  public:
    void connectAp(const char *ssidStringPtr, const char *passwordStringPtr);
  public:
    void disableSoftAp(void);
  public:
    void disconnectAp(void);
  public:
    void enableSoftAp(void);
  public:
    class EmwApiEmw emw;
  public:
    static void deviceStatusChanged(EmwApiBase::EmwInterface interface,
                                    enum EmwApiBase::WiFiEvent status,
                                    void *argPtr);
  private:
    bool checkTimeout(uint32_t tickStart, uint32_t tickCount) const;
  private:
    static volatile bool deviceSoftApUp;
  private:
    static volatile bool deviceStationUp;
};
