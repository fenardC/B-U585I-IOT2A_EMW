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

#include "EmwApiBase.hpp"
#include "EmwOsInterface.hpp"
#include "EmwNetworkStack.hpp"
#if defined(EMW_NETWORK_EMW_MODE)
#include "EmwAddress.hpp"
#endif /* EMW_NETWORK_EMW_MODE */
#include <cstdint>

#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif /* __PACKED_STRUCT */

class EmwApiCore;

#define EMW_API_VERSION ("2.4.0")

class EmwCoreIpc final {
  private:
    EmwCoreIpc(void) noexcept {};
  public:
    enum Status {
      eSUCCESS = (0),
      eERROR = (-1),
      eTIMEOUT = (-2),
      eNO_MEMORY = (-3)
    };

#define ID_NONE (static_cast<std::uint16_t>(0x0000U))
#define CMD_BASE (static_cast<std::uint16_t>(ID_NONE))
  public:
    enum ApiId : std::uint16_t {
      eSYS_CMD_BASE = static_cast<std::uint16_t>(0U),
      eSYS_ECHO_CMD,
      eSYS_REBOOT_CMD,
      eSYS_VERSION_CMD,
      eSYS_RESET_CMD,
      eSYS_FOTA_START_CMD,
      eSYS_CFG_SERVER_START_CMD,
      eSYS_CFG_SERVER_STOP_CMD,

      eWIFI_CMD_BASE = static_cast<std::uint16_t>(CMD_BASE + 0x0100U),
      eWIFI_GET_MAC_CMD,
      eWIFI_SCAN_CMD,
      eWIFI_CONNECT_CMD,
      eWIFI_DISCONNECT_CMD,
      eWIFI_SOFTAP_START_CMD,
      eWIFI_SOFTAP_STOP_CMD,
      eWIFI_GET_IP_CMD,
      eWIFI_GET_LINKINFO_CMD,
      eWIFI_PS_ON_CMD,
      eWIFI_PS_OFF_CMD,
      eWIFI_PING_CMD,
      eWIFI_BYPASS_SET_CMD,
      eWIFI_BYPASS_GET_CMD,
      eWIFI_BYPASS_OUT_CMD,
      eWIFI_EAP_SET_CERT_CMD,
      eWIFI_EAP_CONNECT_CMD,
      eWIFI_WPS_CONNECT_CMD,
      eWIFI_WPS_STOP_CMD,
      eWIFI_GET_IP6_STATE_CMD,
      eWIFI_GET_IP6_ADDR_CMD,
      eWIFI_GET_SOFT_MAC_CMD,
      eWIFI_PING6_CMD,

      eSOCKET_CMD_BASE = static_cast<std::uint16_t>(CMD_BASE + 0x0200U),
      eSOCKET_CREATE_CMD,
      eSOCKET_CONNECT_CMD,
      eSOCKET_SEND_CMD,
      eSOCKET_SENDTO_CMD,
      eSOCKET_RECV_CMD,
      eSOCKET_RECVFROM_CMD,
      eSOCKET_SHUTDOWN_CMD,
      eSOCKET_CLOSE_CMD,
      eSOCKET_GETSOCKOPT_CMD,
      eSOCKET_SETSOCKOPT_CMD,
      eSOCKET_BIND_CMD,
      eSOCKET_LISTEN_CMD,
      eSOCKET_ACCEPT_CMD,
      eSOCKET_SELECT_CMD,
      eSOCKET_GETSOCKNAME_CMD,
      eSOCKET_GETPEERNAME_CMD,
      eSOCKET_GETHOSTBYNAME_CMD,
      eSOCKET_GETADDRINFO_CMD,

      eTLS_SET_VERSION_CMD = static_cast<std::uint16_t>(eSOCKET_CMD_BASE + 0x0081U),
      eTLS_SET_CLIENT_CERTIFICATE_CMD,
      eTLS_SET_SERVER_CERTIFICATE_CMD,
      eTLS_ACCEPT_CMD,
      eTLS_CONNECT_SNI_CMD,
      eTLS_SEND_CMD,
      eTLS_RECEIVE_CMD,
      eTLS_CLOSE_CMD,
      eTLS_SET_NONBLOCK_CMD
    };
  public:
    static void Initialize(const EmwApiCore *corePtr) noexcept;
  public:
    static void Poll(void *THIS, const void *argumentPtr, std::uint32_t timeoutInMs) noexcept;
  public:
    static Status Request(const EmwApiCore &core,
                          std::uint8_t (&commandData)[], std::uint16_t commandDataSize,
                          std::uint8_t (&responseBuffer)[], std::uint16_t &responseBufferSize,
                          std::uint32_t timeoutInMs) noexcept;
  public:
    static void ResetIo(void) noexcept;
  public:
    static Status TestIpcEcho(const EmwApiCore &core,
                              std::uint8_t (&dataIn)[], std::uint16_t dataInLength,
                              std::uint8_t (&dataOut)[], std::uint16_t &dataOutLength,
                              std::uint32_t timeoutInMs) noexcept;
  public:
    static void UnInitialize(void) noexcept;

  public:
    typedef __PACKED_STRUCT CmdParams_s {
      constexpr CmdParams_s(void) noexcept
        : reqId{0U, 0U, 0U, 0U}, apiId{0U, 0U} {}
      constexpr explicit CmdParams_s(std::uint16_t apiId) noexcept
        : reqId{0U, 0U, 0U, 0U}, apiId{static_cast<std::uint8_t>(apiId & 0x00FFU), static_cast<std::uint8_t>((apiId & 0xFF00U) >> 8)} {}
      std::uint8_t reqId[4];
      std::uint8_t apiId[2];
    } CmdParams_t;

