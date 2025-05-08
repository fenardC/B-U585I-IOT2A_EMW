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
#include "AppDhcpService.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip/debug.h"
#include <cinttypes>
#include <cstdbool>
#include <cstring>
#include <cstdio>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

AppDhcpService::AppDhcpService(struct netif *const networkInterfacePtr)
  : addressInfos()
  , addressInfosStored(0U)
    /* 192.168.1.1 = 0xC0A80101 */
  , hostIpAddress(lwip_htonl(networkInterfacePtr->ip_addr.u_addr.ip4.addr))
  , hostNamePtr("ST-HotSpot")
  , lastOfferIpAddress(0U)
    /* 192.168.1.xx = 0xC0A801xx */
  , minimumIpAddress((lwip_htonl(networkInterfacePtr->ip_addr.u_addr.ip4.addr) & 0xFFFFFF00) |
                     AppDhcpService::MIN_IP)
    /* 192.168.1.yy = 0xC0A801yy */
  , maximumIpAddress((lwip_htonl(networkInterfacePtr->ip_addr.u_addr.ip4.addr) & 0xFFFFFF00) |
                     AppDhcpService::MAX_IP)
  , runTask(false)
    /* 255.255.254.0 = 0xFFFFFE00 */
  , subNetworkMask(lwip_htonl(networkInterfacePtr->netmask.u_addr.ip4.addr))
  , netifPtr(networkInterfacePtr)
  , sock(-1)
{
  STD_PRINTF("AppDhcpService::AppDhcpService()>\n")
  STD_PRINTF("AppDhcpService::AppDhcpService()<\n\n")
}

#define BROADCAST_FLAG        (0x80U)

#define SDHCP_ASSERT(test)                                                          \
  do {                                                                              \
    if (!(test)) {                                                                  \
      std::printf("SDHCP: failed (%" PRIu32 ")\n", static_cast<uint32_t>(__LINE__));\
    }                                                                               \
  } while(false)

#define MAX_UDP_MESSAGE_SIZE  (sizeof(AppDhcpService::DhcpMsg_t) + sizeof(AppDhcpService::MAGIC_COOKIE) + 700)

int32_t AppDhcpService::createService(void)
{
  int32_t status;
  const char service_task_name[] = {"SDHCP-Service"};

  STD_PRINTF("AppDhcpService::createService()>\n")

  this->runTask = true;
  status = xTaskCreate(AppDhcpService::serviceTask, service_task_name,
                       AppDhcpService::SDHCP_TASK_STACK_SIZE, static_cast<void *>(this),
                       AppDhcpService::SDHCP_TASK_PRIORITY, NULL);
  if (pdPASS != status) {
    this->runTask = false;
    std::printf("Failed to create task %s\n", service_task_name);
  }
  else {
    status = 0;
    /* Be cooperative. */
    vTaskDelay(10U);
  }
  STD_PRINTF("AppDhcpService::createService()<\n")
  return status;
}

int32_t AppDhcpService::deleteService(void)
{
  STD_PRINTF("AppDhcpService::deleteService()>\n")
  this->runTask = false;
  STD_PRINTF("AppDhcpService::deleteService()<\n")
  return 0;
}

bool AppDhcpService::addAddrFilter(AppDhcpService::DhcpAddress_t *infoPtr, void *const filterPtr)
{
  const uint32_t address = *(static_cast<uint32_t *>(filterPtr));
  return (address == infoPtr->ipAddress);
}

bool AppDhcpService::checkAddrInUse(AppDhcpService::DhcpAddress_t *infoPtr, void *filterPtr)
{
  AppDhcpService::SdhcpProcessRequest_t *const client_id_ptr \
    = static_cast<AppDhcpService::SdhcpProcessRequest_t *>(filterPtr);

  SDHCP_ASSERT(nullptr != client_id_ptr);

  bool status \
    = (0U != infoPtr->clientIdentifierSize) \
      && (client_id_ptr->clientIdentifierSize == infoPtr->clientIdentifierSize) \
      && (0 == memcmp(client_id_ptr->clientIdentifierPtr, infoPtr->clientIdentifierPtr, client_id_ptr->clientIdentifierSize));
  return status;
}

