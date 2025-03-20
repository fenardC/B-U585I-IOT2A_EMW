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
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "cmsis_compiler.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */
#include "EmwApiBase.hpp"
#include "EmwOsInterface.hpp"
#include "EmwNetworkStack.hpp"
#if defined(EMW_NETWORK_EMW_MODE)
#include "EmwAddress.hpp"
#endif /* EMW_NETWORK_EMW_MODE */
#include <stdint.h>

class EmwApiCore;

#define EMW_API_VERSION ("2.4.0")

class EmwCoreIpc final {
  private:
    EmwCoreIpc(void) {};
  public:
    enum Status {
      eSUCCESS = (0),
      eERROR = (-1),
      eTIMEOUT = (-2),
      eNO_MEMORY = (-3)
    };

#define ID_NONE ((uint16_t)(0x0000U))
#define CMD_BASE ((uint16_t)(ID_NONE))
  public:
    enum ApiId {
      eSYS_CMD_BASE = (uint16_t)0U,
      eSYS_ECHO_CMD,
      eSYS_REBOOT_CMD,
      eSYS_VERSION_CMD,
      eSYS_RESET_CMD,
      eSYS_FOTA_START_CMD,
      eSYS_CFG_SERVER_START_CMD,
      eSYS_CFG_SERVER_STOP_CMD,
      eWIFI_CMD_BASE = (uint16_t)(CMD_BASE + 0x0100U),
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
      eSOCKET_CMD_BASE = (uint16_t)(CMD_BASE + 0x0200U),
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
      eSOCKET_GETADDRINFO_CMD
    };
  public:
    static void initialize(const class EmwApiCore *corePtr);
  public:
    static void poll(void *THIS, const class EmwApiCore *corePtr, uint32_t timeoutInMs);
  public:
    static Status request(const class EmwApiCore &core, uint8_t *commandPtr, uint16_t commandSize,
                          uint8_t *responseBufferPtr, uint16_t *responseBufferSize, uint32_t timeoutInMs);
  public:
    static Status testIpcEcho(const class EmwApiCore &core, uint8_t *dataInPtr, uint16_t dataInLength,
                              uint8_t *dataOutPtr, uint16_t *dataOutLengthPtr, uint32_t timeoutInMs);
  public:
    static void setApiId(uint8_t buffer[], uint16_t apiId);
  public:
    static uint8_t *startWithHeader(uint8_t *bytesPtr);
  public:
    static Status unInitialize(void);