    typedef __PACKED_STRUCT SysCommonResponseParams_s {
      constexpr SysCommonResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } SysCommonResponseParams_t;

    typedef __PACKED_STRUCT SysFotaStartParams_s {
      constexpr SysFotaStartParams_s(void) noexcept : url{0}, md5{0} {}
      char url[256];
      char md5[64];
    } SysFotaStartParams_t;

    typedef __PACKED_STRUCT IpcSysFotaStartParams_s {
      constexpr IpcSysFotaStartParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSYS_FOTA_START_CMD), fotaStartParams() {}
      CmdParams_t ipcParams;
      SysFotaStartParams_t fotaStartParams;
    } IpcSysFotaStartParams_t;

    typedef __PACKED_STRUCT IpcInterfaceParams_s {
      constexpr IpcInterfaceParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eWIFI_GET_IP_CMD), interfaceNum(0U) {}
      constexpr explicit IpcInterfaceParams_s(std::uint8_t interfaceNum) noexcept
        : ipcParams(EmwCoreIpc::eWIFI_GET_IP_CMD), interfaceNum(interfaceNum) {}
      CmdParams_t ipcParams;
      std::uint8_t interfaceNum;
    } IpcInterfaceParams_t;

    typedef __PACKED_STRUCT IpcAddressSlotInterfaceParams_s {
      constexpr IpcAddressSlotInterfaceParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_GET_IP6_STATE_CMD), addressSlotInterfaceNum{0U, 0U} {}
      CmdParams_t ipcParams;
      struct {
        std::uint8_t addressSlot;
        std::uint8_t interfaceNum;
      } addressSlotInterfaceNum;
    } IpcAddressSlotInterfaceParams_t;

    typedef __PACKED_STRUCT IpcNoParams_s {
      constexpr IpcNoParams_s(void) noexcept
        : ipcParams() {}
      constexpr explicit IpcNoParams_s(EmwCoreIpc::ApiId apiId) noexcept
        : ipcParams(apiId) {}
      CmdParams_t ipcParams;
    } IpcNoParams_t;

    typedef __PACKED_STRUCT WiFiScanParams_s {
      constexpr WiFiScanParams_s(void) noexcept : ssid{0} {}
      std::int8_t ssid[33];
    } WiFiScanParams_t;

    typedef __PACKED_STRUCT IpcWiFiScanParams_s {
      constexpr IpcWiFiScanParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_SCAN_CMD), scanParams() {}
      CmdParams_t ipcParams;
      WiFiScanParams_t scanParams;
    } IpcWiFiScanParams_t;

    typedef __PACKED_STRUCT WiFiScanResponseParams_s {
      constexpr WiFiScanResponseParams_s(void) noexcept : numberOf(0U), ap{} {}
      std::uint8_t numberOf;
      EmwApiBase::ApInfo_t ap[1];
    } WiFiScanResponseParams_t;

    typedef __PACKED_STRUCT WiFiConnectParams_s {
      constexpr WiFiConnectParams_s(void) noexcept : ssid{0}, key{0}, keyLength(0), useAttribute(0), useIp(0), attr(), ip() {}
      char ssid[33];
      char key[65];
      std::int32_t keyLength;
      std::uint8_t useAttribute;
      std::uint8_t useIp;
      EmwApiBase::ConnectAttributes_t attr;
      EmwApiBase::IpAttributes_t ip;
    } WiFiConnectParams_t;

    typedef __PACKED_STRUCT IpcWiFiConnectParams_s {
      constexpr IpcWiFiConnectParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_CONNECT_CMD), connectParams() {}
      CmdParams_t ipcParams;
      WiFiConnectParams_t connectParams;
    } IpcWiFiConnectParams_t;

    typedef __PACKED_STRUCT WiFiLinkInfo_s{
      constexpr WiFiLinkInfo_s(void) noexcept : isConnected(0), rssi(0), ssid{0}, bssid{0}, key{0}, channel(0), security(0U) {}
      std::int32_t isConnected;
      std::int32_t rssi;
      std::int8_t ssid[33];
      std::uint8_t bssid[6];
      std::int8_t key[65];
      std::int32_t channel;
      EmwApiBase::Security_t security;
    } WiFiLinkInfo_t;

    typedef __PACKED_STRUCT WiFiGetLinkInfoResponseParams_s {
      constexpr WiFiGetLinkInfoResponseParams_s(void) noexcept : status(-1), info() {}
      std::int32_t status;
      WiFiLinkInfo_t info;
    } WiFiGetLinkInfoResponseParams_t;

    typedef __PACKED_STRUCT WiFiGetIpResponseParams_s {
      constexpr WiFiGetIpResponseParams_s(void) noexcept : status(-1), ip() {}
      std::int32_t status;
      EmwApiBase::IpAttributes_t ip;
    } WiFiGetIpResponseParams_t;

    typedef __PACKED_STRUCT WiFiSoftApStartParams_s {
      constexpr WiFiSoftApStartParams_s(void) noexcept : ssid{0}, key{0}, channel(0), ip() {}
      std::int8_t ssid[32];
      std::int8_t key[64];
      std::int32_t channel;
      EmwApiBase::IpAttributes_t ip;
    } WiFiSoftApStartParams_t;

    typedef __PACKED_STRUCT IpcWiFiSoftApStartParams_s {
      constexpr IpcWiFiSoftApStartParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_SOFTAP_START_CMD), softApStartParams() {}
      CmdParams_t ipcParams;
      WiFiSoftApStartParams_t softApStartParams;
    } IpcWiFiSoftApStartParams_t;

    typedef __PACKED_STRUCT WiFiBypassSetParams_s {
      constexpr WiFiBypassSetParams_s(void) noexcept : mode(0) {}
      std::int32_t mode;
    } WiFiBypassSetParams_t;

    typedef __PACKED_STRUCT IpcWiFiBypassSetParams_s {
      constexpr IpcWiFiBypassSetParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_BYPASS_SET_CMD), bypassSetParams() {}
      CmdParams_t ipcParams;
      WiFiBypassSetParams_t bypassSetParams;
    } IpcWiFiBypassSetParams_t;

    typedef __PACKED_STRUCT WiFiBypassGetResponseParams_s {
      constexpr WiFiBypassGetResponseParams_s(void) noexcept : mode(0) {}
      std::int32_t mode;
    } WiFiBypassGetResponseParams_t;

    typedef __PACKED_STRUCT WiFiBypassOutParams_s {
      constexpr WiFiBypassOutParams_s(void) noexcept : idx(0), useless{0U}, dataLength(0U) {}
      std::int32_t idx;
      std::uint8_t useless[16];
      std::uint16_t dataLength;
    } WiFiBypassOutParams_t;

    typedef __PACKED_STRUCT IpcWiFiBypassOutParams_s {
      constexpr IpcWiFiBypassOutParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_BYPASS_OUT_CMD), bypassOutParams() {}
      CmdParams_t ipcParams;
      WiFiBypassOutParams_t bypassOutParams;
    } IpcWiFiBypassOutParams_t;

    typedef __PACKED_STRUCT WiFiBypassInParams_s {
      constexpr WiFiBypassInParams_s(void) noexcept : idx(0), useless{0U}, totalLength(0U) {}
      std::int32_t idx;
      std::uint8_t useless[16];
      std::uint16_t totalLength;
    } WiFiBypassInParams_t;

    enum {
      EAP_ROOTCA = 0x01,
      EAP_CLIENT_CERT,
      EAP_CLIENT_KEY,
      EAP_CERT_TYPE_MAX
    };

    typedef __PACKED_STRUCT WiFiEapSetCertParams_s {
      constexpr WiFiEapSetCertParams_s(void) noexcept : type(0U), length(0U), cert{'\0'} {}
      std::uint8_t type;
      std::uint16_t length;
      char cert[1];
    } WiFiEapSetCertParams_t;

    typedef __PACKED_STRUCT IpcWiFiEapSetCertParams_s {
      constexpr IpcWiFiEapSetCertParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_EAP_SET_CERT_CMD), eapSetCertParams() {}
      CmdParams_t ipcParams;
      WiFiEapSetCertParams_t eapSetCertParams;
    } IpcWiFiEapSetCertParams_t;

    typedef __PACKED_STRUCT WiFiEapConnectParams_s {
      constexpr WiFiEapConnectParams_s(void) noexcept : ssid{0}, identity{0}, password{0}, attrUsed(0U), attr(), ipUsed(0U), ip() {}
      char ssid[32];
      char identity[32];
      char password[64];
      std::uint8_t attrUsed;
      EmwApiBase::EapAttributes_t attr;
      std::uint8_t ipUsed;
      EmwApiBase::IpAttributes_t ip;
    } WiFiEapConnectParams_t;

    typedef __PACKED_STRUCT IpcWiFiEapConnectParams_s {
      constexpr IpcWiFiEapConnectParams_s(void) noexcept : ipcParams(EmwCoreIpc::eWIFI_EAP_CONNECT_CMD), eapConnectParams() {}
      CmdParams_t ipcParams;
      WiFiEapConnectParams_t eapConnectParams;
    } IpcWiFiEapConnectParams_t;

    typedef __PACKED_STRUCT WiFiGetIp6StateParams_s {
      constexpr WiFiGetIp6StateParams_s(void) noexcept : addressSlot(0U), interfaceNum(0U) {}
      std::uint8_t addressSlot;
      uint8_t interfaceNum;
    } WiFiGetIp6StateParams_t;

    typedef __PACKED_STRUCT WiFiGetIp6StateResponseParams_s {
      constexpr WiFiGetIp6StateResponseParams_s(void) noexcept : state(0U) {}
      std::uint8_t state;
    } WiFiGetIp6StateResponseParams_t;

    typedef __PACKED_STRUCT WiFiGetIp6AddrParams_s {
      constexpr WiFiGetIp6AddrParams_s(void) noexcept
        : addressSlot(0U), interfaceNum(0U) {}
      constexpr explicit WiFiGetIp6AddrParams_s(std::uint8_t addressSlot, std::uint8_t interfaceNum) noexcept
        : addressSlot(addressSlot), interfaceNum(interfaceNum) {}
      std::uint8_t addressSlot;
      std::uint8_t interfaceNum;
    } WiFiGetIp6AddrParams_t;

    typedef __PACKED_STRUCT IpcWiFiGetIp6AddrParams_s {
      constexpr IpcWiFiGetIp6AddrParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eWIFI_GET_IP6_ADDR_CMD), getIp6AddrParams() {}
      constexpr explicit IpcWiFiGetIp6AddrParams_s(std::uint8_t addressSlot, std::uint8_t interfaceNum) noexcept
        : ipcParams(EmwCoreIpc::eWIFI_GET_IP6_ADDR_CMD), getIp6AddrParams(addressSlot, interfaceNum) {}
      CmdParams_t ipcParams;
      WiFiGetIp6AddrParams_t getIp6AddrParams;
    } IpcWiFiGetIp6AddrParams_t;

    typedef __PACKED_STRUCT WiFiGetIp6AddrResponseParams_s {
      constexpr WiFiGetIp6AddrResponseParams_s(void) noexcept : status(-1), ip6{0U} {}
      std::int32_t status;
      std::uint8_t ip6[16];
    } WiFiGetIp6AddrResponseParams_t;