int32_t AppDhcpService::findIndex(AppDhcpService::DhcpAddress_t addressInfos[], uint32_t infosCount,
                                  AppDhcpService::SdhcpFindCallback_t filterFunction, void *dataPtr)
{
  int32_t index = -1;
  for (int32_t i = 0; i < static_cast<int32_t>(infosCount); i++) {
    if (filterFunction(&addressInfos[i], dataPtr)) {
      index = i;
      break;
    }
  }
  return index;
}

bool AppDhcpService::findOption(uint8_t option, const uint8_t *optionsPtr, int32_t optionsLength,
                                uint8_t *&dataPtr, uint32_t &dataLength)
{
  bool status_done = false;
  bool is_end_found = false;
  const uint8_t *current_ptr = optionsPtr;

  SDHCP_ASSERT(((0 == optionsLength) || (nullptr != optionsPtr)) \
               && (AppDhcpService::ePAD != option)
               && (AppDhcpService::eEND != option));

  while (((current_ptr - optionsPtr) < optionsLength) && !is_end_found && !status_done) {
    const uint8_t current = *current_ptr;
    if (AppDhcpService::ePAD == current) {
      current_ptr++;
    }
    else if (AppDhcpService::eEND == current) {
      is_end_found = true;
    }
    else {
      current_ptr++;
      if ((current_ptr - optionsPtr) < optionsLength) {
        const uint8_t current_option_len = *current_ptr;
        current_ptr++;
        if (option == current) {
          dataPtr = const_cast<uint8_t *>(current_ptr);
          dataLength = current_option_len;
          status_done = true;
        }
        current_ptr += current_option_len;
      }
      else {
        std::printf("Invalid option data (not enough room for required length byte).\n");
      }
    }
  }
  return status_done;
}

bool AppDhcpService::getMessageType(const uint8_t *optionsPtr, int32_t optionsLength,
                                    AppDhcpService::DhcpMsgTypes_t &messageType)
{
  bool status_done = false;
  uint8_t *dhcp_message_ptr = nullptr;
  uint32_t dhcp_message_length = 0;

  if (this->findOption(AppDhcpService::eDHCPMESSAGETYPE, optionsPtr, optionsLength,
                       dhcp_message_ptr, dhcp_message_length)) {
    const AppDhcpService::DhcpMsgTypes_t type = *(reinterpret_cast<AppDhcpService::DhcpMsgTypes_t *>(dhcp_message_ptr));
    if ((1 == dhcp_message_length) && (AppDhcpService::eDHCP_TYPE_FIRST_VALUE <= type) \
        && (type <= AppDhcpService::eDHCP_TYPE_LAST_VALUE)) {
      messageType = type;
      status_done = true;
    }
  }
  return status_done;
}

bool AppDhcpService::initializeService(void)
{
  bool status_done = false;

  this->sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (this->sock >= 0) {
    struct sockaddr_in s_addr_in = {sizeof(s_addr_in), AF_INET, 0, {0}, {0, 0, 0, 0, 0, 0, 0, 0}};

    s_addr_in.sin_addr.s_addr = lwip_htonl(this->hostIpAddress);
    s_addr_in.sin_port = lwip_htons(AppDhcpService::SDHCP_SERVER_PORT);
    {
      const struct ifreq iface \
        = {{this->netifPtr->name[0], this->netifPtr->name[1], static_cast<char>('0' + this->netifPtr->num), '\0', 0, 0}};
      lwip_setsockopt(this->sock, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(iface));
    }
    if (-1 != lwip_bind(this->sock, reinterpret_cast<struct sockaddr *>(&s_addr_in), sizeof(s_addr_in))) {
      {
        int32_t broadcast = static_cast<int32_t>(true);
        if (0 == lwip_setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))) {
          status_done = true;
        }
        else {
          std::printf("Unable to set socket options.\n");
        }
      }
      {
        uint32_t timeout_in_ms = AppDhcpService::SDHCP_TIMEOUT_IN_MS;
        lwip_setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
      }
    }
    else {
      std::printf("Unable to bind to server socket (port %" PRIu32 ").\n",
                  static_cast<uint32_t>(AppDhcpService::SDHCP_SERVER_PORT));
    }
    if (!status_done) {
      lwip_close(this->sock);
    }
  }
  else {
    std::printf("Unable to open server socket (port %" PRIu32 ").\n",
                static_cast<uint32_t>(AppDhcpService::SDHCP_SERVER_PORT));
  }
  return status_done;
}

