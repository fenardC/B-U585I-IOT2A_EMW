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
#include "WiFiNetworkSoftAp.hpp"
#include "EmwApiEmwBypass.hpp"
#include "EmwNetworkStack.hpp"
#include "emw_conf.hpp"
#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS */
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <memory>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)
//#define ENABLE_DEBUG

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

#define LWIP_IP4_ADDR_GET_BYTE_VAL(ipaddr, idx) (static_cast<u8_t>(((ipaddr).addr >> (idx * 8)) & 0xff))

#define LWIP_IP4_ADDR_ISANY_VAL(addr1)   ((addr1).addr == 0x00000000U)
#define LWIP_IP_ADDR_ISANY_VAL(ipaddr) \
  ((IP_IS_V6_VAL(ipaddr)) ? \
  ip6_addr_isany_val(*ip_2_ip6(&(ipaddr))) : \
  LWIP_IP4_ADDR_ISANY_VAL(*ip_2_ip4(&(ipaddr))))

#define LWIP_IP4_ADDR1_VAL(ipaddr) LWIP_IP4_ADDR_GET_BYTE_VAL(ipaddr, 0)
#define LWIP_IP4_ADDR2_VAL(ipaddr) LWIP_IP4_ADDR_GET_BYTE_VAL(ipaddr, 1)
#define LWIP_IP4_ADDR3_VAL(ipaddr) LWIP_IP4_ADDR_GET_BYTE_VAL(ipaddr, 2)
#define LWIP_IP4_ADDR4_VAL(ipaddr) LWIP_IP4_ADDR_GET_BYTE_VAL(ipaddr, 3)

#define LWIP_NETIF_IS_LINK_UP(netif) \
  (((netif)->flags & NETIF_FLAG_LINK_UP) ? static_cast<u8_t>(1) : static_cast<u8_t>(0))

#define LWIP_NETIF_IS_UP(netif) \
  (((netif)->flags & NETIF_FLAG_UP) ? static_cast<u8_t>(1) : static_cast<u8_t>(0))


static bool CheckTimeout(std::uint32_t tickStart, std::uint32_t tickCount);
#if defined(ENABLE_DEBUG)
static const char *NetifReasonToString(netif_nsc_reason_t reason);
#endif /* ENABLE_DEBUG */


err_t WiFiNetworkSoftAp::InitializeNetif(struct netif *netifPtr) noexcept
{
  err_t status = static_cast<err_t>(ERR_ARG);

  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::InitializeNetif()>\n")

  netif_add_ext_callback(&WiFiNetworkSoftAp::NetifExtCallback, WiFiNetworkSoftAp::NetifExtCallbackFunction);

  if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
    netifPtr->output = etharp_output;
    netifPtr->output_ip6 = ethip6_output;
    netifPtr->ip6_autoconfig_enabled = 1;
    netifPtr->linkoutput = &WiFiNetworkSoftAp::OutputToTransmitFifo;
    netifPtr->mtu = EmwNetworkStack::NETWORK_MTU_SIZE;
    netifPtr->flags = static_cast<u8_t>(NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET);
    netif_set_status_callback(netifPtr, &WiFiNetworkSoftAp::NetifStatusCallback);
    netif_set_link_callback(netifPtr, &WiFiNetworkSoftAp::NetifStatusCallback);
    status = static_cast<err_t>(ERR_OK);
  }
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::InitializeNetif()<\n")
  return status;
}

void WiFiNetworkSoftAp::UnInitializeNetif(struct netif *netifPtr) noexcept
{
  static_cast<void>(netifPtr);

  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::UnInitializeNetif()>\n")

  ::WiFiNetworkCore::UnInitializeEmw();

  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::UnInitializeNetif()<\n")
}

WiFiNetworkSoftAp::WiFiNetworkSoftAp(struct netif &netif) noexcept
  : WiFiNetworkCore()
  , dhcpInformFlag(true)
  , dhcpReleaseOnLinkLost(false)
  , driverAccessChannel(0U)
  , ipAcquired(false)
  , ipAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, gatewayAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, maskAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticIpAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticDnsServerAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticGatewayAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticMaskAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, emwInterfaceUp(false)
