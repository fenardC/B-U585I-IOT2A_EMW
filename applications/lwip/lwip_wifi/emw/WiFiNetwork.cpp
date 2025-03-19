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
#include "WiFiNetwork.hpp"
#include "EmwApiEmwBypass.hpp"
#include "EmwNetworkStack.hpp"
#include "emw_conf.hpp"
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

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)
//#define ENABLE_DEBUG

static bool CheckTimeout(std::uint32_t tickStart, std::uint32_t tickCount);
static const char *NetifReasonToString(netif_nsc_reason_t reason);
static const char *SecurityToString(enum EmwApiBase::SecurityType security);


WiFiNetwork::WiFiNetwork(EmwApiBase::EmwInterface wiFiInterface, struct netif &networkInterface) noexcept
  : deviceIdentifierStringPtr(WiFiNetwork::Driver.systemInformations.productName)
  , deviceNameStringPtr(WiFiNetwork::Driver.systemInformations.productIdentifier)
  , deviceVersionString{0}
  , dhcpInformFlag(true)
  , dhcpReleaseOnLinkLost(false)
  , driverAccessChannel(0U)
  , ipAcquired(false)
  , ipAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, ipAddress6{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V6}
, gatewayAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, gatewayAddress6{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V6}
, maskAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, maskAddress6{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V6}
, staticIpAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticDnsServerAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticGatewayAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, staticMaskAddress{{{{ 0, 0, 0, 0 }, IP6_NO_ZONE}}, IPADDR_TYPE_V4}
, driverInterface(wiFiInterface)
, driverInterfaceUp(false)
, ipV4AddressFound(false)
, ipV6AddressFound(false)
, netInterface(networkInterface)
{
  STD_PRINTF("WiFiNetwork::WiFiNetwork()>\n")

  (void) std::printf("%s\n", WiFiNetwork::Driver.getConfigurationString());
  STD_PRINTF("WiFiNetwork::WiFiNetwork()<\n\n")
}

WiFiNetwork::~WiFiNetwork(void) noexcept
{
  STD_PRINTF("WiFiNetwork::~WiFiNetwork()>\n")
  STD_PRINTF("WiFiNetwork::~WiFiNetwork<()\n\n")
}

void WiFiNetwork::initializeNetif(struct netif *netifPtr) noexcept
{
  STD_PRINTF("WiFiNetwork::initializeNetif()>\n")

  netifPtr->output = etharp_output;
  netifPtr->output_ip6 = ethip6_output;
  netifPtr->ip6_autoconfig_enabled = 1;
  netifPtr->linkoutput = WiFiNetwork::OutputToDriver;
  netifPtr->mtu = EmwNetworkStack::NETWORK_MTU_SIZE;
  netifPtr->flags = static_cast<u8_t>(NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET);
  netif_set_status_callback(netifPtr, WiFiNetwork::NetifStatusCallback);
  netif_set_link_callback(netifPtr, WiFiNetwork::NetifStatusCallback);

  STD_PRINTF("WiFiNetwork::initializeNetif()<\n")
}

std::int32_t WiFiNetwork::startDriver(void) noexcept
{
  std::int32_t status;

  STD_PRINTF("WiFiNetwork::startDriver()>\n")

  if (0U == WiFiNetwork::Driver.numberOfInterfacesRunning()) {
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startDriver(): REBOOT(HW) ...\n", HAL_GetTick())
    (void) WiFiNetwork::Driver.resetHardware();

    if (static_cast<err_t>(ERR_OK) == sys_mbox_new(&WiFiNetwork::TransmitFifo, 8U)) {
      static const char transmit_fifo_name[] = {"WiFiNetwork::FIFO"};
      vQueueAddToRegistry(static_cast<QueueHandle_t>(WiFiNetwork::TransmitFifo.mbx), transmit_fifo_name);
    }
    else {
      LWIP_ASSERT("Creation of the transmit FIFO for the Wi-Fi device failed", false);
    }
    {
      static const char transmit_fifo_thread_name[] = {"NETIF-SendThread"};

      (void) sys_thread_new(transmit_fifo_thread_name,
                            static_cast<lwip_thread_fn>(WiFiNetwork::TranmitThreadFunction), this,
                            WiFiNetwork::TRANSMIT_THREAD_STACK_SIZE, WiFiNetwork::TRANSMIT_THREAD_PRIORITY);
      /* Be cooperative. */
      vTaskDelay(1U);
    }
  }
  if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::Driver.initialize()) {
    LWIP_ASSERT("Initializing the Wi-Fi module failed", false);
    status = -1;
  }
  else {
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startDriver(): EMW initialization [OK]\n", HAL_GetTick())

    WiFiNetwork::Driver.getVersion(this->deviceVersionString, sizeof(this->deviceVersionString));
    this->deviceVersionString[sizeof(this->deviceVersionString) - 1] = '\0';

    if (1U == WiFiNetwork::Driver.numberOfInterfacesRunning()) {
      (void) WiFiNetwork::Driver.setByPass(1, WiFiNetwork::InputFromDriver);
    }
    status = 0;
  }

  STD_PRINTF("WiFiNetwork::startDriver()<\n\n")
  return status;
}