int32_t AppDhcpService::handleDiscoverMessage(bool isAddressSeenBefore, uint32_t offerAddress,
    uint8_t *serviceReqPtr, uint32_t serviceReq,
    AppDhcpService::DhcpMsg_t &reply, AppDhcpService::DhcpMsgTypes_t &replyOptionType)
{
  int32_t status = -1;
  uint32_t new_offer_addr;
  bool is_new_offer_addr_valid = false;

  this->lastOfferIpAddress = this->maximumIpAddress;

  if (isAddressSeenBefore) {
    new_offer_addr = offerAddress;
    is_new_offer_addr_valid = true;
  }
  else {
    new_offer_addr = this->lastOfferIpAddress + 1;
  }
  {
    const uint32_t new_offer_addr_reference = new_offer_addr;
    bool offered_initial = false;

    while (!is_new_offer_addr_valid && !(offered_initial && (new_offer_addr_reference == new_offer_addr))) {
      if (this->maximumIpAddress < new_offer_addr) {
        SDHCP_ASSERT((this->maximumIpAddress + 1) == new_offer_addr);
        new_offer_addr = this->minimumIpAddress;

        is_new_offer_addr_valid = (-1 == findIndex(this->addressInfos, this->addressInfosStored,
                                   AppDhcpService::addAddrFilter, &new_offer_addr));
        offered_initial = true;
        if (!is_new_offer_addr_valid) {
          new_offer_addr++;
        }
      }
    }
    if (is_new_offer_addr_valid) {
      AppDhcpService::DhcpAddress_t info;

      this->lastOfferIpAddress = new_offer_addr;
      info.ipAddress = new_offer_addr;
      info.clientIdentifierPtr = static_cast<uint8_t *>(pvPortMalloc(serviceReq));

      if (nullptr != info.clientIdentifierPtr) {
        memcpy(info.clientIdentifierPtr, serviceReqPtr, serviceReq);
        info.clientIdentifierSize = serviceReq;
        if (isAddressSeenBefore || this->storeInfo(&info)) {
          reply.yiaddr[0] = static_cast<uint8_t>((new_offer_addr & 0xFF000000) >> 24); /* MSB First. */
          reply.yiaddr[1] = static_cast<uint8_t>((new_offer_addr & 0x00FF0000) >> 16);
          reply.yiaddr[2] = static_cast<uint8_t>((new_offer_addr & 0x0000FF00) >> 8);
          reply.yiaddr[3] = static_cast<uint8_t>((new_offer_addr & 0x000000FF));
          replyOptionType = AppDhcpService::eDHCP_TYPE_OFFER;
          status = 0;
        }
        else {
          vPortFree(info.clientIdentifierPtr);
          std::printf("Insufficient memory to add client address.");
        }
        if (isAddressSeenBefore) {
          vPortFree(info.clientIdentifierPtr);
        }
      }
      else {
        std::printf("Insufficient memory to add client address.");
      }
    }
    else {
      std::printf("No more IP addresses available for this client\n");
    }
  }
  return status;
}