  public:
    typedef __PACKED_STRUCT CmdParams_s {
      CmdParams_s(void)
        : reqId{0U, 0U, 0U, 0U}, apiId{0U, 0U} {}
      explicit CmdParams_s(uint16_t apiId)
        : reqId{0U, 0U, 0U, 0U}, apiId{(uint8_t)(apiId & 0x00FFU), (uint8_t)((apiId & 0xFF00U) >> 8)} {}
      uint8_t reqId[4];
      uint8_t apiId[2];
    } CmdParams_t;
    typedef __PACKED_STRUCT SysCommonResponseParams_s {
      SysCommonResponseParams_s(void): status(EmwCoreIpc::eERROR) {}
      int32_t status;
    } SysCommonResponseParams_t;
    typedef __PACKED_STRUCT SysFotaStartParams_s {
      SysFotaStartParams_s(void): url{0}, md5{0} {}
      char url[256];
      char md5[64];
    } SysFotaStartParams_t;
    typedef __PACKED_STRUCT IpcSysFotaStartParams_s {
      IpcSysFotaStartParams_s(void): ipcParams(EmwCoreIpc::eSYS_FOTA_START_CMD), fotaStartParams() {}
      CmdParams_t ipcParams;
      SysFotaStartParams_t fotaStartParams;
    } IpcSysFotaStartParams_t;
    typedef __PACKED_STRUCT IpcInterfaceParams_s {
      IpcInterfaceParams_s(void): ipcParams(EmwCoreIpc::eWIFI_GET_IP_CMD), interfaceNum(0U) {}
      CmdParams_t ipcParams;
      uint8_t interfaceNum;
    } IpcInterfaceParams_t;
    typedef __PACKED_STRUCT IpcAddressSlotInterfaceParams_s {
      IpcAddressSlotInterfaceParams_s(void): ipcParams(EmwCoreIpc::eWIFI_GET_IP6_STATE_CMD), addressSlotInterfaceNum{0U, 0U} {}
      CmdParams_t ipcParams;
      struct {
        uint8_t addressSlot;
        uint8_t interfaceNum;
      } addressSlotInterfaceNum;
    } IpcAddressSlotInterfaceParams_t;
    typedef __PACKED_STRUCT IpcNoParams_s {
      IpcNoParams_s(void)
        : ipcParams(), dataNull(0U) {}
      explicit IpcNoParams_s(EmwCoreIpc::ApiId apiId)
        : ipcParams(apiId), dataNull(0U) {}
      CmdParams_t ipcParams;
      uint8_t dataNull;
    } IpcNoParams_t;
    typedef __PACKED_STRUCT WiFiScanParams_s {
      WiFiScanParams_s(void): ssid{0} {}
      int8_t ssid[33];
    } WiFiScanParams_t;
    typedef __PACKED_STRUCT IpcWiFiScanParams_s {
      IpcWiFiScanParams_s(void): ipcParams(EmwCoreIpc::eWIFI_SCAN_CMD), scanParams() {}
      CmdParams_t ipcParams;
      WiFiScanParams_t scanParams;
    } IpcWiFiScanParams_t;
    typedef __PACKED_STRUCT WiFiScanResponseParams_s {
      WiFiScanResponseParams_s(void): numberOf(0U), ap{} {}
      uint8_t numberOf;
      EmwApiBase::ApInfo_t ap[1];
    } WiFiScanResponseParams_t;
    typedef __PACKED_STRUCT WiFiConnectParams_s {
      WiFiConnectParams_s(void): ssid{0}, key{0}, keyLength(0), useAttribute(0), useIp(0), attr(), ip() {}
      char ssid[33];
      char key[65];
      int32_t keyLength;
      uint8_t useAttribute;
      uint8_t useIp;
      EmwApiBase::ConnectAttributes_t attr;
      EmwApiBase::IpAttributes_t ip;
    } WiFiConnectParams_t;
    typedef __PACKED_STRUCT IpcWiFiConnectParams_s {
      IpcWiFiConnectParams_s(void): ipcParams(EmwCoreIpc::eWIFI_CONNECT_CMD), connectParams() {}
      CmdParams_t ipcParams;
      WiFiConnectParams_t connectParams;
    } IpcWiFiConnectParams_t;
    typedef __PACKED_STRUCT WiFiLinkInfo_s{
      WiFiLinkInfo_s(void): isConnected(0), rssi(0), ssid{0}, bssid{0}, key{0}, channel(0), security(0U) {}
      int32_t isConnected;
      int32_t rssi;
      int8_t ssid[33];
      uint8_t bssid[6];
      int8_t key[65];
      int32_t channel;
      EmwApiBase::Security_t security;
    } WiFiLinkInfo_t;
    typedef __PACKED_STRUCT WiFiGetLinkInfoResponseParams_s {
      WiFiGetLinkInfoResponseParams_s(void): status(EmwCoreIpc::eERROR), info() {}
      int32_t status;
      WiFiLinkInfo_t info;
    } WiFiGetLinkInfoResponseParams_t;
    typedef __PACKED_STRUCT WiFiGetIpResponseParams_s {
      WiFiGetIpResponseParams_s(void): status(EmwCoreIpc::eERROR), ip() {}
      int32_t status;
      EmwApiBase::IpAttributes_t ip;
    } WiFiGetIpResponseParams_t;
    typedef __PACKED_STRUCT WiFiSoftApStartParams_s {
      WiFiSoftApStartParams_s(void): ssid{0}, key{0}, channel(0), ip() {}
      int8_t ssid[32];
      int8_t key[64];
      int32_t channel;
      EmwApiBase::IpAttributes_t ip;
    } WiFiSoftApStartParams_t;
    typedef __PACKED_STRUCT IpcWiFiSoftApStartParams_s {
      IpcWiFiSoftApStartParams_s(void): ipcParams(EmwCoreIpc::eWIFI_SOFTAP_START_CMD), softApStartParams() {}
      CmdParams_t ipcParams;
      WiFiSoftApStartParams_t softApStartParams;
    } IpcWiFiSoftApStartParams_t;
    typedef __PACKED_STRUCT WiFiBypassSetParams_s {
      WiFiBypassSetParams_s(void): mode(0) {}
      int32_t mode;
    } WiFiBypassSetParams_t;
    typedef __PACKED_STRUCT IpcWiFiBypassSetParams_s {
      IpcWiFiBypassSetParams_s(void): ipcParams(EmwCoreIpc::eWIFI_BYPASS_SET_CMD), bypassSetParams() {}
      CmdParams_t ipcParams;
      WiFiBypassSetParams_t bypassSetParams;
    } IpcWiFiBypassSetParams_t;
    typedef __PACKED_STRUCT WiFiBypassGetResponseParams_s {
      WiFiBypassGetResponseParams_s(void): mode(0) {}
      int32_t mode;
    } WiFiBypassGetResponseParams_t;
    typedef __PACKED_STRUCT WiFiBypassOutParams_s {
      WiFiBypassOutParams_s(void): idx(0), useless{0U}, dataLength(0U) {}
      int32_t idx;
      uint8_t useless[16];
      uint16_t dataLength;
    } WiFiBypassOutParams_t;
    typedef __PACKED_STRUCT WiFiBypassInParams_s {
      WiFiBypassInParams_s(void): idx(0), useless{0U}, totalLength(0U) {}
      int32_t idx;
      uint8_t useless[16];
      uint16_t totalLength;
    } WiFiBypassInParams_t;
    enum {
      EAP_ROOTCA = 0x01,
      EAP_CLIENT_CERT,
      EAP_CLIENT_KEY,
      EAP_CERT_TYPE_MAX
    };
    typedef __PACKED_STRUCT WiFiEapSetCertParams_s {
      WiFiEapSetCertParams_s(void): type(0U), length(0U), cert{'\0'} {}
      uint8_t type;
      uint16_t length;
      char cert[1];
    } WiFiEapSetCertParams_t;
    typedef __PACKED_STRUCT IpcWiFiEapSetCertParams_s {
      IpcWiFiEapSetCertParams_s(void): ipcParams(EmwCoreIpc::eWIFI_EAP_SET_CERT_CMD), eapSetCertParams() {}
      CmdParams_t ipcParams;
      WiFiEapSetCertParams_t eapSetCertParams;
    } IpcWiFiEapSetCertParams_t;
    typedef __PACKED_STRUCT WiFiEapConnectParams_s {
      WiFiEapConnectParams_s(void): ssid{0}, identity{0}, password{0}, attrUsed(0U), attr(), ipUsed(0U), ip() {}
      char ssid[32];
      char identity[32];
      char password[64];
      uint8_t attrUsed;
      EmwApiBase::EapAttributes_t attr;
      uint8_t ipUsed;
      EmwApiBase::IpAttributes_t ip;
    } WiFiEapConnectParams_t;
    typedef __PACKED_STRUCT IpcWiFiEapConnectParams_s {
      IpcWiFiEapConnectParams_s(void): ipcParams(EmwCoreIpc::eWIFI_EAP_CONNECT_CMD), eapConnectParams() {}
      CmdParams_t ipcParams;
      WiFiEapConnectParams_t eapConnectParams;
    } IpcWiFiEapConnectParams_t;
    typedef __PACKED_STRUCT WiFiGetIp6StateParams_s {
      WiFiGetIp6StateParams_s(void): addressSlot(0U), interfaceNum(0U) {}
      uint8_t addressSlot;
      uint8_t interfaceNum;
    } WiFiGetIp6StateParams_t;
    typedef __PACKED_STRUCT WiFiGetIp6StateResponseParams_s {
      WiFiGetIp6StateResponseParams_s(void): state(0U) {}
      uint8_t state;
    } WiFiGetIp6StateResponseParams_t;
    typedef __PACKED_STRUCT WiFiGetIp6AddrParams_s {
      WiFiGetIp6AddrParams_s(void)
        : addressSlot(0U), interfaceNum(0U) {}
      WiFiGetIp6AddrParams_s(uint8_t addressSlot, uint8_t interfaceNum)
        : addressSlot(addressSlot), interfaceNum(interfaceNum) {}
      uint8_t addressSlot;
      uint8_t interfaceNum;
    } WiFiGetIp6AddrParams_t;
    typedef __PACKED_STRUCT IpcWiFiGetIp6AddrParams_s {
      IpcWiFiGetIp6AddrParams_s(void)
        : ipcParams(EmwCoreIpc::eWIFI_GET_IP6_ADDR_CMD), getIp6AddrParams() {}
      IpcWiFiGetIp6AddrParams_s(uint8_t addressSlot, uint8_t interfaceNum)
        : ipcParams(EmwCoreIpc::eWIFI_GET_IP6_ADDR_CMD), getIp6AddrParams(addressSlot, interfaceNum) {}
      CmdParams_t ipcParams;
      WiFiGetIp6AddrParams_t getIp6AddrParams;
    } IpcWiFiGetIp6AddrParams_t;
    typedef __PACKED_STRUCT WiFiGetIp6AddrResponseParams_s {
      WiFiGetIp6AddrResponseParams_s(void): status(EmwCoreIpc::eERROR), ip6{0U} {}
      int32_t status;
      uint8_t ip6[16];
    } WiFiGetIp6AddrResponseParams_t;

#if defined(EMW_NETWORK_EMW_MODE)
    typedef __PACKED_STRUCT WiFiPingParams_s {
      WiFiPingParams_s(void): hostname{0}, count(0), delayInMs(0) {}
      char hostname[255];
      int32_t count;
      int32_t delayInMs;
    } WiFiPingParams_t;
    typedef __PACKED_STRUCT IpcWiFiPingParams_s {
      IpcWiFiPingParams_s(uint16_t apiId): ipcParams(apiId), pingParams() {}
      CmdParams_t ipcParams;
      WiFiPingParams_t pingParams;
    } IpcWiFiPingParams_t;
    typedef __PACKED_STRUCT WiFiPingResponseParams_s {
      WiFiPingResponseParams_s(void): numberOf(0), delaysInMs{0} {}
      int32_t numberOf;
      int32_t delaysInMs[10];
    } WiFiPingResponseParams_t;
    typedef __PACKED_STRUCT SocketCreateParams_s {
      SocketCreateParams_s(void)
        : domain(0), type(0), protocol(0) {}
      SocketCreateParams_s(int32_t domain, int32_t type, int32_t protocol)
        : domain(domain), type(type), protocol(protocol) {}
      int32_t domain;
      int32_t type;
      int32_t protocol;
    } SocketCreateParams_t;
    typedef __PACKED_STRUCT IpcSocketCreateParams_s {
      IpcSocketCreateParams_s(void)
        : ipcParams(EmwCoreIpc::eSOCKET_CREATE_CMD), createParams() {}
      explicit IpcSocketCreateParams_s(int32_t domain, int32_t type, int32_t protocol)
        : ipcParams(EmwCoreIpc::eSOCKET_CREATE_CMD), createParams(domain, type, protocol) {}
      CmdParams_t ipcParams;
      SocketCreateParams_t createParams;
    } IpcSocketCreateParams_t;
    typedef __PACKED_STRUCT SocketCreateResponseParams_s {
      SocketCreateResponseParams_s(void): fd(-1) {}
      int32_t fd;
    } SocketCreateResponseParams_t;
    typedef __PACKED_STRUCT SocketSetSockOptParams_s {
      SocketSetSockOptParams_s(void): socket(-1), level(0), name(0), length(0), value{0} {}
      int32_t socket;
      int32_t level;
      int32_t name;
      EmwAddress::SockLen_t length;
      uint8_t value[16];
    } SocketSetSockOptParams_t;
    typedef __PACKED_STRUCT IpcSocketSetSockOptParams_s {
      IpcSocketSetSockOptParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_SETSOCKOPT_CMD), setSockOptParams() {}
      CmdParams_t ipcParams;
      SocketSetSockOptParams_t setSockOptParams;
    } IpcSocketSetSockOptParams_t;
    typedef __PACKED_STRUCT SocketSetSockOptResponseParams_s {
      SocketSetSockOptResponseParams_s(void): status(EmwCoreIpc::eERROR) {}
      int32_t status;
    } SocketSetSockOptResponseParams_t;
    typedef __PACKED_STRUCT SocketGetSockOptParams_s {
      SocketGetSockOptParams_s(void)
        : socket(-1), level(0), name(0) {}
      SocketGetSockOptParams_s(int32_t socket, int32_t level, int32_t name)
        : socket(socket), level(level), name(name) {}
      int32_t socket;
      int32_t level;
      int32_t name;
    } SocketGetSockOptParams_t;
    typedef __PACKED_STRUCT IpcSocketGetSockOptParams_s {
      IpcSocketGetSockOptParams_s(void)
        : ipcParams(), getSockOptParams() {}
      IpcSocketGetSockOptParams_s(int32_t socket, int32_t level, int32_t name)
        : ipcParams(), getSockOptParams(socket, level, name) {}
      CmdParams_t ipcParams;
      SocketGetSockOptParams_t getSockOptParams;
    } IpcSocketGetSockOptParams_t;
    typedef __PACKED_STRUCT SocketGetSockOptResponseParams_s {
      SocketGetSockOptResponseParams_s(void): status(EmwCoreIpc::eERROR), length(0U), value{0} {}
      int32_t status;
      EmwAddress::SockLen_t length;
      uint8_t value[16];
    } SocketGetSockOptResponseParams_t;
    typedef __PACKED_STRUCT SocketBindParams_s {
      SocketBindParams_s(void): socket(-1), addr(), length(0) {}
      int32_t socket;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
    } SocketBindParams_t;
    typedef __PACKED_STRUCT IpcSocketBindParams_s {
      IpcSocketBindParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_BIND_CMD), bindParams() {}
      CmdParams_t ipcParams;
      SocketBindParams_t bindParams;
    } IpcSocketBindParams_t;
    typedef __PACKED_STRUCT SocketBindResponseParams_s {
      SocketBindResponseParams_s(void): status(EmwCoreIpc::eERROR) {}
      int32_t status;
    } SocketBindResponseParams_t;
    typedef __PACKED_STRUCT SocketConnectParams_s {
      SocketConnectParams_s(void): socket(-1), addr(), length(0) {}
      int32_t socket;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
    } SocketConnectParams_t;
    typedef __PACKED_STRUCT IpcSocketConnectParams_s {
      IpcSocketConnectParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_CONNECT_CMD), connectParams() {}
      CmdParams_t ipcParams;
      SocketConnectParams_t connectParams;
    } IpcSocketConnectParams_t;
    typedef __PACKED_STRUCT SocketConnectResponseParams_s {
      SocketConnectResponseParams_s(void): status(EmwCoreIpc::eERROR) {}
      int32_t status;
    } SocketConnectResponseParams_t;
    typedef __PACKED_STRUCT SocketShutDownParams_s {
      SocketShutDownParams_s(void)
        : filedes(-1), how(0) {}
      explicit SocketShutDownParams_s(int32_t filedes, int32_t how)
        : filedes(filedes), how(how) {}
      int32_t filedes;
      int32_t how;
    } SocketShutdownParams_t;
    typedef __PACKED_STRUCT IpcSocketShutDownParams_s {
      IpcSocketShutDownParams_s(void)
        : ipcParams(EmwCoreIpc::eSOCKET_SHUTDOWN_CMD), shutDownParams() {}
      explicit IpcSocketShutDownParams_s(int32_t filedes, int32_t how)
        : ipcParams(EmwCoreIpc::eSOCKET_SHUTDOWN_CMD), shutDownParams(filedes, how) {}
      CmdParams_t ipcParams;
      SocketShutdownParams_t shutDownParams;
    } IpcSocketShutDownParams_t;
    typedef __PACKED_STRUCT SocketShutDownResponseParams_s {
      SocketShutDownResponseParams_s(void): status(EmwCoreIpc::eERROR) {}
      int32_t status;
    } SocketShutDownResponseParams_t;
    typedef __PACKED_STRUCT SocketCloseParams_s {
      SocketCloseParams_s(void): filedes(-1) {}
      int32_t filedes;
    } SocketCloseParams_t;
    typedef __PACKED_STRUCT IpcSocketCloseParams_s {
      IpcSocketCloseParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_CLOSE_CMD), closeParams() {}
      CmdParams_t ipcParams;
      SocketCloseParams_t closeParams;
    } IpcSocketCloseParams_t;
    typedef __PACKED_STRUCT SocketCloseResponseParams_s {
      SocketCloseResponseParams_s(void): status(EmwCoreIpc::eERROR) {}
      int32_t status;
    } SocketCloseResponseParams_t;
    typedef __PACKED_STRUCT SocketSendParams_s {
      SocketSendParams_s(void): socket(-1), size(0), flags(0), buffer{0} {}
      int32_t socket;
      size_t size;
      int32_t flags;
      uint8_t buffer[1];
    } SocketSendParams_t;
    typedef __PACKED_STRUCT IpcSocketSendParams_s {
      IpcSocketSendParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_SEND_CMD), sendParams() {}
      CmdParams_t ipcParams;
      SocketSendParams_t sendParams;
    } IpcSocketSendParams_t;
    typedef __PACKED_STRUCT SocketSendResponseParams_s {
      SocketSendResponseParams_s(void): sent(0) {}
      int32_t sent;
    } SocketSendResponseParams_t;
    typedef __PACKED_STRUCT SocketSendToParams_s {
      SocketSendToParams_s(void): socket(-1), size(0), flags(0), addr(), length(0U), buffer{0U} {}
      int32_t socket;
      size_t size;
      int32_t flags;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
      uint8_t buffer[1];
    } SocketSendToParams_t;
    typedef __PACKED_STRUCT IpcSocketSendToParams_s {
      IpcSocketSendToParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_SENDTO_CMD), sendToParams() {}
      CmdParams_t ipcParams;
      SocketSendToParams_t sendToParams;
    } IpcSocketSendToParams_t;
    typedef __PACKED_STRUCT SocketSendToResponseParams_s {
      SocketSendToResponseParams_s(void): sent(0) {}
      int32_t sent;
    } SocketSendToResponseParams_t;
    typedef __PACKED_STRUCT SocketReceiveParams_s {
      SocketReceiveParams_s(void)
        : socket(-1), size(0), flags(0) {}
      SocketReceiveParams_s(int32_t socket, size_t size, int32_t flags)
        : socket(socket), size(size), flags(flags) {}
      int32_t socket;
      size_t size;
      int32_t flags;
    } SocketReceiveParams_t;
    typedef __PACKED_STRUCT IpcSocketReceiveParams_s {
      IpcSocketReceiveParams_s(void)
        : ipcParams(EmwCoreIpc::eSOCKET_RECV_CMD), receiveParams() {}
      IpcSocketReceiveParams_s(int32_t socket, size_t size, int32_t flags)
        : ipcParams(EmwCoreIpc::eSOCKET_RECV_CMD), receiveParams(socket, size, flags) {}
      CmdParams_t ipcParams;
      SocketReceiveParams_t receiveParams;
    } IpcSocketReceiveParams_t;
    typedef __PACKED_STRUCT SocketReceiveResponseParams_s {
      SocketReceiveResponseParams_s(void): received(0), buffer{0} {}
      int32_t received;
      uint8_t buffer[1];
    } SocketReceiveResponseParams_t;
    typedef __PACKED_STRUCT SocketReceivefromParams_s {
      SocketReceivefromParams_s(void): socket(-1), size(0), flags(0) {}
      int32_t socket;
      size_t size;
      int32_t flags;
    } SocketReceivefromParams_t;
    typedef __PACKED_STRUCT IpcSocketReceiveFromParams_s {
      IpcSocketReceiveFromParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_RECVFROM_CMD), receiveFromParams() {}
      CmdParams_t ipcParams;
      SocketReceivefromParams_t receiveFromParams;
    } IpcSocketReceiveFromParams_t;
    typedef __PACKED_STRUCT SocketReceiveFromResponseParams_s {
      SocketReceiveFromResponseParams_s(void): received(0), addr(), length(0U), buffer{0U} {}
      int32_t received;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
      uint8_t buffer[1];
    } SocketReceiveFromResponseParams_t;
    typedef __PACKED_STRUCT SocketGetHostByNameParams_s {
      SocketGetHostByNameParams_s(void): name{0} {}
      char name[253];
    } SocketGetHostByNameParams_t;
    typedef __PACKED_STRUCT IpcSocketGetHostByNameParams_s {
      IpcSocketGetHostByNameParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_GETHOSTBYNAME_CMD), getHostByNameParams() {}
      CmdParams_t ipcParams;
      SocketGetHostByNameParams_t getHostByNameParams;
    } IpcSocketGetHostByNameParams_t;
    typedef __PACKED_STRUCT SocketGetHostByNameResponseParams_s {
      SocketGetHostByNameResponseParams_s(void): status(EmwCoreIpc::eERROR), s_addr(0U) {}
      int32_t status;
      uint32_t s_addr;
    } SocketGetHostByNameResponseParams_t;
    typedef __PACKED_STRUCT SocketGetAddrInfoParams_s {
      SocketGetAddrInfoParams_s(void): nodeName{0}, serviceName{0} {}
      char nodeName[255 + 1];
      char serviceName[255 + 1];
      EmwAddress::AddrInfo_t hints;
    } SocketGetAddrInfoParams_t;
    typedef __PACKED_STRUCT IpcSocketGetAddrInfoParam_s {
      IpcSocketGetAddrInfoParam_s(void): ipcParams(EmwCoreIpc::eSOCKET_GETADDRINFO_CMD), getAddrInfoParams() {}
      CmdParams_t ipcParams;
      SocketGetAddrInfoParams_t getAddrInfoParams;
    } IpcSocketGetAddrInfoParam_t;
    typedef __PACKED_STRUCT SocketGetAddrInfoResponseParam_s {
      SocketGetAddrInfoResponseParam_s(): status{EmwCoreIpc::eERROR}, res() {}
      int32_t status;
      EmwAddress::AddrInfo_t res;
    } SocketGetAddrInfoResponseParam_t;
    typedef __PACKED_STRUCT SocketGetPeerNameParams_s {
      SocketGetPeerNameParams_s(void): sockfd(-1) {}
      int32_t sockfd;
    } SocketGetPeerNameParams_t;
    typedef __PACKED_STRUCT IpcSocketGetPeerNameParams_s {
      IpcSocketGetPeerNameParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_GETPEERNAME_CMD), getPeerNameParams() {}
      CmdParams_t ipcParams;
      SocketGetPeerNameParams_t getPeerNameParams;
    } IpcSocketGetPeerNameParams_t;
    typedef __PACKED_STRUCT SocketGetPeerNameResponseParams_s {
      SocketGetPeerNameResponseParams_s(void): status(EmwCoreIpc::eERROR), name(), nameLength(0U) {}
      int32_t status;
      EmwAddress::SockAddrStorage_t name;
      EmwAddress::SockLen_t nameLength;
    } SocketGetPeerNameResponseParams_t;
    typedef __PACKED_STRUCT SocketGetSockNameParams_s {
      SocketGetSockNameParams_s(void): sockfd(-1) {}
      int32_t sockfd;
    } SocketGetSockNameParams_t;
    typedef __PACKED_STRUCT IpcSocketGetSockNameParams_s {
      IpcSocketGetSockNameParams_s(void): ipcParams(EmwCoreIpc::eSOCKET_GETSOCKNAME_CMD), getSockNameParams() {}
      CmdParams_t ipcParams;
      SocketGetSockNameParams_t getSockNameParams;
    } IpcSocketGetSockNameParams_t;
    typedef __PACKED_STRUCT SocketGetSockNameResponseParams_s {
      SocketGetSockNameResponseParams_s(void): status(EmwCoreIpc::eERROR), name(), nameLength(0U) {}
      int32_t status;
      EmwAddress::SockAddrStorage_t name;
      EmwAddress::SockLen_t nameLength;
    } SocketGetSockNameResponseParams_t;
    typedef __PACKED_STRUCT {
      int32_t socket;
      int32_t backlog;
    } SocketListenParams_t;
    typedef __PACKED_STRUCT {
      CmdParams_t ipcParams;
      SocketListenParams_t listenParams;
    } IpcSocketListenParams_t;
    typedef __PACKED_STRUCT {
      int32_t status;
    } SocketListenResponseParams_t;
    typedef __PACKED_STRUCT {
      int32_t socket;
    } SocketAcceptParams_t;
    typedef __PACKED_STRUCT {
      CmdParams_t ipcParams;
      SocketAcceptParams_t acceptParams;
    } IpcSocketAcceptParams_t;
    typedef __PACKED_STRUCT {
      int32_t socket;
      EmwAddress::SockAddrStorage_t addr;
      EmwAddress::SockLen_t length;
    } SocketAcceptResponseParams_t;