std::int32_t WiFiNetwork::startSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  std::int32_t status = 0;
  EmwApiBase::SoftApSettings_t access_point_settings;
  EmwApiCore::MacAddress_t mac_address_48bits;

  STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startSoftAp()>\n", HAL_GetTick())

  WiFiNetwork::WiFiNetworks[EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX] = this;
  (void) std::strncpy(access_point_settings.ssidString, ssidString,
                      sizeof(access_point_settings.ssidString) - 1);
  access_point_settings.ssidString[sizeof(access_point_settings.ssidString) - 1] = '\0';

  (void) std::strncpy(access_point_settings.passwordString, passwordString,
                      sizeof(access_point_settings.passwordString) - 1);
  access_point_settings.passwordString[sizeof(access_point_settings.passwordString) - 1] = '\0';
  access_point_settings.channel = this->driverAccessChannel;
  (void) std::snprintf(access_point_settings.ip.ipAddressLocal, sizeof(access_point_settings.ip.ipAddressLocal),
                       "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                       static_cast<std::uint32_t>(ip4_addr1_val(this->staticIpAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr2_val(this->staticIpAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr3_val(this->staticIpAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr4_val(this->staticIpAddress.u_addr.ip4)));
  (void) std::snprintf(access_point_settings.ip.gatewayAddress, sizeof(access_point_settings.ip.gatewayAddress),
                       "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                       static_cast<std::uint32_t>(ip4_addr1_val(this->staticGatewayAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr2_val(this->staticGatewayAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr3_val(this->staticGatewayAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr4_val(this->staticGatewayAddress.u_addr.ip4)));
  (void) std::snprintf(access_point_settings.ip.networkMask, sizeof(access_point_settings.ip.networkMask),
                       "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                       static_cast<std::uint32_t>(ip4_addr1_val(this->staticMaskAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr2_val(this->staticMaskAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr3_val(this->staticMaskAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr4_val(this->staticMaskAddress.u_addr.ip4)));
  (void) std::snprintf(access_point_settings.ip.dnsServerAddress, sizeof(access_point_settings.ip.dnsServerAddress),
                       "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                       static_cast<std::uint32_t>(ip4_addr1_val(this->staticDnsServerAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr2_val(this->staticDnsServerAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr3_val(this->staticDnsServerAddress.u_addr.ip4)),
                       static_cast<std::uint32_t>(ip4_addr4_val(this->staticDnsServerAddress.u_addr.ip4)));

  if (EmwApiBase::eEMW_STATUS_OK \
      != WiFiNetwork::Driver.registerStatusCallback(&WiFiNetwork::InformOfDriverStatus, this, EmwApiBase::eSOFTAP)) {
    LWIP_ASSERT("Registering to the Wi-Fi module failed", false);
    status = -1;
  }
  else {
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startSoftAp()> Setting AP ... \"%s\"\n",
               HAL_GetTick(), access_point_settings.ssidString)

    this->driverInterfaceUp = false;
    (void) WiFiNetwork::Driver.startSoftAp(access_point_settings);
    {
      const std::uint32_t tick_start = HAL_GetTick();

      while (!this->driverInterfaceUp) {
        if (CheckTimeout(tick_start, 10000U)) {
          status = -1;
          break;
        }
        else {
          /* Be cooperative. */
          vTaskDelay(10U);
        }
      }
    }
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startSoftAp(): done\n", HAL_GetTick())
  }
  if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::Driver.getSoftApMacAddress(mac_address_48bits)) {
    LWIP_ASSERT("Cannot get the Software enabled Access Point MAC address of the Wi-Fi module", false);
    status = -1;
  }
  else {
    (void) std::memcpy(this->netInterface.hwaddr, mac_address_48bits.bytes, sizeof(this->netInterface.hwaddr));
    this->netInterface.hwaddr_len = sizeof(this->netInterface.hwaddr);
    STD_PRINTF("WiFiNetwork::startSoftAp(): MAC address %X.%X.%X.%X.%X.%X\n",
               this->netInterface.hwaddr[0], this->netInterface.hwaddr[1], this->netInterface.hwaddr[2],
               this->netInterface.hwaddr[3], this->netInterface.hwaddr[4], this->netInterface.hwaddr[5])
  }
  return status;
}

