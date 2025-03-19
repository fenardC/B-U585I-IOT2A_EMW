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
#include "lwip/sys.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif /* LWIP_DHCP */
#include "lwip/etharp.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

WiFiNetwork::WiFiNetwork(EmwApiBase::EmwInterface interface, struct netif &aNetif)
  : credentials{nullptr, nullptr, 0}
  , deviceIdentifierString{0}
  , deviceNameString{0}
  , deviceVersionString{0}
  , dhcpInformFlag(true)
  , dhcpReleaseOnLinkLost(false)
  , driverAccessChannel(0U)
  , ipAcquired(false)
  , ipAddress{0}
  , gatewayAddress{0}
  , maskAddress{0}
  , staticIpAddress{0}
  , staticDnsServerAddress{0}
  , staticGatewayAddress{0}
  , staticMaskAddress{0}
  , driverInterface(interface)
  , driverInterfaceUp(false)
  , netInterface(aNetif)
{
  STD_PRINTF("WiFiNetwork::WiFiNetwork()>\n")
  std::printf("%s\n", WiFiNetwork::driver.getConfigurationString());
  STD_PRINTF("WiFiNetwork::WiFiNetwork()<\n\n")
}

WiFiNetwork::~WiFiNetwork(void)
{
  STD_PRINTF("WiFiNetwork::~WiFiNetwork()>\n")
  STD_PRINTF("WiFiNetwork::~WiFiNetwork<()\n\n")
}

void WiFiNetwork::initializeNetif(struct netif *netifPtr)
{
  STD_PRINTF("WiFiNetwork::initializeNetif()>\n")
  netifPtr->output = etharp_output;
  netifPtr->linkoutput = WiFiNetwork::outputToDriver;
  netifPtr->mtu = (u16_t) EmwNetworkStack::NETWORK_MTU_SIZE;
  netifPtr->flags = (u8_t)(NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET);
  netif_set_status_callback(netifPtr, WiFiNetwork::netifStatusCallback);
  netif_set_link_callback(netifPtr, WiFiNetwork::netifStatusCallback);
  STD_PRINTF("WiFiNetwork::initializeNetif()<\n")
}

int32_t WiFiNetwork::startDriver(void)
{
  int32_t status;
  STD_PRINTF("WiFiNetwork::startDriver()>\n")

  if (0U == WiFiNetwork::driver.runtime.interfaces) {
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startDriver(): REBOOT(HW) ...\n", HAL_GetTick())

    (void) WiFiNetwork::driver.resetHardware();
    if ((err_t)ERR_OK == sys_mbox_new(&WiFiNetwork::transmitFifo, EMW_MAX_TX_BUFFER_COUNT)) {
#if (configQUEUE_REGISTRY_SIZE > 0)
      static const char transmit_fifo_name[] = {"WiFiNetwork::FIFO"};
      vQueueAddToRegistry((QueueHandle_t)WiFiNetwork::transmitFifo.mbx, transmit_fifo_name);
#endif /* configQUEUE_REGISTRY_SIZE*/
    }
    else {
      LWIP_ASSERT("Creation of the transmit FIFO for the Wi-Fi device failed", false);
    }
    {
      static const char transmit_fifo_thread_name[] = {"NETIF_SendThread"};
      const int32_t stack_size_x32bits = (int32_t)(EMW_TRANSMIT_THREAD_STACK_SIZE);

      (void) sys_thread_new(transmit_fifo_thread_name,
                            (lwip_thread_fn)WiFiNetwork::tranmitThreadFunction, this,
                            stack_size_x32bits, EMW_TRANSMIT_THREAD_PRIORITY);
    }
  }

  if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::driver.initialize()) {
    LWIP_ASSERT("Initializing the Wi-Fi module failed", false);
    status = -1;
  }
  else if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::driver.setMode(1, WiFiNetwork::inputFromDriver, this)) {
    LWIP_ASSERT("Setting bypass for the Wi-Fi module failed", false);
    status = -1;
  }
  else {
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startDriver(): EMW Init [OK]\n", HAL_GetTick())

    (void) strncpy(this->deviceNameString, WiFiNetwork::driver.systemInformations.productName,
                   sizeof(this->deviceNameString));
    this->deviceNameString[sizeof(this->deviceNameString) - 1] = '\0';

    (void) strncpy(this->deviceIdentifierString, WiFiNetwork::driver.systemInformations.productIdentifier,
                   sizeof(this->deviceIdentifierString));
    this->deviceIdentifierString[sizeof(this->deviceIdentifierString) - 1] = '\0';

    WiFiNetwork::driver.getVersion(this->deviceVersionString, (uint32_t) sizeof(this->deviceVersionString));
    this->deviceVersionString[sizeof(this->deviceVersionString) - 1] = '\0';
    status = 0;
  }

  STD_PRINTF("WiFiNetwork::startDriver()<\n\n")
  return status;
}