, ipV4AddressFound(false)
, lwipNetif(netif)
{
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::WiFiNetworkSoftAp()>\n")

  ::WiFiNetworkCore::WiFiNetworkSoftApPtr = this;

  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::WiFiNetworkSoftAp()<\n\n")
}

WiFiNetworkSoftAp::~WiFiNetworkSoftAp(void) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::~WiFiNetworkSoftAp()>\n")

  static_cast<void>(::WiFiNetworkCore::emwPtr->unRegisterStatusCallback(EmwApiBase::eSOFTAP));

  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::~WiFiNetworkSoftAp<()\n\n")
}

std::int32_t WiFiNetworkSoftAp::activateImp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  std::int32_t status = 0;
  EmwApiBase::SoftApSettings_t access_point_settings;
  EmwApiCore::MacAddress_t mac_address_48bits;

  DEBUG_STD_PRINTF("[%6" PRIu32 "] WiFiNetworkSoftAp::activateImp()>\n", HAL_GetTick())

  static_cast<void>(std::strncpy(access_point_settings.ssidString, ssidString,
                                 sizeof(access_point_settings.ssidString) - 1));
  access_point_settings.ssidString[sizeof(access_point_settings.ssidString) - 1] = '\0';

  static_cast<void>(std::strncpy(access_point_settings.passwordString, passwordString,
                                 sizeof(access_point_settings.passwordString) - 1));
  access_point_settings.passwordString[sizeof(access_point_settings.passwordString) - 1] = '\0';
  access_point_settings.channel = this->driverAccessChannel;
  static_cast<void>(std::snprintf(access_point_settings.ip.ipAddressLocal,
                                  sizeof(access_point_settings.ip.ipAddressLocal),
                                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR1_VAL(this->staticIpAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR2_VAL(this->staticIpAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR3_VAL(this->staticIpAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR4_VAL(this->staticIpAddress.u_addr.ip4))));
  static_cast<void>(std::snprintf(access_point_settings.ip.gatewayAddress,
                                  sizeof(access_point_settings.ip.gatewayAddress),
                                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR1_VAL(this->staticGatewayAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR2_VAL(this->staticGatewayAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR3_VAL(this->staticGatewayAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR4_VAL(this->staticGatewayAddress.u_addr.ip4))));
  static_cast<void>(std::snprintf(access_point_settings.ip.networkMask, sizeof(access_point_settings.ip.networkMask),
                                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR1_VAL(this->staticMaskAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR2_VAL(this->staticMaskAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR3_VAL(this->staticMaskAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR4_VAL(this->staticMaskAddress.u_addr.ip4))));
  static_cast<void>(std::snprintf(access_point_settings.ip.dnsServerAddress,
                                  sizeof(access_point_settings.ip.dnsServerAddress),
                                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR1_VAL(this->staticDnsServerAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR2_VAL(this->staticDnsServerAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR3_VAL(this->staticDnsServerAddress.u_addr.ip4)),
                                  static_cast<std::uint32_t>(LWIP_IP4_ADDR4_VAL(this->staticDnsServerAddress.u_addr.ip4))));

  if (EmwApiBase::eEMW_STATUS_OK \
      != ::WiFiNetworkCore::emwPtr->registerStatusCallback(&WiFiNetworkSoftAp::EmwStatusChanged,
          this, EmwApiBase::eSOFTAP)) {
    LWIP_ASSERT("Registering to the Wi-Fi module failed", false);
    status = -1;
  }
  else {
    DEBUG_STD_PRINTF("[%6" PRIu32 "] WiFiNetworkSoftAp::activateImp()> Setting AP ... \"%s\"\n",
                     HAL_GetTick(), access_point_settings.ssidString)

    this->emwInterfaceUp = false;
    static_cast<void>(this->WiFiNetworkCore::emwPtr->startSoftAp(access_point_settings));
    {
      const std::uint32_t tick_start = HAL_GetTick();

      while (!this->emwInterfaceUp) {
        STD_PRINTF("u");
        if (CheckTimeout(tick_start, 10000U)) {
          status = -1;
          break;
        }
        else {
          /* Be cooperative. */
          vTaskDelay(10U);
        }
      }
      STD_PRINTF("\n");
    }
  }
  if (EmwApiBase::eEMW_STATUS_OK != this->WiFiNetworkCore::emwPtr->getSoftApMacAddress(mac_address_48bits)) {
    LWIP_ASSERT("Cannot get the Software enabled Access Point MAC address of the Wi-Fi module", false);
    status = -1;
  }
  else {
    static_cast<void>(std::memcpy(this->lwipNetif.hwaddr, mac_address_48bits.bytes, sizeof(this->lwipNetif.hwaddr)));
    this->lwipNetif.hwaddr_len = sizeof(this->lwipNetif.hwaddr);
    DEBUG_STD_PRINTF("WiFiNetworkSoftAp::activateImp(): MAC address %X.%X.%X.%X.%X.%X\n",
                     this->lwipNetif.hwaddr[0], this->lwipNetif.hwaddr[1], this->lwipNetif.hwaddr[2],
                     this->lwipNetif.hwaddr[3], this->lwipNetif.hwaddr[4], this->lwipNetif.hwaddr[5])
  }
  DEBUG_STD_PRINTF("[%6" PRIu32 "] WiFiNetworkSoftAp::activateImp(): done\n", HAL_GetTick())
  return status;
}



std::int32_t WiFiNetworkSoftAp::deActivateImp(void) noexcept
{
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::deActivateImp()>\n")

  static_cast<void>(::WiFiNetworkCore::emwPtr->stopSoftAp());

  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::deActivateImp()<\n")
  return 0;
}

void WiFiNetworkSoftAp::inputNetifImp(EmwNetworkStack::Buffer_t *bufferPtr) noexcept
{
  const struct eth_hdr *const ethernet_header_ptr \
      = reinterpret_cast<struct eth_hdr *>(EmwNetworkStack::GetBufferPayload(bufferPtr));
  const std::uint16_t eth_type = lwip_htons(ethernet_header_ptr->type);

  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetworkSoftAp::inputNetifImp()> 0x%02" PRIx32 "\n",
              static_cast<std::uint32_>(eth_type)));

  switch (eth_type) {
    case ETHTYPE_IP:
    case ETHTYPE_ARP:
    case ETHTYPE_IPV6:
#if PPPOE_SUPPORT
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
      {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetworkSoftAp::inputNetifImp(): -> input packet 0x%02" PRIx32 " (%" PRIu32 ")\n",
                     static_cast<std::uint32_t>(eth_type), static_cast<std::uint32_t>(bufferPtr->tot_len)));
#if defined(ENABLE_DEBUG)
        {
          DEBUG_STD_PRINTF(">\n")
          for (std::uint32_t i = 0U; i < bufferPtr->tot_len; i++) {
            const uint8_t rx_data = (static_cast<std::uint8_t *>(bufferPtr->payload))[i];
            DEBUG_STD_PRINTF("%c", isprint(rx_data) ? rx_data : '*')
          }
          DEBUG_STD_PRINTF("<\n")
        }
#endif /* ENABLE_DEBUG */
        if (static_cast<err_t>(ERR_OK) != this->lwipNetif.input(bufferPtr, &this->lwipNetif)) {
          LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetworkSoftAp::inputNetifImp(): lwipNetif->input() failed\n"));
          static_cast<void>(pbuf_free(bufferPtr));
        }
        break;
      }
    case ETHTYPE_JUMBO:
    case ETHTYPE_PROFINET:
    case ETHTYPE_ETHERCAT:
    case ETHTYPE_LLDP:
    case ETHTYPE_SERCOS:
    case ETHTYPE_MRP:
    case ETHTYPE_PTP:
    case ETHTYPE_QINQ:
    default: {
        static_cast<void>(pbuf_free(bufferPtr));
        break;
      }
  }
  EMW_STATS_INCREMENT(free)
}

void WiFiNetworkSoftAp::EmwStatusChanged(EmwApiBase::EmwInterface interface,
    enum EmwApiBase::WiFiEvent status, void *argPtr) noexcept
{
  class WiFiNetworkSoftAp *const THIS = static_cast<class WiFiNetworkSoftAp *>(argPtr);

  static_cast<void>(interface);
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::EmwStatusChanged()>\n")

  switch (status) {
    case EmwApiBase::eWIFI_EVENT_AP_DOWN: {
        DEBUG_STD_PRINTF("WiFiNetworkSoftAp::EmwStatusChanged() -> EmwApiBase::eWIFI_EVENT_AP_DOWN\n")
        THIS->emwInterfaceUp = false;
        break;
      }
    case EmwApiBase::eWIFI_EVENT_AP_UP: {
        DEBUG_STD_PRINTF("WiFiNetworkSoftAp::EmwStatusChanged() -> EmwApiBase::eWIFI_EVENT_AP_UP\n")
        THIS->emwInterfaceUp = true;
        break;
      }
    case EmwApiBase::eWIFI_EVENT_STA_DOWN:
    case EmwApiBase::eWIFI_EVENT_STA_UP:
    case EmwApiBase::eWIFI_EVENT_STA_GOT_IP:
    case EmwApiBase::eWIFI_EVENT_NONE:
    default: {
        break;
      }
  }
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::EmwStatusChanged()<\n")
}

err_t WiFiNetworkSoftAp::OutputToTransmitFifo(struct netif *netifPtr, struct pbuf *bufferPtr) noexcept
{
  err_t status;
  static_cast<void>(netifPtr);

  if (nullptr != bufferPtr) {
    bufferPtr->if_idx = EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX;
    WiFiNetworkCore::OutputToTransmitFifo(bufferPtr);
    status = static_cast<err_t>(ERR_OK);
  }
  else {
    status = static_cast<err_t>(ERR_ARG);
  }
  return status;
}

void WiFiNetworkSoftAp::NetifStatusCallback(struct netif *netifPtr) noexcept
{
  std::setbuf(stdout, nullptr);

  if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
    class WiFiNetworkSoftAp *const THIS = WiFiNetworkSoftAp::MySelf(netifPtr);

    DEBUG_STD_PRINTF("WiFiNetworkSoftAp::NetifStatusCallback()>\n")

    {
      const ip_addr_t IP_ADDRESS_ZERO = IPADDR4_INIT(0);
      /* Lost connection, release the IPv4 address. */
      if ((std::memcmp(&netifPtr->ip_addr, &IP_ADDRESS_ZERO, sizeof(netifPtr->ip_addr)) != 0) \
          && (!LWIP_NETIF_IS_LINK_UP(netifPtr)) \
          && THIS->dhcpReleaseOnLinkLost) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetworkSoftAp::NetifStatusCallback(): Calling dhcp_release_and_stop()\n"));

        dhcp_release_and_stop(netifPtr);

        if (LWIP_NETIF_IS_UP(netifPtr)) {
          LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                      ("WiFiNetworkSoftAp::NetifStatusCallback(): Calling dhcp_start()\n"));

          static_cast<void>(dhcp_start(netifPtr));
        }
      }
      /* Up connection, so need to inform other at first time for IPv4. */
      if (THIS->dhcpInformFlag && LWIP_NETIF_IS_LINK_UP(netifPtr)) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetworkSoftAp::NetifStatusCallback(): Calling dhcp_inform()\n"));

        dhcp_inform(netifPtr);
        THIS->dhcpInformFlag = false;
      }
    }
    /* Update IPv4 address */
    if (!ip_addr_cmp(&THIS->ipAddress, &netifPtr->ip_addr)) {
      ip_addr_copy(THIS->ipAddress, netifPtr->ip_addr);
      ip_addr_copy(THIS->maskAddress, netifPtr->netmask);
      ip_addr_copy(THIS->gatewayAddress, netifPtr->gw);
      if (!LWIP_IP_ADDR_ISANY_VAL(THIS->ipAddress)) {
        DEBUG_STD_PRINTF("WiFiNetworkSoftAp::NetifStatusCallback(): Got IPv4 address.\n")
        THIS->ipV4AddressFound = true;
      }
    }
  }
}

