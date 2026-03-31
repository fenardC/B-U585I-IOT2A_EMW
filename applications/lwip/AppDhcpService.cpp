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
#include "AppDhcpService.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip/debug.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

#define LWIP_LWIP_MAKEU32(a,b,c,d) \
  ((static_cast<u32_t>(((a) & 0xff) << 24)) | \
  (static_cast<u32_t>(((b) & 0xff) << 16)) | \
  (static_cast<u32_t>(((c) & 0xff) << 8))  | \
  static_cast<u32_t>(((d) & 0xff)))

AppDhcpService::AppDhcpService(struct netif *const lwipNetifPtr) noexcept
  : addressInfos()
  , addressInfosStored(0U)
    /* 192.168.1.1 = 0xC0A80101 */
  , hostIpAddress(lwip_htonl(lwipNetifPtr->ip_addr.u_addr.ip4.addr))
  , hostNamePtr("EMW-HotSpot")
  , lastOfferIpAddress(0U)
    /* 192.168.1.xx = 0xC0A801xx */
  , MINIMUM_IP_ADDRESS(hostIpAddress | AppDhcpService::MIN_IP)
    /* 192.168.1.yy = 0xC0A801yy */
  , MAXIMUM_IP_ADDRESS(hostIpAddress | AppDhcpService::MAX_IP)
  , serviceTaskRunning(false)
    /* 255.255.254.0 = 0xFFFFFE00 */
  , subNetworkMask(lwip_htonl(lwipNetifPtr->netmask.u_addr.ip4.addr))
  , netifPtr(lwipNetifPtr)
  , sock(-1)
{
  DEBUG_STD_PRINTF(" AppDhcpService::AppDhcpService()>\n")
  DEBUG_STD_PRINTF(" AppDhcpService::AppDhcpService()<\n\n")
}

#define BROADCAST_FLAG  (0x80U)

#define SDHCP_ASSERT(test)                                                                              \
  do {                                                                                                  \
    if (!(test)) {                                                                                      \
      static_cast<void>(std::printf(" SDHCP: failed (%" PRIu32 ")\n", static_cast<uint32_t>(__LINE__)));\
    }                                                                                                   \
  } while(false)

#define MAX_UDP_MESSAGE_SIZE  (sizeof(AppDhcpService::DhcpMessage_t) + sizeof(AppDhcpService::MAGIC_COOKIE) + 700)

std::int32_t AppDhcpService::createService(void) noexcept
{
  std::int32_t status;
  const std::int32_t PASS = 1; /* pdPASS */
  const char service_task_name[] = {"SDHCP-Service"};

  DEBUG_STD_PRINTF(" AppDhcpService::createService()>\n")

  this->serviceTaskRunning = true;
  status = xTaskCreate(AppDhcpService::ServiceTask, service_task_name,
                       AppDhcpService::SDHCP_TASK_STACK_SIZE, static_cast<void *>(this),
                       AppDhcpService::SDHCP_TASK_PRIORITY, NULL);
  if (PASS != status) {
    this->serviceTaskRunning = false;
    STD_PRINTF("\n SDHCP: failed to create service task %s\n", service_task_name);
  }
  else {
    status = 0;
    /* Be cooperative. */
    vTaskDelay(10U);
  }
  DEBUG_STD_PRINTF(" AppDhcpService::createService()<\n")
  return status;
}

std::int32_t AppDhcpService::deleteService(void) noexcept
{
  DEBUG_STD_PRINTF(" AppDhcpService::deleteService()>\n")
  this->serviceTaskRunning = false;
  DEBUG_STD_PRINTF(" AppDhcpService::deleteService()<\n")
  return 0;
}

bool AppDhcpService::CheckIpAddress(const AppDhcpService::DhcpAddress_t &addressInfo, const void *keyPtr) noexcept
{
  const std::uint32_t address = *(static_cast<const std::uint32_t *>(keyPtr));
  const bool status = (address == addressInfo.ipAddress);

  DEBUG_STD_PRINTF(" AppDhcpService::CheckIpAddress()< %" PRIx32 " -> %s\n",
                   addressInfo.ipAddress, (status) ? "true" : "false")
  return status;
}

bool AppDhcpService::CheckHardwareAddress(const AppDhcpService::DhcpAddress_t &addressInfo, const void *keyPtr) noexcept
{
  const AppDhcpService::ClientHardwareAddress_t *const client_ptr \
    = static_cast<const AppDhcpService::ClientHardwareAddress_t *>(keyPtr);

  SDHCP_ASSERT(nullptr != client_ptr);

  const bool status \
    = (0U != addressInfo.hardwareAddressSize) \
      && (client_ptr->size == addressInfo.hardwareAddressSize) \
      && (0 == std::memcmp(client_ptr->ptr, addressInfo.hardwareAddress,
                           client_ptr->size));
  DEBUG_STD_PRINTF(" AppDhcpService::CheckHardwareAddress()< %" PRIx32 " -> %s\n",
                   addressInfo.ipAddress, (status) ? "true" : "false")

  return status;
}