#if defined(EMW_NETWORK_EMW_MODE)
    typedef __PACKED_STRUCT WiFiPingParams_s {
      constexpr WiFiPingParams_s(void) noexcept : hostname{0}, count(0), delayInMs(0) {}
      char hostname[255];
      std::int32_t count;
      std::int32_t delayInMs;
    } WiFiPingParams_t;

    typedef __PACKED_STRUCT IpcWiFiPingParams_s {
      constexpr explicit IpcWiFiPingParams_s(std::uint16_t apiId) noexcept : ipcParams(apiId), pingParams() {}
      CmdParams_t ipcParams;
      WiFiPingParams_t pingParams;
    } IpcWiFiPingParams_t;

    typedef __PACKED_STRUCT WiFiPingResponseParams_s {
      constexpr WiFiPingResponseParams_s(void) noexcept : numberOf(0), delaysInMs{0} {}
      std::int32_t numberOf;
      std::int32_t delaysInMs[10];
    } WiFiPingResponseParams_t;

    typedef __PACKED_STRUCT SocketCreateParams_s {
      constexpr SocketCreateParams_s(void) noexcept
        : domain(0), type(0), protocol(0) {}
      constexpr SocketCreateParams_s(std::int32_t domain, std::int32_t type, std::int32_t protocol) noexcept
        : domain(domain), type(type), protocol(protocol) {}
      std::int32_t domain;
      std::int32_t type;
      std::int32_t protocol;
    } SocketCreateParams_t;

    typedef __PACKED_STRUCT IpcSocketCreateParams_s {
      constexpr IpcSocketCreateParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_CREATE_CMD), createParams() {}
      constexpr explicit IpcSocketCreateParams_s(std::int32_t domain, std::int32_t type, std::int32_t protocol) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_CREATE_CMD), createParams(domain, type, protocol) {}
      CmdParams_t ipcParams;
      SocketCreateParams_t createParams;
    } IpcSocketCreateParams_t;

    typedef __PACKED_STRUCT SocketCreateResponseParams_s {
      constexpr SocketCreateResponseParams_s(void) noexcept : fd(-1) {}
      std::int32_t fd;
    } SocketCreateResponseParams_t;

    typedef __PACKED_STRUCT SocketSetSockOptParams_s {
      constexpr SocketSetSockOptParams_s(void) noexcept
        : socket(-1), level(0), name(0), length(0), value{0} {}
      constexpr explicit SocketSetSockOptParams_s(std::int32_t socket, std::int32_t level, std::int32_t name) noexcept
        : socket(socket), level(level), name(name), length(0), value{0} {}
      std::int32_t socket;
      std::int32_t level;
      std::int32_t name;
      EmwAddress::SockLen_t length;
      std::uint8_t value[16];
    } SocketSetSockOptParams_t;

    typedef __PACKED_STRUCT IpcSocketSetSockOptParams_s {
      constexpr IpcSocketSetSockOptParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_SETSOCKOPT_CMD), setSockOptParams() {}
      constexpr explicit IpcSocketSetSockOptParams_s(std::int32_t socket, std::int32_t level, std::int32_t name) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_SETSOCKOPT_CMD), setSockOptParams(socket, level, name) {}
      CmdParams_t ipcParams;
      SocketSetSockOptParams_t setSockOptParams;
    } IpcSocketSetSockOptParams_t;

    typedef __PACKED_STRUCT SocketSetSockOptResponseParams_s {
      constexpr SocketSetSockOptResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } SocketSetSockOptResponseParams_t;

    typedef __PACKED_STRUCT SocketGetSockOptParams_s {
      constexpr SocketGetSockOptParams_s(void) noexcept
        : socket(-1), level(0), name(0) {}
      constexpr explicit SocketGetSockOptParams_s(std::int32_t socket, std::int32_t level, std::int32_t name) noexcept
        : socket(socket), level(level), name(name) {}
      std::int32_t socket;
      std::int32_t level;
      std::int32_t name;
    } SocketGetSockOptParams_t;

    typedef __PACKED_STRUCT IpcSocketGetSockOptParams_s {
      constexpr IpcSocketGetSockOptParams_s(void) noexcept
        : ipcParams(), getSockOptParams() {}
      constexpr explicit IpcSocketGetSockOptParams_s(std::int32_t socket, std::int32_t level, std::int32_t name) noexcept
        : ipcParams(), getSockOptParams(socket, level, name) {}
      CmdParams_t ipcParams;
      SocketGetSockOptParams_t getSockOptParams;
    } IpcSocketGetSockOptParams_t;

    typedef __PACKED_STRUCT SocketGetSockOptResponseParams_s {
      constexpr SocketGetSockOptResponseParams_s(void) noexcept : status(-1), length(0U), value{0} {}
      std::int32_t status;
      EmwAddress::SockLen_t length;
      std::uint8_t value[16];
    } SocketGetSockOptResponseParams_t;

    typedef __PACKED_STRUCT SocketBindParams_s {
      constexpr SocketBindParams_s(void) noexcept : socket(-1), addr(), length(0) {}
      std::int32_t socket;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
    } SocketBindParams_t;

    typedef __PACKED_STRUCT IpcSocketBindParams_s {
      constexpr IpcSocketBindParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_BIND_CMD), bindParams() {}
      CmdParams_t ipcParams;
      SocketBindParams_t bindParams;
    } IpcSocketBindParams_t;

    typedef __PACKED_STRUCT SocketBindResponseParams_s {
      constexpr SocketBindResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } SocketBindResponseParams_t;

    typedef __PACKED_STRUCT SocketConnectParams_s {
      constexpr SocketConnectParams_s(void) noexcept : socket(-1), addr(), length(0) {}
      std::int32_t socket;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
    } SocketConnectParams_t;

    typedef __PACKED_STRUCT IpcSocketConnectParams_s {
      constexpr IpcSocketConnectParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_CONNECT_CMD), connectParams() {}
      CmdParams_t ipcParams;
      SocketConnectParams_t connectParams;
    } IpcSocketConnectParams_t;

    typedef __PACKED_STRUCT SocketConnectResponseParams_s {
      constexpr SocketConnectResponseParams_s(void) noexcept : status(-1) {}
      int32_t status;
    } SocketConnectResponseParams_t;

    typedef __PACKED_STRUCT SocketShutDownParams_s {
      constexpr SocketShutDownParams_s(void) noexcept
        : filedes(-1), how(0) {}
      constexpr explicit SocketShutDownParams_s(std::int32_t filedes, std::int32_t how) noexcept
        : filedes(filedes), how(how) {}
      std::int32_t filedes;
      std::int32_t how;
    } SocketShutdownParams_t;

    typedef __PACKED_STRUCT IpcSocketShutDownParams_s {
      constexpr IpcSocketShutDownParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_SHUTDOWN_CMD), shutDownParams() {}
      constexpr explicit IpcSocketShutDownParams_s(std::int32_t filedes, std::int32_t how) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_SHUTDOWN_CMD), shutDownParams(filedes, how) {}
      CmdParams_t ipcParams;
      SocketShutdownParams_t shutDownParams;
    } IpcSocketShutDownParams_t;

    typedef __PACKED_STRUCT SocketShutDownResponseParams_s {
      constexpr SocketShutDownResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } SocketShutDownResponseParams_t;

    typedef __PACKED_STRUCT SocketCloseParams_s {
      constexpr SocketCloseParams_s(void) noexcept : filedes(-1) {}
      std::int32_t filedes;
    } SocketCloseParams_t;

    typedef __PACKED_STRUCT IpcSocketCloseParams_s {
      constexpr IpcSocketCloseParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_CLOSE_CMD), closeParams() {}
      CmdParams_t ipcParams;
      SocketCloseParams_t closeParams;
    } IpcSocketCloseParams_t;

    typedef __PACKED_STRUCT SocketCloseResponseParams_s {
      constexpr SocketCloseResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } SocketCloseResponseParams_t;

    typedef __PACKED_STRUCT SocketSendParams_s {
      constexpr SocketSendParams_s(void) noexcept : socket(-1), size(0), flags(0), buffer{0} {}
      std::int32_t socket;
      std::size_t size;
      std::int32_t flags;
      std::uint8_t buffer[1];
    } SocketSendParams_t;

    typedef __PACKED_STRUCT IpcSocketSendParams_s {
      constexpr IpcSocketSendParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_SEND_CMD), sendParams() {}
      CmdParams_t ipcParams;
      SocketSendParams_t sendParams;
    } IpcSocketSendParams_t;

    typedef __PACKED_STRUCT SocketSendResponseParams_s {
      constexpr SocketSendResponseParams_s(void) noexcept : sent(0) {}
      std::int32_t sent;
    } SocketSendResponseParams_t;

    typedef __PACKED_STRUCT SocketSendToParams_s {
      constexpr SocketSendToParams_s(void) noexcept : socket(-1), size(0), flags(0), addr(), length(0U), buffer{0U} {}
      std::int32_t socket;
      std::size_t size;
      std::int32_t flags;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
      std::uint8_t buffer[1];
    } SocketSendToParams_t;

    typedef __PACKED_STRUCT IpcSocketSendToParams_s {
      constexpr IpcSocketSendToParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_SENDTO_CMD), sendToParams() {}
      CmdParams_t ipcParams;
      SocketSendToParams_t sendToParams;
    } IpcSocketSendToParams_t;

    typedef __PACKED_STRUCT SocketSendToResponseParams_s {
      constexpr SocketSendToResponseParams_s(void) noexcept : sent(0) {}
      std::int32_t sent;
    } SocketSendToResponseParams_t;

    typedef __PACKED_STRUCT SocketReceiveParams_s {
      constexpr SocketReceiveParams_s(void) noexcept
        : socket(-1), size(0), flags(0) {}
      constexpr SocketReceiveParams_s(std::int32_t socket, std::size_t size, std::int32_t flags) noexcept
        : socket(socket), size(size), flags(flags) {}
      std::int32_t socket;
      std::size_t size;
      std::int32_t flags;
    } SocketReceiveParams_t;

    typedef __PACKED_STRUCT IpcSocketReceiveParams_s {
      constexpr IpcSocketReceiveParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_RECV_CMD), receiveParams() {}
      constexpr explicit IpcSocketReceiveParams_s(std::int32_t socket, std::size_t size, std::int32_t flags) noexcept
        : ipcParams(EmwCoreIpc::eSOCKET_RECV_CMD), receiveParams(socket, size, flags) {}
      CmdParams_t ipcParams;
      SocketReceiveParams_t receiveParams;
    } IpcSocketReceiveParams_t;

    typedef __PACKED_STRUCT SocketReceiveResponseParams_s {
      constexpr SocketReceiveResponseParams_s(void) noexcept : received(0), buffer{0} {}
      std::int32_t received;
      std::uint8_t buffer[1];
    } SocketReceiveResponseParams_t;

    typedef __PACKED_STRUCT SocketReceivefromParams_s {
      constexpr SocketReceivefromParams_s(void) noexcept : socket(-1), size(0), flags(0) {}
      std::int32_t socket;
      std::size_t size;
      std::int32_t flags;
    } SocketReceivefromParams_t;

    typedef __PACKED_STRUCT IpcSocketReceiveFromParams_s {
      constexpr IpcSocketReceiveFromParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_RECVFROM_CMD), receiveFromParams() {}
      CmdParams_t ipcParams;
      SocketReceivefromParams_t receiveFromParams;
    } IpcSocketReceiveFromParams_t;

    typedef __PACKED_STRUCT SocketReceiveFromResponseParams_s {
      constexpr SocketReceiveFromResponseParams_s(void) noexcept : received(0), addr(), length(0U), buffer{0U} {}
      std::int32_t received;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
      std::uint8_t buffer[1];
    } SocketReceiveFromResponseParams_t;

    typedef __PACKED_STRUCT SocketGetHostByNameParams_s {
      constexpr SocketGetHostByNameParams_s(void) noexcept : name{0} {}
      char name[253];
    } SocketGetHostByNameParams_t;

    typedef __PACKED_STRUCT IpcSocketGetHostByNameParams_s {
      constexpr IpcSocketGetHostByNameParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_GETHOSTBYNAME_CMD), getHostByNameParams() {}
      CmdParams_t ipcParams;
      SocketGetHostByNameParams_t getHostByNameParams;
    } IpcSocketGetHostByNameParams_t;

    typedef __PACKED_STRUCT SocketGetHostByNameResponseParams_s {
      constexpr SocketGetHostByNameResponseParams_s(void) noexcept : status(-1), s_addr(0U) {}
      std::int32_t status;
      std::uint32_t s_addr;
    } SocketGetHostByNameResponseParams_t;

    typedef __PACKED_STRUCT SocketGetAddrInfoParams_s {
      constexpr SocketGetAddrInfoParams_s(void) noexcept : nodeName{0}, serviceName{0} {}
      char nodeName[255 + 1];
      char serviceName[255 + 1];
      EmwAddress::AddrInfo_t hints;
    } SocketGetAddrInfoParams_t;

    typedef __PACKED_STRUCT IpcSocketGetAddrInfoParam_s {
      constexpr IpcSocketGetAddrInfoParam_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_GETADDRINFO_CMD), getAddrInfoParams() {}
      CmdParams_t ipcParams;
      SocketGetAddrInfoParams_t getAddrInfoParams;
    } IpcSocketGetAddrInfoParam_t;

    typedef __PACKED_STRUCT SocketGetAddrInfoResponseParam_s {
      constexpr SocketGetAddrInfoResponseParam_s() noexcept : status(-1), res() {}
      std::int32_t status;
      EmwAddress::AddrInfo_t res;
    } SocketGetAddrInfoResponseParam_t;

    typedef __PACKED_STRUCT SocketGetPeerNameParams_s {
      constexpr SocketGetPeerNameParams_s(void) noexcept : sockfd(-1) {}
      std::int32_t sockfd;
    } SocketGetPeerNameParams_t;

    typedef __PACKED_STRUCT IpcSocketGetPeerNameParams_s {
      constexpr IpcSocketGetPeerNameParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_GETPEERNAME_CMD), getPeerNameParams() {}
      CmdParams_t ipcParams;
      SocketGetPeerNameParams_t getPeerNameParams;
    } IpcSocketGetPeerNameParams_t;

    typedef __PACKED_STRUCT SocketGetPeerNameResponseParams_s {
      constexpr SocketGetPeerNameResponseParams_s(void) noexcept : status(-1), name(), nameLength(0U) {}
      std::int32_t status;
      EmwAddress::SockAddrStorage_t name;
      EmwAddress::SockLen_t nameLength;
    } SocketGetPeerNameResponseParams_t;

    typedef __PACKED_STRUCT SocketGetSockNameParams_s {
      constexpr SocketGetSockNameParams_s(void) noexcept : sockfd(-1) {}
      std::int32_t sockfd;
    } SocketGetSockNameParams_t;

    typedef __PACKED_STRUCT IpcSocketGetSockNameParams_s {
      constexpr IpcSocketGetSockNameParams_s(void) noexcept : ipcParams(EmwCoreIpc::eSOCKET_GETSOCKNAME_CMD), getSockNameParams() {}
      CmdParams_t ipcParams;
      SocketGetSockNameParams_t getSockNameParams;
    } IpcSocketGetSockNameParams_t;

    typedef __PACKED_STRUCT SocketGetSockNameResponseParams_s {
      constexpr SocketGetSockNameResponseParams_s(void) noexcept : status(-1), name(), nameLength(0U) {}
      std::int32_t status;
      EmwAddress::SockAddrStorage_t name;
      EmwAddress::SockLen_t nameLength;
    } SocketGetSockNameResponseParams_t;

    typedef __PACKED_STRUCT {
      std::int32_t socket;
      std::int32_t backlog;
    } SocketListenParams_t;

    typedef __PACKED_STRUCT {
      CmdParams_t ipcParams;
      SocketListenParams_t listenParams;
    } IpcSocketListenParams_t;

    typedef __PACKED_STRUCT {
      std::int32_t status;
    } SocketListenResponseParams_t;

    typedef __PACKED_STRUCT {
      std::int32_t socket;
    } SocketAcceptParams_t;

    typedef __PACKED_STRUCT {
      CmdParams_t ipcParams;
      SocketAcceptParams_t acceptParams;
    } IpcSocketAcceptParams_t;

    typedef __PACKED_STRUCT {
      std::int32_t socket;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
    } SocketAcceptResponseParams_t;

    typedef __PACKED_STRUCT TlsSetVersionParams_s {
      constexpr TlsSetVersionParams_s(void) noexcept
        : version(0U) {}
      constexpr explicit TlsSetVersionParams_s(std::uint8_t version) noexcept
        : version(version) {}
      std::uint8_t version;
    } TlsSetVersionParams_t;

    typedef __PACKED_STRUCT IpcTlsSetVersionParams_s {
      constexpr IpcTlsSetVersionParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eTLS_SET_VERSION_CMD), setVersionParams() {}
      constexpr explicit IpcTlsSetVersionParams_s(std::uint8_t version) noexcept
        : ipcParams(EmwCoreIpc::eTLS_SET_VERSION_CMD), setVersionParams(version) {}
      CmdParams_t ipcParams;
      TlsSetVersionParams_t setVersionParams;
    } IpcTlsSetVersionParams_t;

    typedef __PACKED_STRUCT TlsSetVersionResponseParams_s {
      constexpr TlsSetVersionResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } TlsSetVersionResponseParams_t;

    typedef __PACKED_STRUCT TlsSetClientCertificateParams_s {
      constexpr TlsSetClientCertificateParams_s(void) noexcept : certificatePemSize(0U), privateKeyPemSize(), certificateData{'\0'} {}
      std::uint16_t certificatePemSize;
      std::uint16_t privateKeyPemSize;
      char certificateData[1];
    } TlsSetClientCertificateParams_t;

    typedef __PACKED_STRUCT IpcTlsSetClientCertificateParams_s {
      constexpr IpcTlsSetClientCertificateParams_s(void) noexcept : ipcParams(EmwCoreIpc::eTLS_SET_CLIENT_CERTIFICATE_CMD), setClientCertificateParams() {}
      CmdParams_t ipcParams;
      TlsSetClientCertificateParams_t setClientCertificateParams;
    } IpcTlsSetClientCertificateParams_t;

    typedef __PACKED_STRUCT TlsSetClientCertificateResponseParams_s {
      constexpr TlsSetClientCertificateResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } TlsSetClientCertificateResponseParams_t;

    typedef __PACKED_STRUCT TlsSetServerCertificateParams_s {
      constexpr TlsSetServerCertificateParams_s(void) noexcept : certificatePemSize(0), privateKeyPemSize(0), verifyCaSize(0), certificateData{'\0'} {}
      std::uint16_t certificatePemSize;
      std::uint16_t privateKeyPemSize;
      std::uint16_t verifyCaSize;
      char certificateData[1];
    } TlsSetServerCertificateParams_t;

    typedef __PACKED_STRUCT IpcTlsSetServerCertificateParams_s {
      constexpr IpcTlsSetServerCertificateParams_s(void) noexcept : ipcParams(EmwCoreIpc::eTLS_SET_SERVER_CERTIFICATE_CMD), setServerCertificateParams() {}
      CmdParams_t ipcParams;
      TlsSetServerCertificateParams_t setServerCertificateParams;
    } IpcTlsSetServerCertificateParams_t;

    typedef __PACKED_STRUCT TlsSetServerCertificateResponseParams_s {
      constexpr TlsSetServerCertificateResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } TlsSetServerCertificateResponseParams_t;

    typedef __PACKED_STRUCT TlsAcceptParams_s {
      constexpr TlsAcceptParams_s(void) noexcept : fd(-1) {}
      std::int32_t fd;
    } TlsAcceptParams_t;

    typedef __PACKED_STRUCT TlsAcceptResponseParams_s {
      constexpr TlsAcceptResponseParams_s(void) noexcept : tlsPtr(nullptr) {}
      EmwApiBase::Mtls_t tlsPtr;
    } TlsAcceptResponseParams_t;

    typedef __PACKED_STRUCT TlsConnectSniParams_s {
      constexpr TlsConnectSniParams_s(void) noexcept : addr(), length(0), sniServerName{'\0'}, caLength(0), ca{'\0'} {}
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
      char sniServerName[128];
      std::int32_t caLength;
      char ca[1];
    } TlsConnectSniParams_t;

    typedef __PACKED_STRUCT IpcTlsConnectSniParams_s {
      constexpr IpcTlsConnectSniParams_s(void) noexcept : ipcParams(EmwCoreIpc::eTLS_CONNECT_SNI_CMD), connectSniParams() {}
      CmdParams_t ipcParams;
      TlsConnectSniParams_t connectSniParams;
    } IpcTlsConnectSniParams_t;

    typedef __PACKED_STRUCT TlsConnectSniResponseParams_s {
      constexpr TlsConnectSniResponseParams_s(void) noexcept : tlsPtr(nullptr), emwErrno(0) {}
      EmwApiBase::Mtls_t tlsPtr;
      std::int32_t emwErrno;
    } TlsConnectSniResponseParams_t;

    typedef __PACKED_STRUCT TlsSendParams_s {
      constexpr TlsSendParams_s(void) noexcept : tlsPtr(nullptr), size(0), buffer{0U} {}
      EmwApiBase::Mtls_t tlsPtr;
      std::size_t size;
      std::uint8_t buffer[1];
    } TlsSendParams_t;

    typedef __PACKED_STRUCT IpcTlsSendParams_s {
      constexpr IpcTlsSendParams_s(void) noexcept : ipcParams(EmwCoreIpc::eTLS_SEND_CMD), sendParams() {}
      CmdParams_t ipcParams;
      TlsSendParams_t sendParams;
    } IpcTlsSendParams_t;

    typedef __PACKED_STRUCT TlsSendResponseParams_s {
      constexpr TlsSendResponseParams_s(void) noexcept : sent(-1) {}
      std::int32_t sent;
    } TlsSendResponseParams_t;

    typedef __PACKED_STRUCT TlsReceiveParams_s {
      constexpr TlsReceiveParams_s(void) noexcept
        : tlsPtr(nullptr), size(0) {}
      constexpr explicit TlsReceiveParams_s(EmwApiBase::Mtls_t tlsPtr) noexcept
        : tlsPtr(tlsPtr), size(0) {}
      EmwApiBase::Mtls_t tlsPtr;
      std::size_t size;
    } TlsReceiveParams_t;

    typedef __PACKED_STRUCT IpcTlsReceiveParams_s {
      constexpr IpcTlsReceiveParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eTLS_RECEIVE_CMD), receiveParams() {}
      constexpr explicit IpcTlsReceiveParams_s(EmwApiBase::Mtls_t tlsPtr) noexcept
        : ipcParams(EmwCoreIpc::eTLS_RECEIVE_CMD), receiveParams(tlsPtr) {}
      CmdParams_t ipcParams;
      TlsReceiveParams_t receiveParams;
    } IpcTlsReceiveParams_t;

    typedef __PACKED_STRUCT TlsReceiveResponseParams_s {
      constexpr TlsReceiveResponseParams_s(void) noexcept : received(-1), buffer{0} {}
      std::int32_t received;
      std::uint8_t buffer[1];
    } TlsReceiveResponseParams_t;

    typedef __PACKED_STRUCT TlsCloseParams_s {
      constexpr TlsCloseParams_s(void) noexcept
        : tlsPtr(nullptr) {}
      constexpr explicit TlsCloseParams_s(EmwApiBase::Mtls_t tlsPtr) noexcept
        : tlsPtr(tlsPtr) {}
      EmwApiBase::Mtls_t tlsPtr;
    } TlsCloseParams_t;

    typedef __PACKED_STRUCT IpcTlsCloseParams_s {
      constexpr IpcTlsCloseParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eTLS_CLOSE_CMD), closeParams() {}
      constexpr explicit IpcTlsCloseParams_s(EmwApiBase::Mtls_t tlsPtr) noexcept
        : ipcParams(EmwCoreIpc::eTLS_CLOSE_CMD), closeParams(tlsPtr) {}
      CmdParams_t ipcParams;
      TlsCloseParams_t closeParams;
    } IpcTlsCloseParams_t;

    typedef __PACKED_STRUCT TlsCloseResponseParams_s {
      constexpr TlsCloseResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } TlsCloseResponseParams_t;

    typedef __PACKED_STRUCT TlsSetNonblockParams_s {
      constexpr TlsSetNonblockParams_s(void) noexcept
        : tlsPtr(nullptr), nonblock(-1) {}
      constexpr explicit TlsSetNonblockParams_s(EmwApiBase::Mtls_t tlsPtr, std::int32_t nonblock) noexcept
        : tlsPtr(tlsPtr), nonblock(nonblock) {}
      EmwApiBase::Mtls_t tlsPtr;
      std::int32_t nonblock;
    } TlsSetNonblockParams_t;

    typedef __PACKED_STRUCT IpcTlsSetNonblockParams_s {
      constexpr IpcTlsSetNonblockParams_s(void) noexcept
        : ipcParams(EmwCoreIpc::eTLS_SET_NONBLOCK_CMD), setNonblockParams() {}
      constexpr explicit IpcTlsSetNonblockParams_s(EmwApiBase::Mtls_t tlsPtr, std::int32_t nonblock) noexcept
        : ipcParams(EmwCoreIpc::eTLS_SET_NONBLOCK_CMD), setNonblockParams(tlsPtr, nonblock) {}
      CmdParams_t ipcParams;
      TlsSetNonblockParams_t setNonblockParams;
    } IpcTlsSetNonblockParams_t;

    typedef __PACKED_STRUCT TlsSetNonblockResponseParams_s {
      constexpr TlsSetNonblockResponseParams_s(void) noexcept : status(-1) {}
      std::int32_t status;
    } TlsSetNonblockResponseParams_t;