class WiFiNetworkSoftAp *WiFiNetworkSoftAp::MySelf(struct netif *netifPtr) noexcept {
    return static_cast<class WiFiNetworkSoftAp *>(netifPtr->state);
}


netif_ext_callback_t WiFiNetworkSoftAp::NetifExtCallback;

void WiFiNetworkSoftAp::NetifExtCallbackFunction(struct netif *netifPtr,
    netif_nsc_reason_t reason, const netif_ext_callback_args_t *argsPtr) noexcept
{
  static_cast<void>(netifPtr);
  static_cast<void>(argsPtr);
  static_cast<void>(reason);

#if defined(ENABLE_DEBUG)
  DEBUG_STD_PRINTF("WiFiNetworkSoftAp::NetifExtCallbackFunction() -> (%04" PRIx32 ") %s\n",
                   static_cast<std::uint32_t>(reason), NetifReasonToString(reason))
#endif /* ENABLE_DEBUG */

  if (nullptr != argsPtr) {
    DEBUG_STD_PRINTF("Link state  : %02" PRIx32 "\n", static_cast<std::uint32_t>(argsPtr->link_changed.state))
    DEBUG_STD_PRINTF("Status state: %02" PRIx32 "\n", static_cast<std::uint32_t>(argsPtr->status_changed.state))
  }
}