int32_t WiFiNetwork::startSoftAccesPoint(void)
{
  int32_t status = 0;
  EmwApiBase::SoftApSettings_t access_point_settings;
  EmwApiCore::MacAddress_t mac_address_48bits;
  STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startSoftAccesPoint()>\n", HAL_GetTick())

  WiFiNetwork::wiFiNetworks[EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX] = this;
  (void) strncpy(access_point_settings.ssidString, this->credentials.ssidStringPtr,
                 sizeof(access_point_settings.ssidString) - 1);
  access_point_settings.ssidString[sizeof(access_point_settings.ssidString) - 1] = '\0';
  (void) strncpy(access_point_settings.passwordString, this->credentials.pskStringPtr,
                 sizeof(access_point_settings.passwordString) - 1);
  access_point_settings.passwordString[sizeof(access_point_settings.passwordString) - 1] = '\0';
  access_point_settings.channel = this->driverAccessChannel;
  (void) snprintf(access_point_settings.ip.ipAddressLocal, sizeof(access_point_settings.ip.ipAddressLocal),
                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                  (uint32_t)ip4_addr1_val(this->staticIpAddress),
                  (uint32_t)ip4_addr2_val(this->staticIpAddress),
                  (uint32_t)ip4_addr3_val(this->staticIpAddress),
                  (uint32_t)ip4_addr4_val(this->staticIpAddress));
  (void) snprintf(access_point_settings.ip.gatewayAddress, sizeof(access_point_settings.ip.gatewayAddress),
                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                  (uint32_t)ip4_addr1_val(this->staticGatewayAddress),
                  (uint32_t)ip4_addr2_val(this->staticGatewayAddress),
                  (uint32_t)ip4_addr3_val(this->staticGatewayAddress),
                  (uint32_t)ip4_addr4_val(this->staticGatewayAddress));
  (void) snprintf(access_point_settings.ip.networkMask, sizeof(access_point_settings.ip.networkMask),
                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                  (uint32_t)ip4_addr1_val(this->staticMaskAddress),
                  (uint32_t)ip4_addr2_val(this->staticMaskAddress),
                  (uint32_t)ip4_addr3_val(this->staticMaskAddress),
                  (uint32_t)ip4_addr4_val(this->staticMaskAddress));
  (void) snprintf(access_point_settings.ip.dnsServerAddress, sizeof(access_point_settings.ip.dnsServerAddress),
                  "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "",
                  (uint32_t)ip4_addr1_val(this->staticDnsServerAddress),
                  (uint32_t)ip4_addr2_val(this->staticDnsServerAddress),
                  (uint32_t)ip4_addr3_val(this->staticDnsServerAddress),
                  (uint32_t)ip4_addr4_val(this->staticDnsServerAddress));
  if (EmwApiBase::eEMW_STATUS_OK \
      != WiFiNetwork::driver.registerStatusCallback(&WiFiNetwork::informOfDriverStatus, this, EmwApiBase::eSOFTAP)) {
    LWIP_ASSERT("Registering to the Wi-Fi module failed", false);
    status = -1;
  }
  else {
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startSoftAccesPoint()> Setting AP ... \"%s\"\n",
               HAL_GetTick(), access_point_settings.ssidString)

    this->driverInterfaceUp = false;
    (void) WiFiNetwork::driver.startSoftAp(&access_point_settings);
    {
      const uint32_t tick_start = HAL_GetTick();

      while (!this->driverInterfaceUp) {
        if (this->checkTimeout(tick_start, 10000U)) {
          status = -1;
          break;
        }
        else {
          /* Be cooperative. */
          sys_msleep(10U);
        }
      }
    }
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startSoftAccesPoint(): done\n", HAL_GetTick())
  }

  if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::driver.getSoftApMacAddress(mac_address_48bits)) {
    LWIP_ASSERT("Cannot get the Software enabled Access Point MAC address of the Wi-Fi module", false);
    status = -1;
  }
  else {
    (void) memcpy(this->netInterface.hwaddr, mac_address_48bits.bytes, sizeof(this->netInterface.hwaddr));
    this->netInterface.hwaddr_len = sizeof(this->netInterface.hwaddr);

    STD_PRINTF("WiFiNetwork::startSoftAccesPoint(): MAC address %X.%X.%X.%X.%X.%X\n",
               this->netInterface.hwaddr[0], this->netInterface.hwaddr[1], this->netInterface.hwaddr[2],
               this->netInterface.hwaddr[3], this->netInterface.hwaddr[4], this->netInterface.hwaddr[5])
  }
  return status;
}