bool AppDhcpService::findOption(std::uint8_t option, const std::uint8_t *optionsPtr, std::int32_t optionsLength,
                                std::uint8_t *&dataPtr, uint32_t &dataLength) noexcept
{
  bool status_done = false;
  const std::uint8_t *current_ptr = optionsPtr;
  const std::uint8_t *end_ptr = optionsPtr + optionsLength;

  DEBUG_STD_PRINTF(" AppDhcpService::findOption()> %" PRIu32 "\n", static_cast<std::uint32_t>(option))

  SDHCP_ASSERT((0 == optionsLength) || (nullptr != optionsPtr));

  dataPtr = nullptr;
  dataLength = 0U;

  while (current_ptr < end_ptr) {
    const std::uint8_t current = *current_ptr++;

    if (AppDhcpService::ePAD == current) {
      continue;
    }
    if (AppDhcpService::eEND == current) {
      break;
    }
    if (end_ptr <= current_ptr) {
      STD_PRINTF("\n SDHCP: Invalid option data (not enough room for required length byte)\n");
      break;
    }
    {
      const std::uint8_t current_option_length = *current_ptr++;
      const std::uint8_t *current_option_ptr = current_ptr;

      // Validate option length fits inside buffer
      if (end_ptr < (current_option_ptr + current_option_length)) {
        STD_PRINTF("\n SDHCP: Invalid option length (overflow)\n");
        break;
      }
      if (option == current) {
        dataPtr = const_cast<std::uint8_t *>(current_option_ptr);
        dataLength = current_option_length;
        DEBUG_STD_PRINTF(" AppDhcpService::findOption(): %" PRIu32 " (%" PRIu32 ")\n",
                         static_cast<std::uint32_t>(option), static_cast<std::uint32_t>(dataLength))

        status_done = true;
        break;
      }
      current_ptr += current_option_length;
    }
  }
  return status_done;
}

bool AppDhcpService::getMessageType(const std::uint8_t *optionsPtr, std::int32_t optionsLength,
                                    AppDhcpService::DhcpMessageTypes &messageType) noexcept
{
  bool status_done = false;
  std::uint8_t *dhcp_message_ptr = nullptr;
  std::uint32_t dhcp_message_length = 0U;

  if (this->findOption(AppDhcpService::eDHCPMESSAGETYPE, optionsPtr, optionsLength,
                       dhcp_message_ptr, dhcp_message_length)) {
    const AppDhcpService::DhcpMessageTypes type = *(reinterpret_cast<AppDhcpService::DhcpMessageTypes *>(dhcp_message_ptr));

    if ((1 == dhcp_message_length) && (AppDhcpService::eTYPE_FIRST_VALUE <= type) \
        && (type <= AppDhcpService::eTYPE_LAST_VALUE)) {
      messageType = type;
      status_done = true;
    }
  }
  return status_done;
}

bool AppDhcpService::initializeService(void) noexcept
{
  bool status_done = false;

  this->sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (this->sock >= 0) {
    struct sockaddr_in socket_address_in {};

    socket_address_in.sin_len = sizeof(socket_address_in);
    socket_address_in.sin_family = AF_INET;
    socket_address_in.sin_addr.s_addr = lwip_htonl(this->hostIpAddress);
    socket_address_in.sin_port = lwip_htons(AppDhcpService::SDHCP_SERVER_PORT);
    {
      const struct ifreq iface \
          = {{this->netifPtr->name[0], this->netifPtr->name[1], static_cast<char>('0' + this->netifPtr->num), '\0', 0, 0}};
      static_cast<void>(lwip_setsockopt(this->sock, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(iface)));
    }
    if (-1 != lwip_bind(this->sock, reinterpret_cast<struct sockaddr *>(&socket_address_in), sizeof(socket_address_in))) {
      {
        std::int32_t broadcast = static_cast<std::int32_t>(true);
        if (0 == lwip_setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))) {
          status_done = true;
        }
        else {
          STD_PRINTF(" SDHCP: Unable to set socket options\n");
        }
      }
      {
        std::uint32_t timeout_in_ms = AppDhcpService::SDHCP_TIMEOUT_IN_MS;
        static_cast<void>(lwip_setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_in_ms, sizeof(timeout_in_ms)));
      }
    }
    else {
      STD_PRINTF(" SDHCP: Unable to bind to server socket (port %" PRIu32 ")\n",
                 static_cast<std::uint32_t>(AppDhcpService::SDHCP_SERVER_PORT));
    }
    if (!status_done) {
      static_cast<void>(lwip_close(this->sock));
    }
  }
  else {
    STD_PRINTF(" SDHCP: Unable to open server socket (port %" PRIu32 ")\n",
               static_cast<std::uint32_t>(AppDhcpService::SDHCP_SERVER_PORT));
  }
  return status_done;
}