std::int32_t WiFiNetwork::startConnectionToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  std::int32_t status = 0;
  EmwApiCore::MacAddress_t mac_address_48bits;

  STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startConnectionToAp()>\n", HAL_GetTick())

  WiFiNetwork::WiFiNetworks[EmwApiBase::eWIFI_INTERFACE_STATION_IDX] = this;
  WiFiNetwork::Driver.stationSettings.dhcpIsEnabled = true;
  if (EmwApiBase::eEMW_STATUS_OK \
      != WiFiNetwork::Driver.registerStatusCallback(WiFiNetwork::InformOfDriverStatus, this, EmwApiBase::eSTATION)) {
    LWIP_ASSERT("Registering to the Wi-Fi module failed", false);
    status = -1;
  }
  else if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::Driver.getStationMacAddress(mac_address_48bits)) {
    LWIP_ASSERT("Cannot get the MAC address of the Wi-Fi module", false);
    status = -1;
  }
  else {
    (void) std::memcpy(this->netInterface.hwaddr, mac_address_48bits.bytes, sizeof(this->netInterface.hwaddr));
    this->netInterface.hwaddr_len = sizeof(this->netInterface.hwaddr);
    STD_PRINTF("WiFiNetwork::startConnectionToAp()(): MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
               this->netInterface.hwaddr[0], this->netInterface.hwaddr[1], this->netInterface.hwaddr[2],
               this->netInterface.hwaddr[3], this->netInterface.hwaddr[4], this->netInterface.hwaddr[5])

    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startConnectionToAp(): Joining ... \"%s\"\n",
               HAL_GetTick(), this->credentials.ssidString)
    (void) WiFiNetwork::Driver.connect(ssidString, passwordString, EmwApiBase::eSEC_AUTO);
    {
      const std::uint32_t tick_start = HAL_GetTick();

      while (!this->driverInterfaceUp) {
        (void) std::printf("c");
        if (CheckTimeout(tick_start, 10000U)) {
          status = -1;
          break;
        }
        else {
          /* Be cooperative. */
          vTaskDelay(100U);
        }
      }
      (void) std::printf("\n");
    }
  }
  STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startConnectionToAp()< done\n", HAL_GetTick())
  return status;
}

std::int32_t WiFiNetwork::stopDriver(void) noexcept
{
  STD_PRINTF("WiFiNetwork::stopDriver()>\n")

  if (EmwApiBase::eSTATION == this->driverInterface) {
    (void) WiFiNetwork::Driver.disconnect();
    (void) WiFiNetwork::Driver.unRegisterStatusCallback(EmwApiBase::eSTATION);
  }
  else {
    (void) WiFiNetwork::Driver.stopSoftAp();
    (void) WiFiNetwork::Driver.unRegisterStatusCallback(EmwApiBase::eSOFTAP);
  }
  STD_PRINTF("WiFiNetwork::stopDriver()<\n")
  return 0;
}

std::int32_t WiFiNetwork::unInitializeDriver(void) noexcept
{
  STD_PRINTF("WiFiNetwork::unInitializeDriver()>\n")

  if (1U == WiFiNetwork::Driver.numberOfInterfacesRunning()) {
    (void) WiFiNetwork::Driver.setByPass(0, nullptr);
  }
  WiFiNetwork::Driver.unInitialize();
  STD_PRINTF("WiFiNetwork::unInitializeDriver()<\n")
  return 0;
}