int32_t AppDhcpService::handleRequestMessage(uint32_t ciaddr,
    uint32_t requestedIpAddress, bool reqServerIdFound, uint32_t reqServer,
    bool isAddressSeenBefore, uint32_t offerAddress,
    AppDhcpService::DhcpMsg_t &reply, AppDhcpService::DhcpMsgTypes_t &replyOptionType)
{
  int32_t status = -1;
  AppDhcpService::DhcpMsgTypes_t reply_option_type = AppDhcpService::eDHCP_TYPE_NAK;

  if (reqServerIdFound && (this->hostIpAddress == reqServer)) {
    SDHCP_ASSERT(0U == ciaddr);
    if (isAddressSeenBefore) {
      reply_option_type = AppDhcpService::eDHCP_TYPE_ACK;
    }
  }
  else {
    if ((INADDR_BROADCAST != requestedIpAddress) \
        || ((INADDR_BROADCAST == requestedIpAddress) && (0U != ciaddr))) {
      if (isAddressSeenBefore && ((offerAddress == requestedIpAddress) || (offerAddress == ciaddr))) {
        reply_option_type = AppDhcpService::eDHCP_TYPE_ACK;
      }
    }
    else {
      std::printf("Invalid DHCP message (invalid data)");
    }
  }
  if (reply_option_type == AppDhcpService::eDHCP_TYPE_ACK) {
    SDHCP_ASSERT(INADDR_BROADCAST != offerAddress);
    reply.ciaddr[0] = static_cast<uint8_t>((offerAddress & 0xFF000000) >> 24); /* MSB First. */
    reply.ciaddr[1] = static_cast<uint8_t>((offerAddress & 0x00FF0000) >> 16);
    reply.ciaddr[2] = static_cast<uint8_t>((offerAddress & 0x0000FF00) >> 8);
    reply.ciaddr[3] = static_cast<uint8_t>(offerAddress & 0x000000FF);
    reply.yiaddr[0] = static_cast<uint8_t>((offerAddress & 0xFF000000) >> 24); /* MSB First. */
    reply.yiaddr[1] = static_cast<uint8_t>((offerAddress & 0x00FF0000) >> 16);
    reply.yiaddr[2] = static_cast<uint8_t>((offerAddress & 0x0000FF00) >> 8);
    reply.yiaddr[3] = static_cast<uint8_t>(offerAddress & 0x000000FF);
    status = 0;
  }
  else if (reply_option_type == AppDhcpService::eDHCP_TYPE_NAK) {
    status = 0;
  }
  else {
    /* Nothing to do*/
  }
  replyOptionType = reply_option_type;
  return status;
}