int32_t WiFiNetwork::startStation(void)
{
  int32_t status = 0;
  EmwApiCore::MacAddress_t mac_address_48bits;

  STD_PRINTF("WiFiNetwork::startStation()>\n")

  WiFiNetwork::wiFiNetworks[EmwApiBase::eWIFI_INTERFACE_STATION_IDX] = this;
  WiFiNetwork::driver.stationSettings.dhcpIsEnabled = true;
  if (EmwApiBase::eEMW_STATUS_OK \
      != WiFiNetwork::driver.registerStatusCallback(WiFiNetwork::informOfDriverStatus, this, EmwApiBase::eSTATION)) {
    LWIP_ASSERT("Registering to the Wi-Fi module failed", false);
    status = -1;
  }
  else if (EmwApiBase::eEMW_STATUS_OK != WiFiNetwork::driver.getStationMacAddress(mac_address_48bits)) {
    LWIP_ASSERT("Cannot get the MAC address of the Wi-Fi module", false);
    status = -1;
  }
  else {
    (void) memcpy(this->netInterface.hwaddr, mac_address_48bits.bytes, sizeof(this->netInterface.hwaddr));
    this->netInterface.hwaddr_len = sizeof(this->netInterface.hwaddr);

    STD_PRINTF("WiFiNetwork::startStation()(): MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
               this->netInterface.hwaddr[0], this->netInterface.hwaddr[1], this->netInterface.hwaddr[2],
               this->netInterface.hwaddr[3], this->netInterface.hwaddr[4], this->netInterface.hwaddr[5])
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startStation(): Joining ... \"%s\"\n",
               HAL_GetTick(), this->credentials.ssidStringPtr)

    (void) WiFiNetwork::driver.connect(this->credentials.ssidStringPtr, this->credentials.pskStringPtr,
                                       EmwApiBase::eSEC_AUTO);
    {
      const uint32_t tick_start = HAL_GetTick();

      while (!this->driverInterfaceUp) {
        if (this->checkTimeout(tick_start, 10000U)) {
          status = -1;
          break;
        }
        else {
          /* Be cooperative. */
          sys_msleep(10U);
        }
      }
    }
  }
  STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::startStation()< done\n", HAL_GetTick())
  return status;
}