void WiFiNetwork::unInitializeNetif(void) noexcept
{
  STD_PRINTF("WiFiNetwork::unInitializeNetif()>\n")

  this->unInitializeDriver();
  if (0U == WiFiNetwork::Driver.numberOfInterfacesRunning()) {
    WiFiNetwork::DeleteTransmitFifo();
    sys_mbox_free(&this->TransmitFifo);
  }
  STD_PRINTF("WiFiNetwork::unInitializeNetif()<\n")
}

void WiFiNetwork::InformOfDriverStatus(EmwApiBase::EmwInterface interface, enum EmwApiBase::WiFiEvent status,
                                       void *argPtr) noexcept
{
  class WiFiNetwork *const wifi_network_ptr = static_cast<class WiFiNetwork *>(argPtr);

  STD_PRINTF("WiFiNetwork::InformOfDriverStatus()>\n")

  if (EmwApiBase::eSTATION == interface) {
    switch (status) {
      case EmwApiBase::eWIFI_EVENT_STA_DOWN: {
          STD_PRINTF("WiFiNetwork::InformOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_STA_DOWN\n")
          wifi_network_ptr->driverInterfaceUp = false;
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_UP: {
          STD_PRINTF("WiFiNetwork::InformOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_STA_UP\n")
          wifi_network_ptr->driverInterfaceUp = true;
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_GOT_IP: {
          STD_PRINTF("WiFiNetwork::InformOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_STA_GOT_IP\n")
          break;
        }
      case EmwApiBase::eWIFI_EVENT_AP_DOWN:
      case EmwApiBase::eWIFI_EVENT_AP_UP:
      case EmwApiBase::eWIFI_EVENT_NONE:
      default: {
          break;
        }
    }
  }
  else if (EmwApiBase::eSOFTAP == interface) {
    switch (status) {
      case EmwApiBase::eWIFI_EVENT_AP_DOWN: {
          STD_PRINTF("WiFiNetwork::InformOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_AP_DOWN\n")
          wifi_network_ptr->driverInterfaceUp = false;
          break;
        }
      case EmwApiBase::eWIFI_EVENT_AP_UP: {
          STD_PRINTF("WiFiNetwork::InformOfDriverStatus() -> EmwApiBase::eWIFI_EVENT_AP_UP\n")
          wifi_network_ptr->driverInterfaceUp = true;
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
  }
  else {
    /* nothing */
  }
  STD_PRINTF("WiFiNetwork::InformOfDriverStatus()<\n")
}

void WiFiNetwork::InputFromDriver(EmwNetworkStack::Buffer_t *bufferPtr, std::uint32_t interfaceIndex) noexcept
{
  if (nullptr != bufferPtr) {
    if (0U < EmwNetworkStack::GetBufferPayloadSize(bufferPtr)) {
      struct netif &net_interface = WiFiNetwork::WiFiNetworks[(interfaceIndex == EmwApiBase::eWIFI_INTERFACE_STATION_IDX)
                                    ? EmwApiBase::eWIFI_INTERFACE_STATION_IDX : EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX]->netInterface;
      WiFiNetwork::InputNetif(net_interface, bufferPtr);
    }
    else {
      EmwNetworkStack::FreeBuffer(bufferPtr);
    }
  }
}

void WiFiNetwork::InputNetif(struct netif &networkInterface, EmwNetworkStack::Buffer_t *bufferPtr) noexcept
{
  const struct eth_hdr *const ethernet_header_ptr = reinterpret_cast<struct eth_hdr *>(EmwNetworkStack::GetBufferPayload(
      bufferPtr));
  const std::uint16_t eth_type = lwip_htons(ethernet_header_ptr->type);

  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetwork::inputNetif()> 0x%02" PRIx32 "\n",
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
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetwork::inputNetif(): -> input packet 0x%02" PRIx32 " (%" PRIu32 ")\n",
                    static_cast<std::uint32_t>(eth_type), static_cast<std::uint32_t>(bufferPtr->tot_len)));
#if defined(ENABLE_DEBUG)
        {
          std::printf(">\n");
          for (std::uint32_t i = 0U; i < bufferPtr->tot_len; i++) {
            const uint8_t rx_data = (static_cast<std::uint8_t *>(bufferPtr->payload))[i];
            std::printf("%c", isprint(rx_data) ? rx_data : '*');
          }
          std::printf("<\n");
        }
#endif /* ENABLE_DEBUG */
        if (static_cast<err_t>(ERR_OK) != networkInterface.input(bufferPtr, &networkInterface)) {
          LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetwork::inputNetif(): networkInterface->input() failed\n"));
          (void) pbuf_free(bufferPtr);
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
        (void) pbuf_free(bufferPtr);
        break;
      }
  }
  EMW_STATS_INCREMENT(free)
}

err_t WiFiNetwork::OutputToDriver(struct netif *netifPtr, struct pbuf *bufferPtr) noexcept
{
  err_t status = static_cast<err_t>(ERR_ARG);

  if ((nullptr != netifPtr) && (nullptr != netifPtr->state) && (nullptr != bufferPtr)) {
    const class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::MySelf(netifPtr);

    pbuf_ref(bufferPtr);
    LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                ("WiFiNetwork::OutputToDriver(): Transmit buffer %p next=%p  tot_len=%" PRIu32 " len=%" PRIu32 "\n",
                 static_cast<const void *>(bufferPtr), static_cast<const void *>(bufferPtr->next),
                 static_cast<std::uint32_t>(bufferPtr->tot_len), static_cast<std::uint32_t>(bufferPtr->len)));

    if (wifi_network_ptr->driverInterface == EmwApiBase::eSTATION) {
      bufferPtr->if_idx = static_cast<std::uint8_t>(EmwApiBase::eWIFI_INTERFACE_STATION_IDX);
    }
    else {
      bufferPtr->if_idx = static_cast<std::uint8_t>(EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX);
    }
    if ((nullptr != bufferPtr->next) || (bufferPtr->tot_len != bufferPtr->len)) {
      struct pbuf *buf_send_ptr = pbuf_clone(PBUF_RAW, PBUF_RAM, bufferPtr);

      (void) pbuf_free(bufferPtr);
      if (nullptr == buf_send_ptr) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetwork::OutputToDriver(): No memory for chained buffer!!!"));
        status = static_cast<err_t>(ERR_MEM);
      }
      else {
        (void) sys_mbox_post(&WiFiNetwork::TransmitFifo, buf_send_ptr);
        status = static_cast<err_t>(ERR_OK);

#if defined(ENABLE_DEBUG)
        {
          std::printf(">\n");
          for (std::uint32_t i = 0U; i < buf_send_ptr->tot_len; i++) {
            const std::uint8_t tx_data = (static_cast<std::uint8_t *>(buf_send_ptr->payload))[i];
            std::printf("%x", tx_data);
          }
          std::printf("<\n");
        }
#endif /* ENABLE_DEBUG */
      }
    }
    else {
      sys_mbox_post(&WiFiNetwork::TransmitFifo, bufferPtr);
      status = static_cast<err_t>(ERR_OK);
    }
  }
  else {
    status = static_cast<err_t>(ERR_INPROGRESS);
  }
  return status;
}