#endif /* EMW_NETWORK_EMW_MODE */

  private:
    typedef void (*EventCallback_t)(const class EmwApiCore &coreRef, EmwNetworkStack::Buffer_t *networkBufferPtr);

#define MIPC_API_EVENT_BASE ((uint16_t)(0x8000U))
  private:
    enum EventId {
      eSYS_EVENT_BASE = (uint16_t)(MIPC_API_EVENT_BASE + 0x0000U),
      eSYS_REBOOT_EVENT,
      eSYS_FOTA_STATUS_EVENT,
      eWIFI_EVENT_BASE = (uint16_t)(MIPC_API_EVENT_BASE + 0x0100U),
      eWIFI_STATUS_EVENT,
      eWIFI_BYPASS_INPUT_EVENT
    };
  private:
    typedef struct {
      EventId eventId;
      EventCallback_t callback;
    } EventItem_t;

  private:
    static uint16_t getApiId(const uint8_t buffer[]);
  private:
    static uint32_t getNewReqId(void);
  private:
    static uint32_t getReqId(const uint8_t buffer[]);
  private:
    static void processEvent(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr);
  private:
    static uint8_t *removeHeader(uint8_t *bytesPtr);
  private:
    static void setReqId(uint8_t buffer[], uint32_t reqId);

  private:
    static void fotaStatusEventCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr);
  private:
    static void rebootEventCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr);
  private:
    static void wiFiStatusEventCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr);