int32_t WiFiNetwork::stopDriver(void)
{
  STD_PRINTF("WiFiNetwork::stopDriver()>\n")

  if (EmwApiBase::eSTATION == this->driverInterface) {
    (void) WiFiNetwork::driver.disconnect();
    (void) WiFiNetwork::driver.unRegisterStatusCallback(EmwApiBase::eSTATION);
  }
  else {
    (void) WiFiNetwork::driver.stopSoftAp();
    (void) WiFiNetwork::driver.unRegisterStatusCallback(EmwApiBase::eSOFTAP);
  }
  STD_PRINTF("WiFiNetwork::stopDriver()<\n")
  return 0;
}

int32_t WiFiNetwork::unInitializeDriver(void)
{
  STD_PRINTF("WiFiNetwork::unInitializeDriver()>\n")

  if (1U == WiFiNetwork::driver.runtime.interfaces) {
    (void) WiFiNetwork::driver.setMode(0, nullptr, nullptr);
  }
  (void) WiFiNetwork::driver.unInitialize();
  STD_PRINTF("WiFiNetwork::unInitializeDriver()<\n")
  return 0;
}

void WiFiNetwork::unInitializeNetif(void)
{
  STD_PRINTF("WiFiNetwork::unInitializeNetif()>\n")

  this->unInitializeDriver();
  if (0U == WiFiNetwork::driver.runtime.interfaces) {
    WiFiNetwork::deleteTransmitFifo();
    sys_mbox_free(&this->transmitFifo);
  }
  STD_PRINTF("WiFiNetwork::unInitializeNetif()<\n")
}

bool WiFiNetwork::checkTimeout(uint32_t tickStart, uint32_t tickCount) const
{
  const uint32_t elapsed_ticks = HAL_GetTick() - tickStart;
  bool status;

  if (elapsed_ticks > tickCount) {
    status = true;
  }
  else {
    status = false;
  }
  return status;
}