void WiFiNetwork::PushToDriver(std::uint32_t timeoutInMs) noexcept
{
  void *message_ptr = nullptr;

  if (SYS_ARCH_TIMEOUT != sys_arch_mbox_fetch(&WiFiNetwork::TransmitFifo, &message_ptr, timeoutInMs)) {
    if (nullptr != message_ptr) {
      struct pbuf *const packet_ptr = static_cast<struct pbuf *>(message_ptr);
      const EmwApiBase::Status ret = WiFiNetwork::Driver.output(static_cast<std::uint8_t*>(packet_ptr->payload),
                                     static_cast<std::int32_t>(packet_ptr->len),
                                     static_cast<std::int32_t>(packet_ptr->if_idx));

      LWIP_ASSERT("Driver.output() failed", EmwApiBase::eEMW_STATUS_OK == ret);
      (void) pbuf_free(packet_ptr);
    }
  }
}

void WiFiNetwork::TranmitThreadFunction(void *THIS) noexcept
{
  static_cast<void>(THIS);

  std::setbuf(stdout, nullptr);
  lwip_socket_thread_init();

  WiFiNetwork::TransmitThreadQuitFlag = false;
  while (!WiFiNetwork::TransmitThreadQuitFlag) {
    WiFiNetwork::PushToDriver(500U);
  }
  WiFiNetwork::TransmitThreadQuitFlag = false;
  lwip_socket_thread_cleanup();
  vTaskDelete(NULL);
  for (;;);
}