void AppDhcpService::processRequest(uint8_t requestData[], int32_t requestDataLength)
{
  const AppDhcpService::DhcpMsg_t *const request_ptr = reinterpret_cast<AppDhcpService::DhcpMsg_t *>(requestData);
  const int32_t request_minimal_size = static_cast<int32_t>(sizeof(AppDhcpService::DhcpMsg_t) + sizeof(MAGIC_COOKIE));

  if ((request_minimal_size <= requestDataLength) && (AppDhcpService::BOOTREQUEST == request_ptr->op) \
      && (0 == memcmp(MAGIC_COOKIE, request_ptr->options, sizeof(AppDhcpService::MAGIC_COOKIE)))) {
    const uint8_t *const request_options_ptr = request_ptr->options + sizeof(AppDhcpService::MAGIC_COOKIE);
    const int32_t request_options_size = static_cast<int32_t>(requestDataLength - request_minimal_size);
    AppDhcpService::DhcpMsgTypes_t client_messsage_type = static_cast<AppDhcpService::DhcpMsgTypes_t>(0);
    const uint32_t ciaddr \
      = LWIP_MAKEU32(request_ptr->ciaddr[0], request_ptr->ciaddr[1], request_ptr->ciaddr[2], request_ptr->ciaddr[3]);
    const uint32_t yiaddr \
      = LWIP_MAKEU32(request_ptr->yiaddr[0], request_ptr->yiaddr[1], request_ptr->yiaddr[2], request_ptr->yiaddr[3]);
    const uint32_t giaddr \
      = LWIP_MAKEU32(request_ptr->giaddr[0], request_ptr->giaddr[1], request_ptr->giaddr[2], request_ptr->giaddr[3]);

    if (this->getMessageType(request_options_ptr, request_options_size, client_messsage_type)) {
      char host_name[AppDhcpService::MAX_HOSTNAME_LENGTH] = {0};
      {
        uint8_t *host_name_data_ptr = nullptr;
        uint32_t host_name_data_size = 0U;

        if (findOption(AppDhcpService::eHOSTNAME, request_options_ptr, request_options_size,
                       host_name_data_ptr, host_name_data_size)) {
          strncpy(host_name, reinterpret_cast<const char *>(host_name_data_ptr), sizeof(host_name) - 1);
          host_name[sizeof(host_name) - 1] = '\0';

          if (host_name_data_size < sizeof(host_name) - 1) {
            host_name[host_name_data_size] = '\0';
          }
        }
      }
      if ((nullptr != this->hostNamePtr) \
          && (strncmp(host_name, this->hostNamePtr, strlen(this->hostNamePtr)) != 0U)) {
        uint8_t reply_buffer[sizeof(AppDhcpService::DhcpMsg_t) + sizeof(AppDhcpService::DhcpOption_t)] = {0};
        AppDhcpService::DhcpMsg_t &reply = *(reinterpret_cast<AppDhcpService::DhcpMsg_t *>(&reply_buffer));
        bool is_addr_seen_before = false;
        bool is_reply_to_send = false;
        AppDhcpService::DhcpMsgTypes_t reply_option_type = AppDhcpService::eDHCP_TYPE_OFFER;
        uint32_t offer_addr = static_cast<uint32_t>(INADDR_BROADCAST);
        uint8_t *service_req_ptr = nullptr;
        uint32_t service_req = 0U;

        STD_PRINTF("Dealing with %" PRIX32 " %" PRIX32 " %" PRIX32 " %" PRIX32 " %" PRIX32 " %" PRIX32 "\n",
                   (uint32_t)request_ptr->chaddr[0], (uint32_t)request_ptr->chaddr[1], (uint32_t)request_ptr->chaddr[2],
                   (uint32_t)request_ptr->chaddr[3], (uint32_t)request_ptr->chaddr[4], (uint32_t)request_ptr->chaddr[5])

        if (!findOption(AppDhcpService::eCLIENTIDENTIFIER, request_options_ptr, request_options_size,
                        service_req_ptr, service_req)) {
          service_req_ptr = const_cast<uint8_t *>(&request_ptr->chaddr[0]);
          service_req = sizeof(request_ptr->chaddr);
        }
        {
          AppDhcpService::SdhcpProcessRequest_t cid = {service_req_ptr, static_cast<uint32_t>(service_req)};
          const int32_t index = findIndex(this->addressInfos, this->addressInfosStored, AppDhcpService::checkAddrInUse, &cid);

          if (-1 != index) {
            offer_addr = this->addressInfos[index].ipAddress;
            is_addr_seen_before = true;
          }
        }
        reply.op = AppDhcpService::BOOTREPLY;
        reply.htype = request_ptr->htype;
        reply.hlen = request_ptr->hlen;
        reply.xid = request_ptr->xid;
        reply.flags = request_ptr->flags;
        memcpy(reply.giaddr, request_ptr->giaddr, sizeof(reply.giaddr));
        memcpy(reply.chaddr, request_ptr->chaddr, sizeof(reply.chaddr));
        strncpy(reply.sname, this->hostNamePtr, sizeof(reply.sname) - 1);
        reply.sname[sizeof(reply.sname) - 1] = '\0';

        switch (client_messsage_type) {
          case AppDhcpService::eDHCP_TYPE_DISCOVER: {
              const int32_t status = this->handleDiscoverMessage(is_addr_seen_before, offer_addr,
                                     service_req_ptr, service_req, reply, reply_option_type);

              if ((status == 0) && (reply_option_type == AppDhcpService::eDHCP_TYPE_OFFER)) {
                is_reply_to_send = true;

                STD_PRINTF("OFFERING client \"%s\" IP address %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
                           host_name,
                           (uint32_t)reply->yiaddr[0], (uint32_t)reply->yiaddr[1],
                           (uint32_t)reply->yiaddr[2], (uint32_t)reply->yiaddr[3])
              }
              else {
                std::printf("No more IP addresses available for client \"%s\"", host_name);
              }
              break;
            }
          case AppDhcpService::eDHCP_TYPE_REQUEST: {
              uint32_t requested_address = static_cast<uint32_t>(INADDR_BROADCAST);
              {
                uint8_t *address_ptr = nullptr;
                uint32_t address_size = 0U;

                if (findOption(AppDhcpService::eREQUESTEDIPADDRESS, request_options_ptr, request_options_size,
                               address_ptr, address_size)) {
                  if (sizeof(requested_address) == address_size) {
                    /* MSB first */
                    requested_address = LWIP_MAKEU32(*address_ptr, *(address_ptr + 1),
                                                     *(address_ptr + 2), *(address_ptr + 3));
                    STD_PRINTF("REQUEST with client \"%s\" which has" \
                               " IP address %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
                               host_name,
                               (uint32_t)((requested_address & 0xFF000000) >> 24),
                               (uint32_t)((requested_address & 0x00FF0000) >> 16),
                               (uint32_t)((requested_address & 0x0000FF00) >> 8),
                               (uint32_t)(requested_address & 0x000000FF))
                  }
                }
              }
              {
                int32_t status;
                uint32_t req_server = 0;
                bool server_id_found = false;
                {
                  uint8_t *server_id_ptr = nullptr;
                  uint32_t server_id_size = 0U;
                  server_id_found = findOption(AppDhcpService::eSERVERIDENTIFIER, request_options_ptr, request_options_size,
                                               server_id_ptr, server_id_size);
                  if (server_id_found && (sizeof(uint32_t) == server_id_size)) {
                    req_server = LWIP_MAKEU32(*server_id_ptr, *(server_id_ptr + 1),
                                              *(server_id_ptr + 2), *(server_id_ptr + 3));
                  }
                }
                status = this->handleRequestMessage(ciaddr, requested_address,
                                                    server_id_found, req_server,
                                                    is_addr_seen_before, offer_addr,
                                                    reply, reply_option_type);
                if ((status == 0) \
                    && ((reply_option_type == AppDhcpService::eDHCP_TYPE_ACK) \
                        || (reply_option_type == AppDhcpService::eDHCP_TYPE_NAK))) {
                  is_reply_to_send = true;
                  if (reply_option_type == AppDhcpService::eDHCP_TYPE_ACK) {
                    STD_PRINTF("ACKNOWLEDGING client \"%s\" " \
                               "has IP address %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n", host_name,
                               (uint32_t)reply->ciaddr[0], (uint32_t)reply->ciaddr[1],
                               (uint32_t)reply->ciaddr[2], (uint32_t)reply->ciaddr[3])
                  }
                  if (reply_option_type == AppDhcpService::eDHCP_TYPE_NAK) {
                    STD_PRINTF("DENYING client \"%s\" due un-offered IP address.\n", host_name)
                  }
                }
                else {
                  STD_PRINTF("Invalid DHCP message (invalid data)\n")
                }
              }
              break;
            }
          case AppDhcpService::eDHCP_TYPE_DECLINE:
          case AppDhcpService::eDHCP_TYPE_RELEASE:
          case AppDhcpService::eDHCP_TYPE_INFORM: {
              break;
            }
          case AppDhcpService::eDHCP_TYPE_OFFER:
          case AppDhcpService::eDHCP_TYPE_ACK:
          case AppDhcpService::eDHCP_TYPE_NAK: {
              std::printf("Unexpected DHCP message type.\n");
              break;
            }
          default: {
              std::printf("Invalid DHCP message type.\n");
              break;
            }
        }
        if (is_reply_to_send) {
          this->setReplyOptions(*(reinterpret_cast<AppDhcpService::DhcpOption_t *>(&reply.options)), reply_option_type);
          this->sendReply(reply_buffer, sizeof(reply_buffer), reply_option_type, giaddr, ciaddr, yiaddr, request_ptr->flags);
        }
      }
    }
  }
}