#endif /* EMW_NETWORK_EMW_MODE */

  private:
    typedef void (*EventCallback_t)(const EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr);

#define MIPC_API_EVENT_BASE (static_cast<std::uint16_t>(0x8000U))
  private:
    enum EventId : std::uint16_t {
      eSYS_EVENT_BASE = static_cast<std::uint16_t>(MIPC_API_EVENT_BASE + 0x0000U),
      eSYS_REBOOT_EVENT,
      eSYS_FOTA_STATUS_EVENT,
      eWIFI_EVENT_BASE = static_cast<std::uint16_t>(MIPC_API_EVENT_BASE + 0x0100U),
      eWIFI_STATUS_EVENT,
      eWIFI_BYPASS_INPUT_EVENT
    };
  private:
    typedef struct {
      EventId eventId;
      EventCallback_t callback;
    } EventItem_t;
  private:
    static void ProcessEvent(const EmwApiCore & core,
                             EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
  private:
    static void FotaStatusEventCallback(const EmwApiCore & core,
                                        EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
  private:
    static void RebootEventCallback(const EmwApiCore & core,
                                    EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
  private:
    static void WiFiStatusEventCallback(const EmwApiCore & core,
                                        EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
#if defined(EMW_NETWORK_BYPASS_MODE)
  private:
    static void WiFiNetlinkInputCallback(const EmwApiCore & core,
                                         EmwNetworkStack::Buffer_t *networkBufferPtr) noexcept;
#endif /* EMW_NETWORK_BYPASS_MODE */

  private:
    static EmwOsInterface::Mutex_t IpcLock;
  private:
    static bool IsPowerSaveEnabled;
  private:
    typedef struct {
      volatile /*_Atomic*/ std::uint32_t reqId;
      EmwOsInterface::Semaphore_t sem;
      std::uint16_t *responseSizePtr;
      std::uint8_t *responsePtr;
    } HciResponse_t;
  private:
    static HciResponse_t PendingRequest;
  private:
    static const std::uint16_t HEADER_SIZE = 6U;
  private:
    static const std::uint16_t PACKET_MIN_SIZE = EmwCoreIpc::HEADER_SIZE;
};