std::int32_t AppDhcpService::handleDiscoverMessage(bool isAddressSeenBefore, std::uint32_t offerAddress,
    const std::uint8_t *clientHardwareAddressPtr, std::uint32_t clientHardwareAddressLength,
    AppDhcpService::DhcpMessage_t &reply, AppDhcpService::DhcpMessageTypes &replyOptionType) noexcept
{
  std::int32_t status = -1;
  std::uint32_t new_offer_addr;

  DEBUG_STD_PRINTF(" AppDhcpService::handleDiscoverMessage()>\n")

#define HARDWARE_ADDRESS_LENGTH sizeof((static_cast<AppDhcpService::DhcpAddress_s *>(nullptr))->hardwareAddress)

  if ((nullptr != clientHardwareAddressPtr) \
      && (0U != clientHardwareAddressLength) \
      && (HARDWARE_ADDRESS_LENGTH >= clientHardwareAddressLength)) {
    bool is_new_offer_addr_valid = false;

    if (isAddressSeenBefore) {
      new_offer_addr = offerAddress;
      is_new_offer_addr_valid = true;
    }
    else {
      const std::uint32_t pool_size = (this->MAXIMUM_IP_ADDRESS - this->MINIMUM_IP_ADDRESS) + 1;

      if ((this->MINIMUM_IP_ADDRESS > this->lastOfferIpAddress) || (this->MAXIMUM_IP_ADDRESS < this->lastOfferIpAddress)) {
        new_offer_addr = this->MINIMUM_IP_ADDRESS;
      }
      else if (this->MAXIMUM_IP_ADDRESS == this->lastOfferIpAddress) {
        new_offer_addr = this->MINIMUM_IP_ADDRESS;
      }
      else {
        new_offer_addr = this->lastOfferIpAddress + 1;
      }

      for (std::uint32_t i = 0U; i < pool_size; ++i) {
        if (-1 == AppDhcpService::FindIndex(this->addressInfos, this->addressInfosStored,
                                            AppDhcpService::CheckIpAddress, &new_offer_addr)) {
          is_new_offer_addr_valid = true;
          break;
        }

        if (this->MAXIMUM_IP_ADDRESS == new_offer_addr) {
          new_offer_addr = this->MINIMUM_IP_ADDRESS;
        }
        else {
          ++new_offer_addr;
        }
      }
    }
    if (is_new_offer_addr_valid) {
      AppDhcpService::DhcpAddress_t address_info;

      this->lastOfferIpAddress = new_offer_addr;
      address_info.ipAddress = new_offer_addr;
      std::memcpy(address_info.hardwareAddress, clientHardwareAddressPtr, clientHardwareAddressLength);
      address_info.hardwareAddressSize = clientHardwareAddressLength;

      if (isAddressSeenBefore || this->storeInfo(address_info)) {
        reply.yiaddr[0] = static_cast<std::uint8_t>((new_offer_addr & 0xFF000000U) >> 24); /* MSB First. */
        reply.yiaddr[1] = static_cast<std::uint8_t>((new_offer_addr & 0x00FF0000U) >> 16);
        reply.yiaddr[2] = static_cast<std::uint8_t>((new_offer_addr & 0x0000FF00U) >> 8);
        reply.yiaddr[3] = static_cast<std::uint8_t>((new_offer_addr & 0x000000FFU));
        replyOptionType = AppDhcpService::eTYPE_OFFER;
        status = 0;

        DEBUG_STD_PRINTF(" AppDhcpService::handleDiscoverMessage(): your IP address "
                         "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
                         static_cast<std::uint32_t>(reply.yiaddr[0]), static_cast<std::uint32_t>(reply.yiaddr[1]),
                         static_cast<std::uint32_t>(reply.yiaddr[2]), static_cast<std::uint32_t>(reply.yiaddr[3]))
      }
      else {
        STD_PRINTF("\n SDHCP: Insufficient memory to add client address");
      }
    }
    else {
      STD_PRINTF("\n SDHCP: No more IP addresses available for this client\n");
    }
  }
  DEBUG_STD_PRINTF(" AppDhcpService::handleDiscoverMessage()<\n")
  return status;
}