void WiFiNetwork::informOfDriverStatus(EmwApiBase::EmwInterface interface, enum EmwApiBase::WiFiEvent status,
                                       void *argPtr)
{
  class WiFiNetwork *const wifi_network_ptr = static_cast<class WiFiNetwork *>(argPtr);

  if (EmwApiBase::eSTATION == interface) {
    switch (status) {
      case EmwApiBase::eWIFI_EVENT_STA_DOWN: {
          std::printf("[%6" PRIu32 "] WiFiNetwork::informOfDriverStatus(): " \
                      "-> EmwApiBase::eWIFI_EVENT_STA_DOWN\n", HAL_GetTick());
          wifi_network_ptr->driverInterfaceUp = false;
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_UP: {
          std::printf("[%6" PRIu32 "] WiFiNetwork::informOfDriverStatus(): " \
                      "-> EmwApiBase::eWIFI_EVENT_STA_UP\n", HAL_GetTick());
          wifi_network_ptr->driverInterfaceUp = true;
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_GOT_IP: {
          std::printf("[%6" PRIu32 "] WiFiNetwork::informOfDriverStatus(): " \
                      "-> EmwApiBase::eWIFI_EVENT_STA_GOT_IP\n", HAL_GetTick());
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
          std::printf("[%6" PRIu32 "] WiFiNetwork::informOfDriverStatus(): " \
                      "-> EmwApiBase::eWIFI_EVENT_AP_DOWN\n", HAL_GetTick());
          wifi_network_ptr->driverInterfaceUp = false;
          break;
        }
      case EmwApiBase::eWIFI_EVENT_AP_UP: {
          std::printf("[%6" PRIu32 "] WiFiNetwork::informOfDriverStatus(): " \
                      "-> EmwApiBase::eWIFI_EVENT_AP_UP\n", HAL_GetTick());
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
}

void WiFiNetwork::inputFromDriver(void *bufferPtr, void *argPtr)
{
  if ((nullptr != argPtr) && (nullptr != bufferPtr)) {
    EmwNetworkStack::Buffer_t *const network_buffer_ptr = static_cast<EmwNetworkStack::Buffer_t *>(bufferPtr);

    if (0U < EmwNetworkStack::getBufferPayloadSize(network_buffer_ptr)) {
      const uint8_t low_level_netif_idx = (uint8_t)(* (static_cast<uint32_t *>(argPtr)));
      struct netif &net_interface \
        = WiFiNetwork::wiFiNetworks[(low_level_netif_idx == EmwApiBase::eWIFI_INTERFACE_STATION_IDX) ? \
                                    EmwApiBase::eWIFI_INTERFACE_STATION_IDX : EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX]->netInterface;
      WiFiNetwork::inputNetif(net_interface, static_cast<struct pbuf*>(bufferPtr));
    }
    else {
      EmwNetworkStack::freeBuffer(network_buffer_ptr);
    }
  }
}

void WiFiNetwork::inputNetif(struct netif & aNetif, struct pbuf * bufferPtr)
{
  const struct eth_hdr *const ethernet_header_ptr = static_cast<struct eth_hdr *>(bufferPtr->payload);
  const uint16_t ethertype = lwip_htons(ethernet_header_ptr->type);
  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetwork::inputNetif()> 0x%02" PRIx32 "\n", (uint32_t)ethertype));
  switch (ethertype) {
    case ETHTYPE_IP:
    case ETHTYPE_ARP:
    case ETHTYPE_IPV6:
#if PPPOE_SUPPORT
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
      {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetwork::inputNetif(): -> input packet 0x%02" PRIx32 " (%" PRIu32 ")\n",
                    (uint32_t)ethertype, (uint32_t)bufferPtr->tot_len));
#if defined(ENABLE_DEBUG)
        {
          std::printf(">\n");
          for (uint32_t i = 0; i < bufferPtr->tot_len; i++) {
            const uint8_t rx_data = (static_cast<uint8_t *>(bufferPtr->payload))[i];
            std::printf("%c", isprint(rx_data) ? rx_data : '*');
          }
          std::printf("<\n");
        }
#endif /* ENABLE_DEBUG */
        if (aNetif.input(bufferPtr, &aNetif) != (err_t)ERR_OK) {
          LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, ("WiFiNetwork::inputNetif(): netifPtr->input() failed\n"));
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

err_t WiFiNetwork::outputToDriver(struct netif * netifPtr, struct pbuf * bufferPtr)
{
  err_t status = (err_t) ERR_ARG;
  if ((nullptr != netifPtr) && (nullptr != netifPtr->state) && (nullptr != bufferPtr)) {
    const class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::mySelf(netifPtr);
    pbuf_ref(bufferPtr);
    LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                ("WiFiNetwork::outputToDriver(): Transmit buffer %p next=%p  tot_len=%" PRIu32 " len=%" PRIu32 "\n",
                 static_cast<void *>(bufferPtr), static_cast<void *>(bufferPtr->next),
                 static_cast<uint32_t>(bufferPtr->tot_len), static_cast<uint32_t>(bufferPtr->len)));
    if (wifi_network_ptr->driverInterface == EmwApiBase::eSTATION) {
      bufferPtr->if_idx = (uint8_t)EmwApiBase::eWIFI_INTERFACE_STATION_IDX;
    }
    else {
      bufferPtr->if_idx = (uint8_t)EmwApiBase::eWIFI_INTERFACE_SOFTAP_IDX;
    }
    if ((nullptr != bufferPtr->next) || (bufferPtr->tot_len != bufferPtr->len)) {
      struct pbuf *buf_send_ptr = pbuf_clone(PBUF_RAW, PBUF_RAM, bufferPtr);
      (void) pbuf_free(bufferPtr);
      if (nullptr == buf_send_ptr) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("WiFiNetwork::outputToDriver(): No memory for chained buffer!!!"));
        status = (err_t)ERR_MEM;
      }
      else {
        (void) sys_mbox_post(&WiFiNetwork::transmitFifo, buf_send_ptr);
        status = (err_t)ERR_OK;
#if defined(ENABLE_DEBUG)
        {
          std::printf(">\n");
          for (uint32_t i = 0; i < buf_send_ptr->tot_len; i++) {
            const uint8_t tx_data = (static_cast<uint8_t *>(buf_send_ptr->payload))[i];
            std::printf("%x", tx_data);
          }
          std::printf("<\n");
        }
#endif /* ENABLE_DEBUG */
      }
    }
    else {
      (void)sys_mbox_post(&WiFiNetwork::transmitFifo, bufferPtr);
      status = (err_t)ERR_OK;
    }
  }
  else {
    status = (err_t)ERR_INPROGRESS;
  }
  return status;
}

void WiFiNetwork::pushToDriver(uint32_t timeoutInMs)
{
  void *message_ptr = nullptr;
  if (SYS_ARCH_TIMEOUT != sys_arch_mbox_fetch(&WiFiNetwork::transmitFifo, &message_ptr, timeoutInMs)) {
    if (nullptr != message_ptr) {
      struct pbuf *const packet_ptr = static_cast<struct pbuf *>(message_ptr);
      const EmwApiBase::Status ret = WiFiNetwork::driver.output(static_cast<uint8_t*>(packet_ptr->payload),
                                     (int32_t)packet_ptr->len,
                                     (int32_t)packet_ptr->if_idx);
      LWIP_ASSERT("driver.output() failed", EmwApiBase::eEMW_STATUS_OK == ret);
      (void) pbuf_free(packet_ptr);
    }
  }
}

void WiFiNetwork::tranmitThreadFunction(void *THIS)
{
  class WiFiNetwork * const wifi_network_ptr = static_cast<class WiFiNetwork *>(THIS);
  setbuf(stdout, NULL);
  WiFiNetwork::transmitThreadQuitFlag = false;
  while (!WiFiNetwork::transmitThreadQuitFlag) {
    wifi_network_ptr->pushToDriver(500);
  }
  WiFiNetwork::transmitThreadQuitFlag = false;
  vTaskDelete(NULL);
  for (;;);
}

void WiFiNetwork::netifStatusCallback(struct netif * netifPtr)
{
  setbuf(stdout, nullptr);
  if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
    static bool ipv4_flag = false;
    class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::mySelf(netifPtr);
    STD_PRINTF("[%6" PRIu32 "] WiFiNetwork::netifStatusCallback()>\n", HAL_GetTick())
#if LWIP_DHCP
    {
      const ip_addr_t IP_ADDRESS_ZERO = IPADDR4_INIT(0);

      /* Lost connection, release the IPv4 address. */
      if ((memcmp(&netifPtr->ip_addr, &IP_ADDRESS_ZERO, sizeof(netifPtr->ip_addr)) != 0) \
          && (!netif_is_link_up(netifPtr)) \
          && wifi_network_ptr->dhcpReleaseOnLinkLost) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("[%6" PRIu32 "] WiFiNetwork::netifStatusCallback(): Calling dhcp_release()\n",
                     HAL_GetTick()));

        (void) dhcp_release_and_stop(netifPtr);

        if (netif_is_up(netifPtr)) {
          LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                      ("[%6" PRIu32 "] WiFiNetwork::netifStatusCallback(): Calling dhcp_start()\n",
                       HAL_GetTick()));

          (void) dhcp_start(netifPtr);
        }
      }

      /* Up connection, so need to inform other at first time for IPv4. */
      if (wifi_network_ptr->dhcpInformFlag && netif_is_link_up(netifPtr)) {
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE,
                    ("[%6" PRIu32 "] WiFiNetwork::netifStatusCallback(): Calling dhcp_inform()\n",
                     HAL_GetTick()));

        dhcp_inform(netifPtr);
        wifi_network_ptr->dhcpInformFlag = false;
      }
    }