void AppDhcpService::sendReply(uint8_t replyBuffer[], uint32_t replyBufferSize,
                               AppDhcpService::DhcpMsgTypes_t messageType, uint32_t giaddr,
                               uint32_t ciaddr, uint32_t yiaddr, uint16_t requestedFlag)
{
  uint32_t peer_addr = static_cast<uint32_t>(INADDR_LOOPBACK);

  if (0U == giaddr) {
    switch (messageType) {
      case AppDhcpService::eDHCP_TYPE_OFFER:
      case AppDhcpService::eDHCP_TYPE_ACK: {
          if (0U == ciaddr) {
            if (0U != (BROADCAST_FLAG & requestedFlag)) {
              peer_addr = INADDR_BROADCAST;
            }
            else {
              peer_addr = yiaddr;
              if (0U == peer_addr) {
                peer_addr = INADDR_BROADCAST;
              }
            }
          }
          else {
            peer_addr = ciaddr;
          }
          break;
        }
      case AppDhcpService::eDHCP_TYPE_NAK: {
          peer_addr = INADDR_BROADCAST;
          break;
        }
      case AppDhcpService::eDHCP_TYPE_DISCOVER:
      case AppDhcpService::eDHCP_TYPE_REQUEST:
      case AppDhcpService::eDHCP_TYPE_DECLINE:
      case AppDhcpService::eDHCP_TYPE_RELEASE:
      case AppDhcpService::eDHCP_TYPE_INFORM: {
          break;
        }
      default: {
          std::printf("Invalid DHCP message type");
          break;
        }
    }
  }
  else {
    peer_addr = giaddr;
    replyBuffer[AppDhcpService::NET_DHCP_MSG_OFFSET_FLAGS] |= BROADCAST_FLAG;
  }
  SDHCP_ASSERT((INADDR_LOOPBACK != peer_addr) && (0U != peer_addr));
  {
    struct sockaddr_in s_addr_in = {sizeof(s_addr_in), AF_INET, 0, {0}, {0, 0, 0, 0, 0, 0, 0, 0}};

    s_addr_in.sin_addr.s_addr = lwip_htonl(peer_addr);
    s_addr_in.sin_port = lwip_htons(AppDhcpService::SDHCP_CLIENT_PORT);
    STD_PRINTF("SDHCP: sending to %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n\n",
               (uint32_t)((peer_addr & 0xFF000000) >> 24), (uint32_t)((peer_addr & 0x00FF0000) >> 16),
               (uint32_t)((peer_addr & 0x0000FF00) >> 8), (uint32_t)(peer_addr & 0x000000FF)) {
      int32_t ret = lwip_sendto(this->sock, replyBuffer, replyBufferSize,
                                0, reinterpret_cast<struct sockaddr *>(&s_addr_in), sizeof(s_addr_in));
      SDHCP_ASSERT(ret != -1);
    }
  }
}