std::int32_t AppDhcpService::handleRequestMessage(uint32_t clientIpAddress,
    std::uint32_t requestedIpAddress, bool reqServerIdFound, std::uint32_t reqServer,
    bool isAddressSeenBefore, std::uint32_t offerAddress,
    AppDhcpService::DhcpMessage_t &reply, AppDhcpService::DhcpMessageTypes &replyOptionType) noexcept
{
  std::int32_t status = -1;
  AppDhcpService::DhcpMessageTypes reply_option_type = AppDhcpService::eTYPE_NAK;

  DEBUG_STD_PRINTF(" AppDhcpService::handleRequestMessage()>\n")

  if (reqServerIdFound) {
    if (reqServer == this->hostIpAddress) {
      SDHCP_ASSERT(clientIpAddress == 0U);

      if (isAddressSeenBefore) {
        reply_option_type = AppDhcpService::eTYPE_ACK;
      }
      else {
        reply_option_type = AppDhcpService::eTYPE_NAK;
      }
    }
    else {
      reply_option_type = AppDhcpService::eTYPE_NAK;
    }
  }
  else {
    if ((AppDhcpService::ADDRESS_BROADCAST != requestedIpAddress) || (0U != clientIpAddress)) {
      if (isAddressSeenBefore && ((offerAddress == requestedIpAddress) || (offerAddress == clientIpAddress))) {
        reply_option_type = AppDhcpService::eTYPE_ACK;
      }
    }
    else {
      STD_PRINTF("\nSDHCP: Invalid DHCP message (invalid data)\n");
    }
  }
  if (reply_option_type == AppDhcpService::eTYPE_ACK) {
    SDHCP_ASSERT(AppDhcpService::ADDRESS_BROADCAST != offerAddress);
    reply.ciaddr[0] = static_cast<std::uint8_t>((offerAddress & 0xFF000000U) >> 24); /* MSB First. */
    reply.ciaddr[1] = static_cast<std::uint8_t>((offerAddress & 0x00FF0000U) >> 16);
    reply.ciaddr[2] = static_cast<std::uint8_t>((offerAddress & 0x0000FF00U) >> 8);
    reply.ciaddr[3] = static_cast<std::uint8_t>(offerAddress & 0x000000FFU);
    reply.yiaddr[0] = static_cast<std::uint8_t>((offerAddress & 0xFF000000U) >> 24); /* MSB First. */
    reply.yiaddr[1] = static_cast<std::uint8_t>((offerAddress & 0x00FF0000U) >> 16);
    reply.yiaddr[2] = static_cast<std::uint8_t>((offerAddress & 0x0000FF00U) >> 8);
    reply.yiaddr[3] = static_cast<std::uint8_t>(offerAddress & 0x000000FFU);
    status = 0;
  }
  else if (reply_option_type == AppDhcpService::eTYPE_NAK) {
    status = 0;
  }
  else {
    /* Nothing to do*/
  }
  replyOptionType = reply_option_type;

  DEBUG_STD_PRINTF(" AppDhcpService::handleRequestMessage()<\n")

  return status;
}

