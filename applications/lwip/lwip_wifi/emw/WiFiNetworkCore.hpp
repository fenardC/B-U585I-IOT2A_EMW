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

#include "EmwApiEmwBypass.hpp"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "arch/sys_arch.h"
#include <cstdint>

template <class WiFiNetwork> class WiFiNetworkInterface {
  protected:
    WiFiNetworkInterface(void) noexcept {}
  public:
    virtual ~WiFiNetworkInterface(void) noexcept {}
  public:
    std::int32_t activate(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
    {
      return static_cast<WiFiNetwork *>(this)->activateImp(ssidString, passwordString);
    }
  public:
    std::int32_t deActivate(void) noexcept
    {
      return static_cast<WiFiNetwork *>(this)->deActivateImp();
    }
  public:
    void inputNetif(EmwNetworkStack::Buffer_t *bufferPtr) noexcept
    {
      static_cast<WiFiNetwork *>(this)->inputNetifImp(bufferPtr);
    }
};


class WiFiNetworkSoftAp;
class WiFiNetworkStation;

class WiFiNetworkCore {
  public:
    static void InitializeSystem(class EmwApiCore *wifiEmwPtr) noexcept;
  public:
    static void UnInitializeSystem(void) noexcept;

  public:
    WiFiNetworkCore(void) noexcept;
  public:
    explicit WiFiNetworkCore(const WiFiNetworkCore &other) = delete;
  public:
    virtual ~WiFiNetworkCore(void) noexcept;

  protected:
    static std::int32_t UnInitializeEmw(void) noexcept;
  protected:
    static void OutputToTransmitFifo(struct pbuf *bufferPtr) noexcept;

  private:
    static void DeleteTransmitFifo(void) noexcept;
  private:
    static void InputFromEmw(EmwNetworkStack::Buffer_t *bufferPtr, std::uint32_t interfaceIndex) noexcept;
  private:
    static void PushToEmw(std::uint32_t timeoutInMs) noexcept;
  private:
    static void TranmitThreadFunction(void *argumentPtr) noexcept;

  protected:
    static class EmwApiEmwBypass *emwPtr;
  protected:
    static class WiFiNetworkInterface<WiFiNetworkSoftAp> *WiFiNetworkSoftApPtr;
  protected:
    static class WiFiNetworkInterface<WiFiNetworkStation> *WiFiNetworkStationPtr;

  private:
    static sys_mbox_t TransmitFifo;
  private:
    static volatile bool TransmitThreadQuitFlag;
  private:
    static const int TRANSMIT_THREAD_PRIORITY = 17;
  private:
    static const int TRANSMIT_THREAD_STACK_SIZE = 256;
};