static bool CheckTimeout(std::uint32_t tickStart, std::uint32_t tickCount)
{
  const std::uint32_t elapsed_ticks = HAL_GetTick() - tickStart;
  bool status;

  if (elapsed_ticks > tickCount) {
    status = true;
  }
  else {
    status = false;
  }
  return status;
}

#if defined(ENABLE_DEBUG)
#define CASE(x) case x: {return #x; /*break;*/}
#define DEFAULT default: {return "UNKNOWN"; /*break;*/}

static const char *NetifReasonToString(netif_nsc_reason_t reason)
{
  switch (reason) {
      CASE(LWIP_NSC_NONE)
      CASE(LWIP_NSC_NETIF_ADDED)
      CASE(LWIP_NSC_NETIF_REMOVED)
      CASE(LWIP_NSC_LINK_CHANGED)
      CASE(LWIP_NSC_STATUS_CHANGED)
      CASE(LWIP_NSC_IPV4_ADDRESS_CHANGED)
      CASE(LWIP_NSC_IPV4_GATEWAY_CHANGED)
      CASE(LWIP_NSC_IPV4_NETMASK_CHANGED)
      CASE(LWIP_NSC_IPV4_SETTINGS_CHANGED)
      CASE(LWIP_NSC_IPV4_ADDRESS_CHANGED | LWIP_NSC_IPV4_GATEWAY_CHANGED \
           | LWIP_NSC_IPV4_NETMASK_CHANGED | LWIP_NSC_IPV4_SETTINGS_CHANGED)
      CASE(LWIP_NSC_IPV6_SET)
      CASE(LWIP_NSC_IPV6_ADDR_STATE_CHANGED)
      CASE(LWIP_NSC_IPV4_ADDR_VALID | LWIP_NSC_IPV4_SETTINGS_CHANGED \
           | LWIP_NSC_IPV4_NETMASK_CHANGED | LWIP_NSC_IPV4_GATEWAY_CHANGED | LWIP_NSC_IPV4_ADDRESS_CHANGED)
      DEFAULT;
  }
}
#endif /* ENABLE_DEBUG */