#endif /* LWIP_DHCP */
    /* Update IPv4 address */
    if (!ip_addr_cmp(&wifi_network_ptr->ipAddress, &netifPtr->ip_addr)) {
      ip_addr_copy(wifi_network_ptr->ipAddress, netifPtr->ip_addr);
      ip_addr_copy(wifi_network_ptr->maskAddress, netifPtr->netmask);
      ip_addr_copy(wifi_network_ptr->gatewayAddress, netifPtr->gw);
      if (!ip_addr_isany_val(wifi_network_ptr->ipAddress)) {
        STD_PRINTF("WiFiNetwork::netifStatusCallback(): Got IPv4 address OK.\n")
        ipv4_flag = true;
      }
    }
    if (ipv4_flag) {
      wifi_network_ptr->ipAcquired = true;

      ipv4_flag = false;
    }
  }
}

void WiFiNetwork::checkIoSpeed(void)
{
  uint8_t *const ipc_test_data_ptr = static_cast<uint8_t *>(pvPortMalloc(EmwNetworkStack::NETWORK_BUFFER_SIZE));
  const uint16_t ipc_test_data_size = EmwNetworkStack::NETWORK_BUFFER_SIZE;

  if (nullptr != ipc_test_data_ptr) {
    uint32_t send_count = 0U;
    uint32_t receive_count = 0U;
    uint32_t time_ms_count;

    std::printf("\nAppWiFiLwip::checkIoSpeed()> (130 x (%" PRIu32 " + %" PRIu32 ")) ...\n",
                static_cast<uint32_t>(ipc_test_data_size), static_cast<uint32_t>(ipc_test_data_size));
    time_ms_count = HAL_GetTick();
    for (uint32_t i = 0U; i < 130U; i++) {
      uint16_t echoed_length = ipc_test_data_size;
      const EmwApiBase::Status status = WiFiNetwork::driver.testIpcEcho(ipc_test_data_ptr, ipc_test_data_size,
                                        ipc_test_data_ptr, &echoed_length, 5000U);

      if (EmwApiBase::eEMW_STATUS_OK != status) {
        std::printf("Failed to call testIpcEcho() (%" PRId32 ")\n", (int32_t)status);
        vPortFree(ipc_test_data_ptr);
        return;
      }
      else {
        send_count += ipc_test_data_size;
        receive_count += echoed_length;
      }
    }
    time_ms_count = HAL_GetTick() - time_ms_count;
    if (0 != time_ms_count) {
      const uint32_t total_sent_and_received = send_count + receive_count;
      const uint32_t speed = (8 * total_sent_and_received) / time_ms_count;

      std::printf("transfered: %" PRIu32 " bytes, time: %" PRIu32 " ms, Speed: %" PRIu32 " Kbps\n",
                  total_sent_and_received, time_ms_count, speed);
    }
    vPortFree(ipc_test_data_ptr);
  }
}