void WiFiNetwork::NetifStatusCallback(struct netif *netifPtr) noexcept
{
  std::setbuf(stdout, nullptr);

  if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
    class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::MySelf(netifPtr);

    STD_PRINTF("WiFiNetwork::NetifStatusCallback()>\n")

    {
      const ip_addr_t IP_ADDRESS_ZERO = IPADDR4_INIT(0);
      /* Lost connection, release the IPv4 address. */
      if ((std::memcmp(&netifPtr->ip_addr, &IP_ADDRESS_ZERO, sizeof(netifPtr->ip_addr)) != 0) \
          && (!netif_is_link_up(netifPtr)) \
          && wifi_network_ptr->dhcpReleaseOnLinkLost) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetwork::NetifStatusCallback(): Calling dhcp_release_and_stop()\n"));

        dhcp_release_and_stop(netifPtr);

        if (netif_is_up(netifPtr)) {
          LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                      ("WiFiNetwork::NetifStatusCallback(): Calling dhcp_start()\n"));

          (void) dhcp_start(netifPtr);
        }
      }
      /* Up connection, so need to inform other at first time for IPv4. */
      if (wifi_network_ptr->dhcpInformFlag && netif_is_link_up(netifPtr)) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetwork::NetifStatusCallback(): Calling dhcp_inform()\n"));

        dhcp_inform(netifPtr);
        wifi_network_ptr->dhcpInformFlag = false;
      }
    }
    /* Update IPv4 address */
    if (!ip_addr_cmp(&wifi_network_ptr->ipAddress, &netifPtr->ip_addr)) {
      ip_addr_copy(wifi_network_ptr->ipAddress, netifPtr->ip_addr);
      ip_addr_copy(wifi_network_ptr->maskAddress, netifPtr->netmask);
      ip_addr_copy(wifi_network_ptr->gatewayAddress, netifPtr->gw);
      if (!ip_addr_isany_val(wifi_network_ptr->ipAddress)) {
        STD_PRINTF("WiFiNetwork::NetifStatusCallback(): Got IPv4 address.\n")
        wifi_network_ptr->ipV4AddressFound = true;
      }
    }
    /* Got local IPv6 address */
    if (ip6_addr_isvalid(netifPtr->ip6_addr_state[0])) {
      if (!ip_addr_cmp(&wifi_network_ptr->ipAddress6, &netifPtr->ip6_addr[0])) {
        ip_addr_copy(wifi_network_ptr->ipAddress6, netifPtr->ip6_addr[0]);
        if (!ip_addr_isany_val(wifi_network_ptr->ipAddress6)) {
          STD_PRINTF("WiFiNetwork::NetifStatusCallback(): Got local IPv6 address.\n")
          // wifi_network_ptr->ipV6AddressFound = false;
        }
      }
    }
    if (ip6_addr_isvalid(netifPtr->ip6_addr_state[1])) {
      if (!ip_addr_cmp(&wifi_network_ptr->ipAddress6, &netifPtr->ip6_addr[1])) {
        ip_addr_copy(wifi_network_ptr->ipAddress6, netifPtr->ip6_addr[1]);

        if (!ip_addr_isany_val(wifi_network_ptr->ipAddress6)) {
          STD_PRINTF("WiFiNetwork::NetifStatusCallback(): Got IPv6 address.\n")
          wifi_network_ptr->ipV6AddressFound = true;
        }
      }
    }
    if (wifi_network_ptr->ipV6AddressFound && wifi_network_ptr->ipV4AddressFound) {
      wifi_network_ptr->ipAcquired = true;
      wifi_network_ptr->ipV4AddressFound = false;
      wifi_network_ptr->ipV6AddressFound = false;
    }
  }
}