void AppDhcpService::setReplyOptions(AppDhcpService::DhcpOption_t &replyServerOptions,
                                     AppDhcpService::DhcpMsgTypes_t replyServerOptionType)
{
  replyServerOptions.magic[0] = AppDhcpService::MAGIC_COOKIE[0];
  replyServerOptions.magic[1] = AppDhcpService::MAGIC_COOKIE[1];
  replyServerOptions.magic[2] = AppDhcpService::MAGIC_COOKIE[2];
  replyServerOptions.magic[3] = AppDhcpService::MAGIC_COOKIE[3];
  replyServerOptions.messageType[0] = AppDhcpService::eDHCPMESSAGETYPE;
  replyServerOptions.messageType[1] = 1;
  replyServerOptions.messageType[2] = replyServerOptionType;
  if (replyServerOptionType != AppDhcpService::eDHCP_TYPE_NAK) {
    const uint32_t lease_time = 1 * 60 * 60;
    replyServerOptions.leaseTime[0] = AppDhcpService::eIPADDRESSLEASETIME;
    replyServerOptions.leaseTime[1] = 4U;
    replyServerOptions.leaseTime[2] = static_cast<uint8_t>((lease_time & 0xFF000000) >> 24); /* MSB First. */
    replyServerOptions.leaseTime[3] = static_cast<uint8_t>((lease_time & 0x00FF0000) >> 16);
    replyServerOptions.leaseTime[4] = static_cast<uint8_t>((lease_time & 0x0000FF00) >> 8);
    replyServerOptions.leaseTime[5] = static_cast<uint8_t>(lease_time & 0x000000FF);
    replyServerOptions.subNetworkMask[0] = AppDhcpService::eSUBNETMASK;
    replyServerOptions.subNetworkMask[1] = 4U;
    replyServerOptions.subNetworkMask[2] = static_cast<uint8_t>((this->subNetworkMask & 0xFF000000) >> 24);
    replyServerOptions.subNetworkMask[3] = static_cast<uint8_t>((this->subNetworkMask & 0x00FF0000) >> 16);
    replyServerOptions.subNetworkMask[4] = static_cast<uint8_t>((this->subNetworkMask & 0x0000FF00) >> 8);
    replyServerOptions.subNetworkMask[5] = static_cast<uint8_t>(this->subNetworkMask & 0x000000FF);
  }
  replyServerOptions.serverIdentifier[0] = AppDhcpService::eSERVERIDENTIFIER;
  replyServerOptions.serverIdentifier[1] = 4U;
  replyServerOptions.serverIdentifier[2] = static_cast<uint8_t>((this->hostIpAddress & 0xFF000000) >> 24);
  replyServerOptions.serverIdentifier[3] = static_cast<uint8_t>((this->hostIpAddress & 0x00FF0000) >> 16);
  replyServerOptions.serverIdentifier[4] = static_cast<uint8_t>((this->hostIpAddress & 0x0000FF00) >> 8);
  replyServerOptions.serverIdentifier[5] = static_cast<uint8_t>(this->hostIpAddress & 0x000000FF);
  replyServerOptions.router[0] = AppDhcpService::eROUTER;
  replyServerOptions.router[1] = 4U;
  replyServerOptions.router[2] = static_cast<uint8_t>((this->hostIpAddress & 0xFF000000) >> 24);
  replyServerOptions.router[3] = static_cast<uint8_t>((this->hostIpAddress & 0x00FF0000) >> 16);
  replyServerOptions.router[4] = static_cast<uint8_t>((this->hostIpAddress & 0x0000FF00) >> 8);
  replyServerOptions.router[5] = static_cast<uint8_t>(this->hostIpAddress & 0x000000FF);
  replyServerOptions.end = AppDhcpService::eEND;
}