#if defined(EMW_NETWORK_BYPASS_MODE)
  private:
    static void wiFiNetlinkInputCallback(const class EmwApiCore &core, EmwNetworkStack::Buffer_t *networkBufferPtr);
#endif /* EMW_NETWORK_BYPASS_MODE */
  private:
    static EmwOsInterface::mutex_t IpcLock;
  private:
    static bool IsPowerSaveEnabled;
  private:
    typedef struct {
      volatile /*_Atomic*/ uint32_t reqId;
      EmwOsInterface::semaphore_t responseSem;
      uint16_t *responseBufferSizePtr;
      uint8_t *responseBufferPtr;
    } HciResponse_t;
  private:
    static HciResponse_t PendingRequest;
  private:
    static const uint16_t HEADER_SIZE = 6U;;
  private:
    static const uint16_t PACKET_MIN_SIZE = EmwCoreIpc::HEADER_SIZE;
  private:
    static const uint32_t REQ_ID_RESET_VAL = UINT32_MAX;
  private:
    static const uint32_t PACKET_REQ_ID_OFFSET = 0U;
  private:
    static const uint32_t PACKET_REQ_ID_SIZE = 4U;
  private:
    static const uint32_t PACKET_API_ID_OFFSET = EmwCoreIpc::PACKET_REQ_ID_OFFSET + EmwCoreIpc::PACKET_REQ_ID_SIZE;
  private:
    static const uint32_t PACKET_API_ID_SIZE = 2U;
  private:
    static const uint32_t PACKET_PARAMS_OFFSET = EmwCoreIpc::PACKET_API_ID_OFFSET + EmwCoreIpc::PACKET_API_ID_SIZE;
};
