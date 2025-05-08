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
#include "lwip/netif.h"
#include <cstdint>

class AppDhcpService final {
  public:
    explicit AppDhcpService(struct netif *const networkInterfacePtr);
  public:
    ~AppDhcpService(void) {}
  public:
    int32_t createService(void);
  public:
    int32_t deleteService(void);

  private:
    typedef struct DhcpAddress_s {
      DhcpAddress_s(void) : ipAddress(0U), clientIdentifierPtr(nullptr), clientIdentifierSize(0U) {}
      uint32_t ipAddress;
      uint8_t *clientIdentifierPtr;
      uint32_t clientIdentifierSize;
    } DhcpAddress_t;
  private:
    enum OptionValues_t {
      ePAD = 0,
      eSUBNETMASK = 1,
      eHOSTNAME = 12,
      eREQUESTEDIPADDRESS = 50,
      eIPADDRESSLEASETIME = 51,
      eDHCPMESSAGETYPE = 53,
      eSERVERIDENTIFIER = 54,
      eCLIENTIDENTIFIER = 61,
      eROUTER = 3,
      eEND = 255
    };
  private:
    enum DhcpMsgTypes_t {
      eDHCP_TYPE_DISCOVER = 1U,
      eDHCP_TYPE_FIRST_VALUE = eDHCP_TYPE_DISCOVER,
      eDHCP_TYPE_OFFER = 2U,
      eDHCP_TYPE_REQUEST = 3U,
      eDHCP_TYPE_DECLINE = 4U,
      eDHCP_TYPE_ACK = 5U,
      eDHCP_TYPE_NAK = 6U,
      eDHCP_TYPE_RELEASE = 7U,
      eDHCP_TYPE_INFORM = 8U,
      eDHCP_TYPE_LAST_VALUE  = eDHCP_TYPE_INFORM
    };
  private:
    typedef __PACKED_STRUCT {
      uint8_t op;            /*  0 */
      uint8_t htype;         /*  1 */
      uint8_t hlen;          /*  2 */
      uint8_t hops;          /*  3 */
      uint32_t xid;          /*  4 */
      uint16_t secs;         /*  8 */
      uint16_t flags;        /* 10 */
      uint8_t ciaddr[4];
      uint8_t yiaddr[4];
      uint8_t siaddr[4];
      uint8_t giaddr[4];
      uint8_t chaddr[16];
      char sname[64];
      char file[128];
      uint8_t options[1];
    } DhcpMsg_t;
  private:
    typedef __PACKED_STRUCT {
      uint8_t magic[4];
      uint8_t messageType[3];
      uint8_t leaseTime[6];
      uint8_t subNetworkMask[6];
      uint8_t serverIdentifier[6];
      uint8_t router[6];
      uint8_t end;
    } DhcpOption_t;
  private:
    typedef __PACKED_STRUCT {
      uint8_t *clientIdentifierPtr;
      uint32_t clientIdentifierSize;
    } SdhcpProcessRequest_t;

  private:
    typedef bool (*SdhcpFindCallback_t)(AppDhcpService::DhcpAddress_t *raiuiPtr, void *filterDataPtr);
  private:
    static bool addAddrFilter(AppDhcpService::DhcpAddress_t *infoPtr, void *const filterPtr);
  private:
    static bool checkAddrInUse(AppDhcpService::DhcpAddress_t *infoPtr, void *filterPtr);
  private:
    int32_t findIndex(AppDhcpService::DhcpAddress_t addressInfos[], uint32_t infosCount,
                      AppDhcpService::SdhcpFindCallback_t filterFunction, void *dataPtr);
  private:
    bool findOption(uint8_t option, const uint8_t *optionsPtr, int32_t optionsLength,
                    uint8_t *&dataPtr, uint32_t &dataLength);
  private:
    bool getMessageType(const uint8_t *optionsPtr, int32_t optionsLength,
                        AppDhcpService::DhcpMsgTypes_t &messageType);
  private:
    int32_t handleDiscoverMessage(bool isAddressSeenBefore, uint32_t offerAddress,
                                  uint8_t *serviceReqPtr, uint32_t serviceReq,
                                  AppDhcpService::DhcpMsg_t &reply, AppDhcpService::DhcpMsgTypes_t &replyOptionType);
  private:
    int32_t handleRequestMessage(uint32_t ciaddr,
                                 uint32_t requestedIpAddress,
                                 bool reqServerIdFound, uint32_t reqServer,
                                 bool isAddressSeenBefore, uint32_t offerAddress,
                                 AppDhcpService::DhcpMsg_t &reply, AppDhcpService::DhcpMsgTypes_t &replyOptionType);
  private:
    bool initializeService(void);
  private:
    void processRequest(uint8_t requestData[], int32_t requestDataLength);
  private:
    void sendReply(uint8_t reply_buffer[], uint32_t reply_buffer_size,
                   AppDhcpService::DhcpMsgTypes_t message_type, uint32_t giaddr,
                   uint32_t ciaddr, uint32_t yiaddr, uint16_t requested_flag);
  private:
    void setReplyOptions(AppDhcpService::DhcpOption_t &replyServerOptions,
                         AppDhcpService::DhcpMsgTypes_t replyServerOptionType);
  private:
    void serviceTask(void);

  private:
    static void serviceTask(void *THIS)
    {
      (reinterpret_cast<AppDhcpService *>(THIS))->serviceTask();
    }
  private:
    bool storeInfo(AppDhcpService::DhcpAddress_t *infoPtr);

  private:
    static const uint32_t MIN_IP = 15U;
  private:
    static const uint32_t MAX_IP = 20U;
  private:
    static const uint32_t MAX_ADDRESS_INFO = (AppDhcpService::MAX_IP - AppDhcpService::MIN_IP);

  private:
    DhcpAddress_t addressInfos[AppDhcpService::MAX_ADDRESS_INFO];
  private:
    uint32_t addressInfosStored;
  private:
    const uint32_t hostIpAddress;
  private:
    const char *hostNamePtr;
  private:
    uint32_t lastOfferIpAddress;
  private:
    const uint32_t minimumIpAddress;
  private:
    const uint32_t maximumIpAddress;
  private:
    volatile bool runTask = false;
  private:
    const uint32_t subNetworkMask;
  private:
    struct netif *const netifPtr;
  private:
    int32_t sock;

  private:
    static const uint8_t BOOTREQUEST = 1;
  private:
    static const uint8_t BOOTREPLY = 2;
  private:
    static const size_t MAX_HOSTNAME_LENGTH = 64;
  private:
    static const uint8_t MAGIC_COOKIE[4];
  private:
    static const uint16_t NET_DHCP_MSG_OFFSET_FLAGS = 10;
  private:
    static const uint16_t SDHCP_CLIENT_PORT = 68;
  private:
    static const uint16_t SDHCP_SERVER_PORT = 67;
  private:
    static const int SDHCP_TASK_PRIORITY = 16;
  private:
    static const int SDHCP_TASK_STACK_SIZE = 2048;
  private:
    static const uint32_t SDHCP_TIMEOUT_IN_MS = 300U;
};