void AppDhcpService::serviceTask(void)
{
  setbuf(stdout, NULL);

  if (!this->initializeService()) {
    std::printf("Unable to initialize the DHCP server.\n");
  }
  else {
    while (this->runTask) {
      uint8_t udp_buffer[MAX_UDP_MESSAGE_SIZE];
      struct sockaddr_in s_addr_in = {sizeof(s_addr_in), AF_INET, 0, {0}, {0, 0, 0, 0, 0, 0, 0, 0}};
      uint32_t s_addr_in_len = sizeof(s_addr_in);
      int32_t size = lwip_recvfrom(this->sock, udp_buffer, sizeof(udp_buffer), 0,
                                   reinterpret_cast<struct sockaddr *>(&s_addr_in), &s_addr_in_len);
      if (size > 0) {
        this->processRequest(udp_buffer, size);
      }
    }
    STD_PRINTF("service exit.\n")

    lwip_close(this->sock);
    for (auto i = 0U; i < this->addressInfosStored; i++) {
      vPortFree(this->addressInfos[i].clientIdentifierPtr);
    }
  }
  vTaskDelete(NULL);
  for (;;);
}

bool AppDhcpService::storeInfo(AppDhcpService::DhcpAddress_t *infoPtr)
{
  bool status_done = false;
  if (this->addressInfosStored < AppDhcpService::MAX_ADDRESS_INFO) {
    this->addressInfos[this->addressInfosStored] = *infoPtr;
    this->addressInfosStored++;
    status_done = true;
  }
  return status_done;
}

const uint8_t AppDhcpService::MAGIC_COOKIE[4] = { 99, 130, 83, 99 }; /* MSB first. */
