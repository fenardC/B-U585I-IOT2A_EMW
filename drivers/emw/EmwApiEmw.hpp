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
#include <cstdint>

class EmwApiEmw final : public EmwApiCore {
  public:
    EmwApiEmw(void) noexcept;
  public:
    explicit EmwApiEmw(const EmwApiEmw& other) = delete;
  public:
    ~EmwApiEmw(void) noexcept override;
  public:
    std::int32_t socketClose(std::int32_t socketFd) noexcept;
  public:
    std::int32_t socketCreate(std::int32_t domain, std::int32_t type, std::int32_t protocol) noexcept;
  public:
    std::int32_t socketConnect(std::int32_t socketFd,
                               const EmwAddress::SockAddr_t &socketAddress, std::int32_t socketAddressLength) noexcept;
  public:
    std::int32_t socketGetAddrInfo(const char (&nodeNameString)[255], const char (&serviceNameString)[255],
                                   const EmwAddress::AddrInfo_t &hints, EmwAddress::AddrInfo_t &result) noexcept;
  public:
    std::int32_t socketGetHostByName(EmwAddress::SockAddr_t &socketAddress, const char (&nameString)[255]) noexcept;
  public:
    std::int32_t socketGetSockOpt(std::int32_t socketFd, std::int32_t level,
                                  std::int32_t optionName, void *optionValuePtr, std::uint32_t &optionLength) noexcept;
  public:
    std::int32_t socketPing(const char (&hostnameString)[255],
                            std::int32_t count, std::int32_t delayInMs, std::int32_t (&responses)[10]) noexcept;
  public:
    std::int32_t socketPing6(const char (&hostnameString)[255],
                             std::int32_t count, std::int32_t delayInMs, std::int32_t (&responses)[10]) noexcept;

  public:
    std::int32_t socketSend(std::int32_t socketFd, const std::uint8_t (&data)[], std::int32_t dataLength,
                            std::int32_t flags) noexcept;
  public:
    std::int32_t socketSetSockOpt(std::int32_t socketFd, std::int32_t level,
                                  std::int32_t optionName, const void *optionValuePtr, std::int32_t optionLength) noexcept;
  public:
    std::int32_t socketShutDown(std::int32_t socketFd, std::int32_t mode) noexcept;
  public:
    std::int32_t socketReceive(std::int32_t socketFd, uint8_t (&buffer)[], std::int32_t bufferLength,
                               std::int32_t flags) noexcept;
  public:
    enum /*class*/ TlsVersion : std::uint8_t {
      SSL_V3_MODE = 1,
      TLS_V1_0_MODE = 2,
      TLS_V1_1_MODE = 3,
      TLS_V1_2_MODE = 4
    };

  public:
    std::int32_t tlsSetVersion(EmwApiEmw::TlsVersion version) noexcept;
  public:
    std::int32_t tlsSetClientCertificate(const std::uint8_t (&certificate)[], std::uint16_t certificateLength) noexcept;
  public:
    std::int32_t tlsSetClientPrivateKey(const std::uint8_t (&privateKey)[], std::uint16_t privateKeyLength) noexcept;
  public:
    std::int32_t tlsConnect(std::int32_t domain, std::int32_t type, std::int32_t protocol,
                            const EmwAddress::SockAddrStorage_t &socketAddress, std::int32_t socketAddressSize,
                            const char (&caString)[2500], std::int32_t caStringLength) noexcept;
  public:
    std::int32_t tlsConnectSni(const char (&serverNameInformationString)[128], std::int32_t serverNameInformationLength,
                               const EmwAddress::SockAddrStorage_t &socketAddress, std::int32_t socketAddressSize,
                               const char (&caString)[2500], std::int32_t caStringLength) noexcept;
  public:
    std::int32_t tlsSend(EmwApiBase::Mtls_t, const std::uint8_t (&data)[], std::int32_t dataLength) noexcept;
  public:
    std::int32_t tlsReceive(EmwApiBase::Mtls_t, std::uint8_t (&data)[], std::int32_t dataLength) noexcept;
  public:
    std::int32_t tlsClose(EmwApiBase::Mtls_t) noexcept;
  public:
    std::int32_t tlsSetNonBlocking(EmwApiBase::Mtls_t, std::int32_t nonblock) noexcept;

  private:
    std::int32_t doSocketPing(std::uint16_t apiId,
                              const char (&hostnameString)[255],
                              std::int32_t count, std::int32_t delayInMs, std::int32_t (&responses)[10]) noexcept;
  private:
    static EmwAddress::SockAddrIn_t socketAddressIn_FromPacked(const EmwAddress::SockAddrStorage_t &socketAddress) noexcept;
  private:
    static EmwAddress::SockAddrIn6_t socketAddressIn6_FromPacked(const EmwAddress::SockAddrStorage_t &socketAddress)
    noexcept;
  private:
    static EmwAddress::SockAddrStorage_t socketAddressIn_ToPacked(const EmwAddress::SockAddr_t &socketAddress) noexcept;
  private:
    static EmwAddress::SockAddrStorage_t socketAddressIn6_ToPacked(const EmwAddress::SockAddr_t &socketAddress) noexcept;
};