class WiFiNetwork *WiFiNetwork::mySelf(struct netif * netifPtr)
{
    return (class WiFiNetwork *)netifPtr->state;
}

void WiFiNetwork::scan(void)
{
  static EmwApiBase::ApInfo_t APs[10];
  const EmwApiBase::ApInfo_t ap_default;
  int8_t count = 0;
  EmwApiBase::Status status;

  status = WiFiNetwork::driver.scan(EmwApiBase::ePASSIVE, nullptr, 0);

  if (EmwApiBase::eEMW_STATUS_OK == status) {
    for (auto i = 0U; i < (sizeof(APs) / sizeof(APs[0])); i++) {
      APs[i] = ap_default;
    }
    count = WiFiNetwork::driver.getScanResults(reinterpret_cast<uint8_t *>(APs), 11);
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

    std::printf("######### Scan %" PRIi32 " BSS ##########\n", (int32_t)count);
    for (int32_t i = 0; i < count; i++) {
#define AP_SSID_LENGTH sizeof((static_cast<EmwApiBase::ApInfo_t *>(0))->ssid)

      APs[i].ssid[AP_SSID_LENGTH - 1] = '\0';

      std::printf("\t%2" PRIi32 "\t%40s ch %2" PRIi32 " rss %" PRIi32 " Security %10s country %4s"
                  " bssid %02x.%02x.%02x.%02x.%02x.%02x\n",
                  i, APs[i].ssid, APs[i].channel, APs[i].rssi,
                  WiFiNetwork::securityToString(emw_security[APs[i].security]), ".CN",
                  APs[i].bssid[0], APs[i].bssid[1], APs[i].bssid[2], APs[i].bssid[3], APs[i].bssid[4],
                  APs[i].bssid[5]);
    }
    std::printf("######### End of Scan ##########\n");
  }
  else {
    std::printf("WiFiNetwork::scan(): operation failed (%" PRIi32 ")!\n", (int32_t)status);
  }
}