void WiFiNetwork::CheckIoSpeed(void) noexcept
{
  const std::uint16_t ipc_test_data_size = EmwNetworkStack::NETWORK_BUFFER_SIZE;
  std::unique_ptr<std::uint8_t[], decltype(&vPortFree)> \
  ipc_test_data_ptr(static_cast<std::uint8_t*>(pvPortMalloc(ipc_test_data_size)), &vPortFree);

  if (nullptr != ipc_test_data_ptr) {
    std::uint32_t send_count = 0U;
    std::uint32_t receive_count = 0U;
    std::uint32_t time_ms_count;

    std::memset(&ipc_test_data_ptr[0], 0x00, ipc_test_data_size);

    (void) std::printf("\nWiFiNetwork::CheckIoSpeed()> (130 x (%" PRIu32 " + %" PRIu32 ")) ...\n",
                       static_cast<std::uint32_t>(ipc_test_data_size), static_cast<std::uint32_t>(ipc_test_data_size));
    time_ms_count = HAL_GetTick();

    for (std::uint32_t i = 0U; i < 130U; i++) {
      std::uint16_t echoed_length = ipc_test_data_size;
      const EmwApiBase::Status status \
        = WiFiNetwork::Driver.testIpcEcho(reinterpret_cast<std::uint8_t (&)[]>(* &ipc_test_data_ptr[0]), ipc_test_data_size,
                                          reinterpret_cast<std::uint8_t (&)[]>(* &ipc_test_data_ptr[0]), echoed_length, 5000U);
      if (EmwApiBase::eEMW_STATUS_OK != status) {
        (void) std::printf("Failed to call testIpcEcho() (%" PRId32 ")\n", static_cast<std::int32_t>(status));
        return;
      }
      else {
        send_count += ipc_test_data_size;
        receive_count += echoed_length;
      }
    }
    time_ms_count = HAL_GetTick() - time_ms_count;
    if (0 != time_ms_count) {
      const std::uint32_t total_sent_and_received = send_count + receive_count;
      const std::uint32_t speed = (8 * total_sent_and_received) / time_ms_count;
      (void) std::printf("transferred: %" PRIu32 " bytes, time: %" PRIu32 " ms, Speed: %" PRIu32 " Kbps\n",
                         total_sent_and_received, time_ms_count, speed);
    }
  }
}

class WiFiNetwork *WiFiNetwork::MySelf(struct netif *netifPtr) noexcept {
    return static_cast<class WiFiNetwork *>(netifPtr->state);
}

std::int32_t WiFiNetwork::Scan(void) noexcept
{
  std::int32_t status = -1;
  static EmwApiBase::ApInfo_t APs[10];
  const EmwApiBase::ApInfo_t ap_default;
  std::int8_t count = 0;
  const char ssid[33] = {""};
  const EmwApiBase::Status emw_status = WiFiNetwork::Driver.scan(EmwApiBase::ePASSIVE, ssid, 0);

  if (EmwApiBase::eEMW_STATUS_OK == emw_status) {
    status = 0;

    for (auto &ap : APs) {
      ap = ap_default;
    }

    count \
      = WiFiNetwork::Driver.getScanResults(reinterpret_cast<std::uint8_t (&)[480]>(*reinterpret_cast<std::uint8_t *>(APs)),
                                           11);
  }

  if (count > 0) {
    static const enum EmwApiBase::SecurityType emw_security[] = {
      EmwApiBase::eSEC_NONE,
      EmwApiBase::eSEC_WEP,
      EmwApiBase::eSEC_WPA_TKIP,
      EmwApiBase::eSEC_WPA_AES,
      EmwApiBase::eSEC_WPA2_TKIP,
      EmwApiBase::eSEC_WPA2_AES,
      EmwApiBase::eSEC_WPA2_MIXED,
      EmwApiBase::eSEC_WPA3,
      EmwApiBase::eSEC_AUTO
    };

    (void) std::printf("######### Scan %" PRIi32 " BSS ##########\n", static_cast<std::int32_t>(count));
    for (auto i = 0; i < count; i++) {
#define AP_SSID_LENGTH sizeof((static_cast<EmwApiBase::ApInfo_t *>(0))->ssid)

      APs[i].ssid[AP_SSID_LENGTH - 1] = '\0';

      (void) std::printf("%2" PRIi32 "\t%32s ch %2" PRIi32 " rss %" PRIi32 " Security %10s"
                         " bssid %02x.%02x.%02x.%02x.%02x.%02x\n",
                         static_cast<std::int32_t>(i), APs[i].ssid, APs[i].channel, APs[i].rssi,
                         SecurityToString(emw_security[APs[i].security]),
                         APs[i].bssid[0], APs[i].bssid[1], APs[i].bssid[2], APs[i].bssid[3], APs[i].bssid[4],
                         APs[i].bssid[5]);
    }
    (void) std::printf("######### End of Scan ##########\n");
  }
  else {
    (void) std::printf("WiFiNetwork::scan(): operation failed (%" PRIi32 ")!\n", static_cast<std::int32_t>(emw_status));
  }
  return status;
}

