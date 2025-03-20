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

#include "EmwAddress.hpp"
#include "EmwApiBase.hpp"
#include "EmwApiCore.hpp"
#include "EmwCoreIpc.hpp"
#include <stdint.h>

class EmwApiEmw final : public EmwApiCore {
  public:
    EmwApiEmw(void) {}
  public:
    ~EmwApiEmw(void) {}
  public:
    int32_t socketClose(int32_t socketFd);
  public:
    int32_t socketCreate(int32_t domain, int32_t type, int32_t protocol);
  public:
    int32_t socketConnect(int32_t socketFd,
                          const EmwAddress::SockAddr_t *socketAddressPtr, int32_t socketAddressLength);
  public:
    int32_t socketGetAddrInfo(const char *nodeNameStringPtr, const char *serviceNameStringPtr,
                              const EmwAddress::AddrInfo_t &hints, EmwAddress::AddrInfo_t *resultPtr);
  public:
    int32_t socketGetHostByName(EmwAddress::SockAddr_t *socketAddressPtr, const char *nameStringPtr);
  public:
    int32_t socketGetSockOpt(int32_t socketFd, int32_t level,
                             int32_t optionName, void *optionValuePtr, uint32_t *optionLengthPtr);
  public:
    int32_t socketPing(const char *hostnameStringPtr,
                       int32_t count, int32_t delayInMs, int32_t responses[]);
  public:
    int32_t socketPing6(const char *hostnameStringPtr,
                        int32_t count, int32_t delayInMs, int32_t responses[]);

  public:
    int32_t socketSend(int32_t socketFd, const uint8_t *dataPtr,
                       int32_t dataLength, int32_t flags);
  public:
    int32_t socketSetSockOpt(int32_t socketFd, int32_t level,
                             int32_t optionName, const void *optionValuePtr, int32_t optionLength);
  public:
    int32_t socketShutDown(int32_t socketFd, int32_t mode);
  public:
    int32_t socketReceive(int32_t socketFd, uint8_t *bufferPtr, int32_t bufferLength, int32_t flags);

  private:
    int32_t doSocketPing(EmwCoreIpc::ApiId api_id,
                         const char *hostnameStringPtr,
                         int32_t count, int32_t delayInMs, int32_t responses[]);
  private:
    static EmwAddress::SockAddrIn_t socketAddressIn_FromPacked(const EmwAddress::SockAddrStorage_t *socketAddressPtr);
  private:
    static EmwAddress::SockAddrIn6_t socketAddressIn6_FromPacked(const EmwAddress::SockAddrStorage_t *socketAddressPtr);
  private:
    static EmwAddress::SockAddrStorage_t socketAddressIn_ToPacked(const EmwAddress::SockAddr_t *socketAddressPtr);
  private:
    static EmwAddress::SockAddrStorage_t socketAddressIn6_ToPacked(const EmwAddress::SockAddr_t *socketAddressPtr);
};
