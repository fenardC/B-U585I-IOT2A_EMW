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

#include <cstdint>
#include <cstddef>

#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif /* __PACKED_STRUCT */

class AppDhcpService final {
  public:
    explicit AppDhcpService(struct netif *const networkInterfacePtr) noexcept;
  public:
    ~AppDhcpService(void) noexcept {}
  public:
    std::int32_t createService(void) noexcept;
  public:
    std::int32_t deleteService(void) noexcept;

  private:
    typedef struct DhcpAddress_s {
      constexpr DhcpAddress_s(void) noexcept : ipAddress(0U), hardwareAddress{0}, hardwareAddressSize(0U) {}
      std::uint32_t ipAddress;
      std::uint8_t hardwareAddress[16];
      std::uint32_t hardwareAddressSize;
    } DhcpAddress_t;
  private:
    enum OptionValues : std::uint8_t {
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
    enum DhcpMessageTypes {
      eTYPE_DISCOVER = 1U,
      eTYPE_FIRST_VALUE = eTYPE_DISCOVER,
      eTYPE_OFFER = 2U,
      eTYPE_REQUEST = 3U,
      eTYPE_DECLINE = 4U,
      eTYPE_ACK = 5U,
      eTYPE_NAK = 6U,
      eTYPE_RELEASE = 7U,
      eTYPE_INFORM = 8U,
      eTYPE_LAST_VALUE = eTYPE_INFORM
    };
  private:
    typedef __PACKED_STRUCT {
      std::uint8_t op; /*  0 */
      std::uint8_t htype; /*  1 */
      std::uint8_t hlen; /*  2 */
      std::uint8_t hops; /*  3 */
      std::uint32_t xid; /*  4 */
      std::uint16_t secs; /*  8 */
      std::uint16_t flags; /* 10 */
      std::uint8_t ciaddr[4];
      std::uint8_t yiaddr[4];
      std::uint8_t siaddr[4];
      std::uint8_t giaddr[4];
      std::uint8_t chaddr[16];
      char sname[64];
      char file[128];
    } DhcpMessage_t;

  private:
    typedef __PACKED_STRUCT {
      std::uint8_t magic[4];
      std::uint8_t messageType[3];
      std::uint8_t leaseTime[6];
      std::uint8_t subNetworkMask[6];
      std::uint8_t serverIdentifier[6];
      std::uint8_t router[6];
      std::uint8_t end;
    } DhcpOption_t;

  private:
    typedef struct {
      std::uint8_t *ptr;
      std::uint32_t size;
    } ClientHardwareAddress_t;

  private:
    typedef bool (*FindFunction_t)(const AppDhcpService::DhcpAddress_t &addressInfo, const void *keyPtr);
  private:
    static bool CheckIpAddress(const AppDhcpService::DhcpAddress_t &addressInfo, const void *keyPtr) noexcept;
  private:
    static bool CheckHardwareAddress(const AppDhcpService::DhcpAddress_t &addressInfo, const void *keyPtr) noexcept;
  private:
    int32_t findIndex(const AppDhcpService::DhcpAddress_t addressInfos[], std::uint32_t infosCount,
                      AppDhcpService::FindFunction_t filterFunction, const void *keyPtr) noexcept;
  private:
    bool findOption(std::uint8_t option, const std::uint8_t *optionsPtr, int32_t optionsLength,
                    std::uint8_t *&dataPtr, std::uint32_t &dataLength) noexcept;
  private:
    bool getMessageType(const std::uint8_t *optionsPtr, std::int32_t optionsLength,
                        AppDhcpService::DhcpMessageTypes &messageType) noexcept;
  private:
    int32_t handleDiscoverMessage(bool isAddressSeenBefore, std::uint32_t offerAddress,
                                  std::uint8_t *serviceReqPtr, std::uint32_t serviceReqLength,
                                  AppDhcpService::DhcpMessage_t &reply, AppDhcpService::DhcpMessageTypes &replyOptionType) noexcept;
  private:
    int32_t handleRequestMessage(std::uint32_t clientIpAddress,
                                 std::uint32_t requestedIpAddress,
                                 bool reqServerIdFound, std::uint32_t reqServer,
                                 bool isAddressSeenBefore, std::uint32_t offerAddress,
                                 AppDhcpService::DhcpMessage_t &reply, AppDhcpService::DhcpMessageTypes &replyOptionType) noexcept;
  private:
    bool initializeService(void) noexcept;
  private:
    void processRequest(const std::uint8_t requestData[], std::int32_t requestDataLength) noexcept;
  private:
    void sendReply(std::uint8_t replyBuffer[], std::uint32_t replyBufferSize,
                   AppDhcpService::DhcpMessageTypes messageType, std::uint32_t gatewayIpAddress,
                   std::uint32_t clientIpAddress, std::uint32_t yourIpAddress, uint16_t requestedFlag) noexcept;
  private:
    void setReplyOptions(AppDhcpService::DhcpOption_t &replyServerOptions,
                         AppDhcpService::DhcpMessageTypes replyServerOptionType) noexcept;
  private:
    void serviceTask(void) noexcept;

  private:
    static void ServiceTask(void *THIS) noexcept
    {
      (reinterpret_cast<AppDhcpService *>(THIS))->serviceTask();
    }
  private:
    bool storeInfo(AppDhcpService::DhcpAddress_t &addressInfo) noexcept;

  private:
    static const std::uint32_t MIN_IP = 15U;
  private:
    static const std::uint32_t MAX_IP = 20U;
  private:
    static const std::uint32_t MAX_ADDRESS_INFO = (AppDhcpService::MAX_IP - AppDhcpService::MIN_IP);

  private:
    DhcpAddress_t addressInfos[AppDhcpService::MAX_ADDRESS_INFO];
  private:
    std:: uint32_t addressInfosStored;
  private:
    const std::uint32_t hostIpAddress;
  private:
    const char *hostNamePtr;
  private:
    std::uint32_t lastOfferIpAddress;
  private:
    const std::uint32_t minimumIpAddress;
  private:
    const std::uint32_t maximumIpAddress;
  private:
    volatile bool serviceTaskRunning = false;
  private:
    const std::uint32_t subNetworkMask;
  private:
    struct netif *const netifPtr;
  private:
    std::int32_t sock;

  private:
    static const std::uint8_t BOOTREQUEST = 1;
  private:
    static const std::uint8_t BOOTREPLY = 2;
  private:
    static const std::uint32_t ADDRESS_BROADCAST = 0xFFFFFFFF;
  private:
    static const std::uint32_t ADDRESS_LOOPBACK = 0x7F000001;
  private:
    static const std::size_t MAX_HOSTNAME_LENGTH = 64;
  private:
    static const std::uint8_t MAGIC_COOKIE[4];
  private:
    static const std::uint16_t NET_DHCP_MSG_OFFSET_FLAGS = 10;
  private:
    static const std::uint16_t SDHCP_CLIENT_PORT = 68;
  private:
    static const std::uint16_t SDHCP_SERVER_PORT = 67;
  private:
    static const int SDHCP_TASK_PRIORITY = 16;
  private:
    static const int SDHCP_TASK_STACK_SIZE = 2048;
  private:
    static const std::uint32_t SDHCP_TIMEOUT_IN_MS = 300U;
};