void AppDhcpService::processRequest(const std::uint8_t requestData[], std::int32_t requestDataLength) noexcept
{
  const AppDhcpService::DhcpMessage_t *const request_ptr \
    = reinterpret_cast<const AppDhcpService::DhcpMessage_t *>(requestData);
  const std::int32_t request_minimal_size \
    = static_cast<std::int32_t>(sizeof(AppDhcpService::DhcpMessage_t) + sizeof(AppDhcpService::MAGIC_COOKIE));

  DEBUG_STD_PRINTF(" AppDhcpService::processRequest(): client address : %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32
                   "\n",
                   static_cast<std::uint32_t>(request_ptr->ciaddr[0]), static_cast<std::uint32_t>(request_ptr->ciaddr[1]),
                   static_cast<std::uint32_t>(request_ptr->ciaddr[2]), static_cast<std::uint32_t>(request_ptr->ciaddr[3]))

  DEBUG_STD_PRINTF(" AppDhcpService::processRequest(): your address   : %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32
                   "\n",
                   static_cast<std::uint32_t>(request_ptr->yiaddr[0]), static_cast<std::uint32_t>(request_ptr->yiaddr[1]),
                   static_cast<std::uint32_t>(request_ptr->yiaddr[2]), static_cast<std::uint32_t>(request_ptr->yiaddr[3]))

  DEBUG_STD_PRINTF(" AppDhcpService::processRequest(): gateway address: %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32
                   "\n",
                   static_cast<std::uint32_t>(request_ptr->giaddr[0]), static_cast<std::uint32_t>(request_ptr->giaddr[1]),
                   static_cast<std::uint32_t>(request_ptr->giaddr[2]), static_cast<std::uint32_t>(request_ptr->giaddr[3]))

  if ((request_minimal_size <= requestDataLength) && (AppDhcpService::BOOTREQUEST == request_ptr->op) \
      && (0 == std::memcmp(AppDhcpService::MAGIC_COOKIE, &request_ptr->file[128], sizeof(AppDhcpService::MAGIC_COOKIE)))) {
    const std::uint8_t *const request_options_ptr \
      = reinterpret_cast<const std::uint8_t *>(&request_ptr->file[128]) + sizeof(AppDhcpService::MAGIC_COOKIE);
    const std::int32_t request_options_size = static_cast<std::int32_t>(requestDataLength - request_minimal_size);
    AppDhcpService::DhcpMessageTypes client_message_type = static_cast<AppDhcpService::DhcpMessageTypes>(0);
    const std::uint32_t client_ip_address \
      = LWIP_LWIP_MAKEU32(request_ptr->ciaddr[0], request_ptr->ciaddr[1], request_ptr->ciaddr[2], request_ptr->ciaddr[3]);
    const std::uint32_t your_ip_address \
      = LWIP_LWIP_MAKEU32(request_ptr->yiaddr[0], request_ptr->yiaddr[1], request_ptr->yiaddr[2], request_ptr->yiaddr[3]);
    const std::uint32_t gateway_ip_address \
      = LWIP_LWIP_MAKEU32(request_ptr->giaddr[0], request_ptr->giaddr[1], request_ptr->giaddr[2], request_ptr->giaddr[3]);

    if (this->getMessageType(request_options_ptr, request_options_size, client_message_type)) {
      char host_name[AppDhcpService::MAX_HOSTNAME_LENGTH] = {0};
      {
        std::uint8_t *host_name_data_ptr = nullptr;
        std::uint32_t host_name_data_size = 0U;

        if (findOption(AppDhcpService::eHOSTNAME, request_options_ptr, request_options_size,
                       host_name_data_ptr, host_name_data_size)) {
          static_cast<void>(std::strncpy(host_name, reinterpret_cast<const char *>(host_name_data_ptr), sizeof(host_name) - 1));
          host_name[sizeof(host_name) - 1] = '\0';

          if (host_name_data_size < sizeof(host_name) - 1) {
            host_name[host_name_data_size] = '\0';
          }
        }
      }

      if (0 != std::strncmp(host_name, this->hostNamePtr, strlen(this->hostNamePtr))) {
        std::uint8_t reply_buffer[sizeof(AppDhcpService::DhcpMessage_t) + sizeof(AppDhcpService::DhcpOption_t)] = {0};
        AppDhcpService::DhcpMessage_t &reply = *(reinterpret_cast<AppDhcpService::DhcpMessage_t *>(&reply_buffer));
        bool is_addr_seen_before = false;
        bool is_reply_to_send = false;
        AppDhcpService::DhcpMessageTypes reply_option_type = AppDhcpService::eTYPE_OFFER;
        std::uint32_t offer_addr = AppDhcpService::ADDRESS_BROADCAST;
        std::uint8_t *client_hardware_address_ptr = nullptr;
        std::uint32_t client_hardware_address_length = 0U;

        DEBUG_STD_PRINTF("\n SDHCP: dealing with %" PRIX32 " %" PRIX32 " %" PRIX32 " %" PRIX32 " %" PRIX32 " %" PRIX32 "\n",
                         static_cast<std::uint32_t>(request_ptr->chaddr[0]), static_cast<std::uint32_t>(request_ptr->chaddr[1]),
                         static_cast<std::uint32_t>(request_ptr->chaddr[2]), static_cast<std::uint32_t>(request_ptr->chaddr[3]),
                         static_cast<std::uint32_t>(request_ptr->chaddr[4]), static_cast<std::uint32_t>(request_ptr->chaddr[5]))

        if (!findOption(AppDhcpService::eCLIENTIDENTIFIER, request_options_ptr, request_options_size,
                        client_hardware_address_ptr, client_hardware_address_length)) {
          client_hardware_address_ptr = const_cast<std::uint8_t *>(&request_ptr->chaddr[0]);
          client_hardware_address_length = sizeof(request_ptr->chaddr);
        }
        {
          AppDhcpService::ClientHardwareAddress_t client \
            = {client_hardware_address_ptr, static_cast<std::uint32_t>(client_hardware_address_length)};
          const int32_t index = AppDhcpService::FindIndex(this->addressInfos, this->addressInfosStored,
                                AppDhcpService::CheckHardwareAddress, &client);

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
        reply.giaddr[0] = request_ptr->giaddr[0];
        reply.giaddr[1] = request_ptr->giaddr[1];
        reply.giaddr[2] = request_ptr->giaddr[2];
        reply.giaddr[3] = request_ptr->giaddr[3];
        std::memcpy(reply.chaddr, request_ptr->chaddr, sizeof(reply.chaddr));
        std::strncpy(reply.sname, this->hostNamePtr, sizeof(reply.sname) - 1);
        reply.sname[sizeof(reply.sname) - 1] = '\0';

        switch (client_message_type) {
          case AppDhcpService::eTYPE_DISCOVER: {
              const int32_t status = this->handleDiscoverMessage(is_addr_seen_before, offer_addr,
                                     client_hardware_address_ptr, client_hardware_address_length, reply, reply_option_type);

              if ((status == 0) && (reply_option_type == AppDhcpService::eTYPE_OFFER)) {
                is_reply_to_send = true;

                DEBUG_STD_PRINTF("SDHCP: OFFERING client \"%s\" IP address %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
                                 host_name,
                                 static_cast<std::uint32_t>(reply.yiaddr[0]), static_cast<std::uint32_t>(reply.yiaddr[1]),
                                 static_cast<std::uint32_t>(reply.yiaddr[2]), static_cast<std::uint32_t>(reply.yiaddr[3]))
              }
              else {
                STD_PRINTF("\n SDHCP: No more IP addresses available for client \"%s\"", host_name);
              }
              break;
            }
          case AppDhcpService::eTYPE_REQUEST: {
              std::uint32_t requested_address = AppDhcpService::ADDRESS_BROADCAST;
              {
                std::uint8_t *address_ptr = nullptr;
                std::uint32_t address_size = 0U;

                if (findOption(AppDhcpService::eREQUESTEDIPADDRESS, request_options_ptr, request_options_size,
                               address_ptr, address_size)) {
                  if (sizeof(requested_address) == address_size) {
                    /* MSB first */
                    requested_address = LWIP_LWIP_MAKEU32(*(address_ptr), *(address_ptr + 1),
                                                          *(address_ptr + 2), *(address_ptr + 3));
                    DEBUG_STD_PRINTF(" SDHCP: REQUEST with client \"%s\" which has" \
                                     " IP address %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
                                     host_name,
                                     static_cast<std::uint32_t>((requested_address & 0xFF000000U) >> 24),
                                     static_cast<std::uint32_t>((requested_address & 0x00FF0000U) >> 16),
                                     static_cast<std::uint32_t>((requested_address & 0x0000FF00U) >> 8),
                                     static_cast<std::uint32_t>((requested_address & 0x000000FFU)))
                  }
                }
              }
              {
                std::int32_t status;
                std::uint32_t req_server = 0;
                bool server_id_found = false;
                {
                  std::uint8_t *server_id_ptr = nullptr;
                  std::uint32_t server_id_size = 0U;
                  server_id_found = findOption(AppDhcpService::eSERVERIDENTIFIER, request_options_ptr, request_options_size,
                                               server_id_ptr, server_id_size);
                  if (server_id_found && (sizeof(std::uint32_t) == server_id_size)) {
                    req_server = LWIP_LWIP_MAKEU32(*(server_id_ptr), *(server_id_ptr + 1),
                                                   *(server_id_ptr + 2), *(server_id_ptr + 3));
                  }
                }
                status = this->handleRequestMessage(client_ip_address, requested_address,
                                                    server_id_found, req_server,
                                                    is_addr_seen_before, offer_addr,
                                                    reply, reply_option_type);
                if ((status == 0) \
                    && ((reply_option_type == AppDhcpService::eTYPE_ACK) \
                        || (reply_option_type == AppDhcpService::eTYPE_NAK))) {
                  is_reply_to_send = true;
                  if (reply_option_type == AppDhcpService::eTYPE_ACK) {
                    DEBUG_STD_PRINTF(" SDHCP: ACKNOWLEDGING client \"%s\" " \
                                     "has IP address %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n", host_name,
                                     static_cast<std::uint32_t>(reply.ciaddr[0]), static_cast<std::uint32_t>(reply.ciaddr[1]),
                                     static_cast<std::uint32_t>(reply.ciaddr[2]), static_cast<std::uint32_t>(reply.ciaddr[3]))
                  }
                  if (reply_option_type == AppDhcpService::eTYPE_NAK) {
                    DEBUG_STD_PRINTF(" SDHCP: DENYING client \"%s\" due un-offered IP address.\n", host_name)
                  }
                }
                else {
                  STD_PRINTF(" SDHCP: invalid DHCP message (invalid data)\n");
                }
              }
              break;
            }
          case AppDhcpService::eTYPE_DECLINE:
          case AppDhcpService::eTYPE_RELEASE:
          case AppDhcpService::eTYPE_INFORM: {
              break;
            }
          case AppDhcpService::eTYPE_OFFER:
          case AppDhcpService::eTYPE_ACK:
          case AppDhcpService::eTYPE_NAK: {
              STD_PRINTF(" SDHCP: unexpected DHCP message type\n");
              break;
            }
          default: {
              STD_PRINTF(" SDHCP: invalid DHCP message type\n");
              break;
            }
        }
        if (is_reply_to_send) {
          AppDhcpService::DhcpOption_t &reply_option \
            = *(reinterpret_cast<AppDhcpService::DhcpOption_t *>(&reply_buffer[sizeof(AppDhcpService::DhcpMessage_t)]));

          this->setReplyOptions(reply_option, reply_option_type);
          this->sendReply(reply_buffer, sizeof(reply_buffer), reply_option_type,
                          gateway_ip_address, client_ip_address, your_ip_address, request_ptr->flags);
        }
      }
    }
  }
  DEBUG_STD_PRINTF(" AppDhcpService::processRequest()<\n")

}

void AppDhcpService::sendReply(std::uint8_t replyBuffer[], std::uint32_t replyBufferSize,
                               AppDhcpService::DhcpMessageTypes messageType, std::uint32_t gatewayIpAddress,
                               std::uint32_t clientIpAddress, std::uint32_t yourIpAddress, std::uint16_t requestedFlag) noexcept
{
  std::uint32_t peer_addr = AppDhcpService::ADDRESS_LOOPBACK;

  DEBUG_STD_PRINTF(" AppDhcpService::sendReply()> %" PRIu32 " %" PRIu32 " %" PRIu32 "\n\n", gatewayIpAddress,
                   clientIpAddress, yourIpAddress)

  if (0U == gatewayIpAddress) {
    switch (messageType) {
      case AppDhcpService::eTYPE_OFFER:
      case AppDhcpService::eTYPE_ACK: {
          if (0U == clientIpAddress) {
            if (0U != (BROADCAST_FLAG & requestedFlag)) {
              peer_addr = AppDhcpService::ADDRESS_BROADCAST;
            }
            else {
              peer_addr = yourIpAddress;
              if (0U == peer_addr) {
                peer_addr = AppDhcpService::ADDRESS_BROADCAST;
              }
            }
          }
          else {
            peer_addr = clientIpAddress;
          }
          break;
        }
      case AppDhcpService::eTYPE_NAK: {
          peer_addr = AppDhcpService::ADDRESS_BROADCAST;
          break;
        }
      case AppDhcpService::eTYPE_DISCOVER:
      case AppDhcpService::eTYPE_REQUEST:
      case AppDhcpService::eTYPE_DECLINE:
      case AppDhcpService::eTYPE_RELEASE:
      case AppDhcpService::eTYPE_INFORM: {
          break;
        }
      default: {
          STD_PRINTF("\n SDHCP: invalid DHCP message type");
          break;
        }
    }
  }
  else {
    peer_addr = gatewayIpAddress;
    replyBuffer[AppDhcpService::NET_DHCP_MSG_OFFSET_FLAGS] |= BROADCAST_FLAG;
  }

  SDHCP_ASSERT((AppDhcpService::ADDRESS_LOOPBACK != peer_addr) && (0U != peer_addr));
  {
    struct sockaddr_in sock_address_in {};

    sock_address_in.sin_len = sizeof(sock_address_in);
    sock_address_in.sin_family = AF_INET;
    sock_address_in.sin_addr.s_addr = lwip_htonl(peer_addr);
    sock_address_in.sin_port = lwip_htons(AppDhcpService::SDHCP_CLIENT_PORT);

    DEBUG_STD_PRINTF(" SDHCP: sending to %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n\n",
                     static_cast<std::uint32_t>((peer_addr & 0xFF000000U) >> 24),
                     static_cast<std::uint32_t>((peer_addr & 0x00FF0000U) >> 16),
                     static_cast<std::uint32_t>((peer_addr & 0x0000FF00U) >> 8), static_cast<std::uint32_t>(peer_addr & 0x000000FFU))

    {
      std::int32_t ret = lwip_sendto(this->sock, replyBuffer, replyBufferSize,
                                     0, reinterpret_cast<struct sockaddr *>(&sock_address_in), sizeof(sock_address_in));
      SDHCP_ASSERT(ret != -1);
    }
  }
}

void AppDhcpService::setReplyOptions(AppDhcpService::DhcpOption_t &replyServerOptions,
                                     AppDhcpService::DhcpMessageTypes replyServerOptionType) noexcept
{
  replyServerOptions.magic[0] = AppDhcpService::MAGIC_COOKIE[0];
  replyServerOptions.magic[1] = AppDhcpService::MAGIC_COOKIE[1];
  replyServerOptions.magic[2] = AppDhcpService::MAGIC_COOKIE[2];
  replyServerOptions.magic[3] = AppDhcpService::MAGIC_COOKIE[3];
  replyServerOptions.messageType[0] = AppDhcpService::eDHCPMESSAGETYPE;
  replyServerOptions.messageType[1] = 1;
  replyServerOptions.messageType[2] = replyServerOptionType;

  if (replyServerOptionType != AppDhcpService::eTYPE_NAK) {
    const std::uint32_t lease_time = 1 * 60 * 60;
    replyServerOptions.leaseTime[0] = AppDhcpService::eIPADDRESSLEASETIME;
    replyServerOptions.leaseTime[1] = 4U;
    replyServerOptions.leaseTime[2] = static_cast<std::uint8_t>((lease_time & 0xFF000000U) >> 24); /* MSB First. */
    replyServerOptions.leaseTime[3] = static_cast<std::uint8_t>((lease_time & 0x00FF0000U) >> 16);
    replyServerOptions.leaseTime[4] = static_cast<std::uint8_t>((lease_time & 0x0000FF00U) >> 8);
    replyServerOptions.leaseTime[5] = static_cast<std::uint8_t>(lease_time & 0x000000FFU);
    replyServerOptions.subNetworkMask[0] = AppDhcpService::eSUBNETMASK;
    replyServerOptions.subNetworkMask[1] = 4U;
    replyServerOptions.subNetworkMask[2] = static_cast<std::uint8_t>((this->subNetworkMask & 0xFF000000U) >> 24);
    replyServerOptions.subNetworkMask[3] = static_cast<std::uint8_t>((this->subNetworkMask & 0x00FF0000U) >> 16);
    replyServerOptions.subNetworkMask[4] = static_cast<std::uint8_t>((this->subNetworkMask & 0x0000FF00U) >> 8);
    replyServerOptions.subNetworkMask[5] = static_cast<std::uint8_t>(this->subNetworkMask & 0x000000FFU);
  }
  replyServerOptions.serverIdentifier[0] = AppDhcpService::eSERVERIDENTIFIER;
  replyServerOptions.serverIdentifier[1] = 4U;
  replyServerOptions.serverIdentifier[2] = static_cast<std::uint8_t>((this->hostIpAddress & 0xFF000000U) >> 24);
  replyServerOptions.serverIdentifier[3] = static_cast<std::uint8_t>((this->hostIpAddress & 0x00FF0000U) >> 16);
  replyServerOptions.serverIdentifier[4] = static_cast<std::uint8_t>((this->hostIpAddress & 0x0000FF00U) >> 8);
  replyServerOptions.serverIdentifier[5] = static_cast<std::uint8_t>(this->hostIpAddress & 0x000000FFU);
  replyServerOptions.router[0] = AppDhcpService::eROUTER;
  replyServerOptions.router[1] = 4U;
  replyServerOptions.router[2] = static_cast<std::uint8_t>((this->hostIpAddress & 0xFF000000U) >> 24);
  replyServerOptions.router[3] = static_cast<std::uint8_t>((this->hostIpAddress & 0x00FF0000U) >> 16);
  replyServerOptions.router[4] = static_cast<std::uint8_t>((this->hostIpAddress & 0x0000FF00U) >> 8);
  replyServerOptions.router[5] = static_cast<std::uint8_t>(this->hostIpAddress & 0x000000FFU);
  replyServerOptions.end = AppDhcpService::eEND;
}

void AppDhcpService::serviceTask(void) noexcept
{
  std::setbuf(stdout, nullptr);
  lwip_socket_thread_init();

  if (!this->initializeService()) {
    STD_PRINTF(" SDHCP: Unable to initialize the server.\n");
  }
  else {
    while (this->serviceTaskRunning) {
      std::uint8_t udp_buffer[MAX_UDP_MESSAGE_SIZE];
      struct sockaddr_in sock_address_in {};
      std::uint32_t sock_address_in_length = sizeof(sock_address_in);
      std::int32_t size;

      sock_address_in.sin_len = sizeof(sock_address_in);
      sock_address_in.sin_family = AF_INET;

      size = lwip_recvfrom(this->sock, udp_buffer, sizeof(udp_buffer), 0,
                           reinterpret_cast<struct sockaddr *>(&sock_address_in), &sock_address_in_length);
      if (size > 0) {
        this->processRequest(udp_buffer, size);
      }
    }
    STD_PRINTF("\n SDHCP: service is exiting\n");
    lwip_close(this->sock);
  }
  lwip_socket_thread_cleanup();
  vTaskDelete(NULL);
  for (;;);
}

bool AppDhcpService::storeInfo(const AppDhcpService::DhcpAddress_t &addressInfo) noexcept
{
  bool status_done = false;

  if (this->addressInfosStored < AppDhcpService::MAX_ADDRESS_INFO) {
    this->addressInfos[this->addressInfosStored] = addressInfo;
    this->addressInfosStored++;
    status_done = true;
  }
  DEBUG_STD_PRINTF(" AppDhcpService::storeInfo()< %" PRIx32 "\n", addressInfo.ipAddress)

  return status_done;
}

const std::uint8_t AppDhcpService::MAGIC_COOKIE[4] = {99U, 130U, 83U, 99U}; /* MSB first. */

std::int32_t AppDhcpService::FindIndex(const AppDhcpService::DhcpAddress_t addressInfosStorage[],
                                       std::uint32_t infosCount,
                                       AppDhcpService::FindFunction_t filterFunction, const void *keyPtr) noexcept
{
  std::int32_t index = -1;

  DEBUG_STD_PRINTF("\n\n AppDhcpService::FindIndex()>\n")

  for (std::int32_t i = 0; i < static_cast<std::int32_t>(infosCount); i++) {
    if (filterFunction(addressInfosStorage[i], keyPtr)) {
      index = i;
      break;
    }
  }
  DEBUG_STD_PRINTF(" AppDhcpService::FindIndex()< %" PRIi32 "\n\n", index)

  return index;
}