std::int32_t WiFiNetwork::DeleteTransmitFifo(void) noexcept
{
  WiFiNetwork::TransmitThreadQuitFlag = true;

  while (WiFiNetwork::TransmitThreadQuitFlag) {
    /* Be cooperative. */
    vTaskDelay(50U);
  }
  return 0;
}

netif_ext_callback_t WiFiNetwork::NetifExtCallback;

void WiFiNetwork::NetifExtCallbackFunction(struct netif *netifPtr, netif_nsc_reason_t reason,
    const netif_ext_callback_args_t *argsPtr) noexcept
{
  static_cast<void>(netifPtr);
  static_cast<void>(argsPtr);
  static_cast<void>(reason);

  STD_PRINTF("WiFiNetwork::NetifExtCallbackFunction() -> (%04" PRIx32 ") %s\n",
             static_cast<std::uint32_t>(reason), WiFiNetwork::netifReasonToString(reason))

  if (nullptr != argsPtr) {
    STD_PRINTF("Link state  : %02" PRIx32 "\n", static_cast<std::uint32_t>(argsPtr->link_changed.state))
    STD_PRINTF("Status state: %02" PRIx32 "\n", static_cast<std::uint32_t>(argsPtr->status_changed.state))
  }
}

class EmwApiEmwBypass WiFiNetwork::Driver;
sys_mbox_t WiFiNetwork::TransmitFifo = {nullptr};
volatile bool WiFiNetwork::TransmitThreadQuitFlag;
class WiFiNetwork *WiFiNetwork::WiFiNetworks[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX] = { nullptr, nullptr };

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

static const char *SecurityToString(enum EmwApiBase::SecurityType security)
{
  const char *string_ptr;

  if (security == EmwApiBase::eSEC_NONE) {
    string_ptr = "Open";
  }
  else if (security == EmwApiBase::eSEC_WEP) {
    string_ptr = "WEP-shared";
  }
  else if (security == EmwApiBase::eSEC_WPA_TKIP) {
    string_ptr = "WPA-TKIP";
  }
  else if (security == EmwApiBase::eSEC_WPA_AES) {
    string_ptr = "WPA-AES";
  }
  else if (security == EmwApiBase::eSEC_WPA2_TKIP) {
    string_ptr = "WPA2-TKIP";
  }
  else if (security == EmwApiBase::eSEC_WPA2_AES) {
    string_ptr = "WPA2-AES";
  }
  else if (security == EmwApiBase::eSEC_WPA2_MIXED) {
    string_ptr = "WPA2_Mixed";
  }
  else if (security == EmwApiBase::eSEC_WPA3) {
    string_ptr = "WPA3";
  }
  else if (security == EmwApiBase::eSEC_AUTO) {
    string_ptr = "Auto";
  }
  else {
    string_ptr = "Unknown";
  }
  return string_ptr;
}


extern "C" {
  err_t InitializeWiFiNetif(struct netif *netifPtr)
  {
    err_t status = (err_t) ERR_ARG;

    STD_PRINTF("InitializeWiFiNetif()>\n")

    if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
      class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::MySelf(netifPtr);

      if (0 == wifi_network_ptr->startDriver()) {
        {
          static bool ext_callback_added_done = false;

          if (!ext_callback_added_done) {
            netif_add_ext_callback(&WiFiNetwork::NetifExtCallback, WiFiNetwork::NetifExtCallbackFunction);
            ext_callback_added_done = true;
          }
        }
        wifi_network_ptr->initializeNetif(netifPtr);
        status = static_cast<err_t>(ERR_OK);
      }
    }
    STD_PRINTF("InitializeWiFiNetif()<\n\n")
    return status;
  }
}

extern "C" {
  void UnInitializeWiFiNetif(struct netif *netifPtr)
  {
    STD_PRINTF("UnInitializeWiFiNetif()>\n")

    if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
      class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::MySelf(netifPtr);

      wifi_network_ptr->stopDriver();
      {
        static bool ext_callback_removed_done = false;
        if (!ext_callback_removed_done) {
          netif_remove_ext_callback(&WiFiNetwork::NetifExtCallback);
          ext_callback_removed_done = true;
        }
      }
      wifi_network_ptr->unInitializeNetif();
    }
    STD_PRINTF("UnInitializeWiFiNetif()<\n\n")
  }
}