const char *WiFiNetwork::securityToString(enum EmwApiBase::SecurityType security)
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

int32_t WiFiNetwork::deleteTransmitFifo(void)
{
  WiFiNetwork::transmitThreadQuitFlag = true;
  while (WiFiNetwork::transmitThreadQuitFlag) {
    /* Be cooperative. */
    sys_msleep(50U);
  }
  return 0;
}

#if LWIP_NETIF_EXT_STATUS_CALLBACK
netif_ext_callback_t WiFiNetwork::netifExtCallback;

void WiFiNetwork::netifExtCallbackFunction(struct netif * netifPtr, netif_nsc_reason_t reason,
    const netif_ext_callback_args_t *argsPtr)
{
  (void)netifPtr;
  (void)argsPtr;
  (void)reason;
  std::printf("[%6" PRIu32 "] WiFiNetwork::netifExtCallbackFunction()> -> (%04" PRIx32 ") %s\n",
              HAL_GetTick(), (uint32_t)reason, WiFiNetwork::netifReasonToString(reason));

  if (nullptr != argsPtr) {
    STD_PRINTF("Link state  : %02" PRIx32 "\n", (uint32_t)argsPtr->link_changed.state)
    STD_PRINTF("Status state: %02" PRIx32 "\n", (uint32_t)argsPtr->status_changed.state)
  }
}

#define CASE(x) case x: {return #x; break;}
#define DEFAULT default: {return "UNKNOWN"; break;}

const char *WiFiNetwork::netifReasonToString(netif_nsc_reason_t reason)
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
      DEFAULT;
  }
}
#endif /* LWIP_NETIF_EXT_STATUS_CALLBACK */

class EmwApiEmwBypass WiFiNetwork::driver;
sys_mbox_t WiFiNetwork::transmitFifo = {nullptr};
volatile bool WiFiNetwork::transmitThreadQuitFlag;
class WiFiNetwork *WiFiNetwork::wiFiNetworks[EmwApiBase::eWIFI_INTERFACE_COUNT_MAX] = { nullptr, nullptr };

extern "C" {
  err_t InitializeWiFiNetif(struct netif *netifPtr)
  {
    err_t status = (err_t) ERR_ARG;
    STD_PRINTF("InitializeWiFiNetif()>\n")

    if ((nullptr != netifPtr) && (nullptr != netifPtr->state)) {
      class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::mySelf(netifPtr);
      if (0 == wifi_network_ptr->startDriver()) {
#if LWIP_NETIF_EXT_STATUS_CALLBACK
        {
          static bool ext_callback_added_done = false;
          if (!ext_callback_added_done)
          {
            netif_add_ext_callback(&WiFiNetwork::netifExtCallback, WiFiNetwork::netifExtCallbackFunction);
            ext_callback_added_done = true;
          }
        }
#endif /* LWIP_NETIF_EXT_STATUS_CALLBACK */
        wifi_network_ptr->initializeNetif(netifPtr);
        status = (err_t) ERR_OK;
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
      class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::mySelf(netifPtr);
      wifi_network_ptr->stopDriver();
#if LWIP_NETIF_EXT_STATUS_CALLBACK
      {
        static bool ext_callback_removed_done = false;
        if (!ext_callback_removed_done) {
          netif_remove_ext_callback(&WiFiNetwork::netifExtCallback);
          ext_callback_removed_done = true;
        }
      }
#endif /* LWIP_NETIF_EXT_STATUS_CALLBACK */
      (void) wifi_network_ptr->unInitializeNetif();
    }
    STD_PRINTF("UnInitializeWiFiNetif()<\n\n")
  }
}
