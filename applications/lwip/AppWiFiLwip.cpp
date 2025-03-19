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
#include "AppWiFiLwip.hpp"
#include "AppConsoleEcho.hpp"
#include "AppConsoleIperf.hpp"
#include "AppConsolePing.hpp"
#include "AppConsoleScan.hpp"
#include "AppConsoleStats.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "EmwNetworkStack.hpp"
#include "emw_conf.hpp"
#include "main.hpp"
#include "lwip/api.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "stm32u5xx_hal.h"
#include "WiFiNetwork.hpp"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <cstdbool>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

extern "C" void AppTaskFunction(void *argumentPtr);

AppWiFiLwip::AppWiFiLwip(void)
  : netifSOFTAP()
  , netifSTA()
  , networkSOFTAP(EmwApiBase::eSOFTAP, netifSOFTAP)
  , networkSTA(EmwApiBase::eSTATION, netifSTA)
{
  STD_PRINTF("AppWiFiLwip::AppWiFiLwip()>\n")
  this->netifSOFTAP.name[0] = 'M';
  this->netifSOFTAP.name[1] = 'A';
  this->netifSTA.name[0] = 'M';
  this->netifSTA.name[1] = 'S';
  netif_set_hostname(&this->netifSOFTAP, "lwip-softap");
  netif_set_hostname(&this->netifSTA, "lwip-sta");
  STD_PRINTF("AppWiFiLwip::AppWiFiLwip()<\n\n")
}

AppWiFiLwip::~AppWiFiLwip(void)
{
  STD_PRINTF("AppWiFiLwip::~AppWiFiLwip()>\n")
  STD_PRINTF("AppWiFiLwip::~AppWiFiLwip()<\n\n")
}

extern "C" {
  void AppTaskFunction(void *argumentPtr)
  {
    AppWiFiLwip the_application;
    (void) argumentPtr;

    STD_PRINTF("\nAppTaskFunction()>\n")

#if LWIP_IPV4 && LWIP_IPV6
    std::printf("LWIP_IPV4, LWIP_IPV6\n");
#endif /* LWIP_IPV4 && LWIP_IPV6 */
    std::printf("NETWORK_BUFFER_SIZE: %6" PRIu32 "\n", (uint32_t)EmwNetworkStack::NETWORK_BUFFER_SIZE);
    std::printf("MEM_SIZE           : %6" PRIu32 "\n", (uint32_t)MEM_SIZE);
    std::printf("PBUF_POOL_BUFSIZE  : %6" PRIu32 "\n\n", (uint32_t)PBUF_POOL_BUFSIZE);

    tcpip_init(NULL, NULL);
    lwip_socket_thread_init();

    std::printf("\nWi-Fi network interface initialization (SOFTAP)\n");
    the_application.constructSoftApNetworkInterface();

    std::printf("\nWi-Fi network interface initialization (STATION)\n");
    the_application.constructStationNetworkInterface();

    WiFiNetwork::checkIoSpeed();

    the_application.enableSoftAp();

    {
      std::printf("\nWi-Fi scan\n");
      class AppConsoleScan scan(nullptr);

      if (0 != scan.execute(0, nullptr)) {
        std::printf("%s: Wi-Fi scan failed\n", scan.getName());
      }
    }

    {
      std::printf("\nWi-Fi connection\n");
      the_application.connectAp(WIFI_SSID, WIFI_PASSWORD);
      {
        class AppConsoleEcho echo(nullptr);
        class AppConsoleIperf iperf(&the_application.netifSTA);
        class AppConsolePing ping(&the_application.netifSTA);
        class AppConsoleScan scan(nullptr);
        class AppConsoleStats stats(nullptr);
        class Cmd *cmds[] = {&echo, &iperf, &ping, &scan, &stats, nullptr};
        class Console the_console("app>", cmds);

        the_console.run();
      }
    }

    std::printf("\nWi-Fi network interface un-initialization (STATION)\n");
    the_application.tearDownStationNetworkInterface();

    std::printf("\nWi-Fi network interface un-initialization (SOFTAP)\n");
    the_application.tearDownSoftApNetworkInterface();
    lwip_socket_thread_cleanup();
    for (;;) {
      std::printf(".");
      vTaskDelay(5000U);
    }
    STD_PRINTF("AppTaskFunction()<\n\n")
  }
}

void AppWiFiLwip::constructSoftApNetworkInterface(void)
{
  static const ip_addr_t IP_ADDRESS_FIXED = IPADDR4_INIT_BYTES(10U, 10U, 10U, 1U);
  static const ip_addr_t NETWORK_MASK = IPADDR4_INIT_BYTES(255U, 255U, 255U, 0U);
  STD_PRINTF("AppWiFiLwip::constructSoftApNetworkInterface()>\n")

  this->networkSOFTAP.staticIpAddress = IP_ADDRESS_FIXED;
  this->networkSOFTAP.staticGatewayAddress = IP_ADDRESS_FIXED;
  this->networkSOFTAP.staticMaskAddress = NETWORK_MASK;
  this->networkSOFTAP.dhcpInformFlag = false;
  this->networkSOFTAP.dhcpReleaseOnLinkLost = false;
  this->networkSOFTAP.driverAccessChannel = 6U;
  if (nullptr == netif_add_noaddr(&this->netifSOFTAP, &this->networkSOFTAP, &InitializeWiFiNetif, &tcpip_input)) {
    ErrorHandler();
    return;
  }
  netif_set_remove_callback(&this->netifSOFTAP, &UnInitializeWiFiNetif);
  {
    const class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::mySelf(&this->netifSOFTAP);

    std::printf("[%6" PRIu32 "] Wi-Fi driver ready (SOFTAP):\n", HAL_GetTick());
    std::printf("          - Device Name    : %s.\n", wifi_network_ptr->deviceNameStringPtr);
    std::printf("          - Device ID      : %s.\n", wifi_network_ptr->deviceIdentifierStringPtr);
    std::printf("          - Device Version : %s.\n", wifi_network_ptr->deviceVersionString);
  }
  STD_PRINTF("AppWiFiLwip::constructSoftApNetworkInterface()<\n\n")
}

void AppWiFiLwip::constructStationNetworkInterface(void)
{
  static const ip_addr_t IP_ADDRESS_FIXED = IPADDR4_INIT_BYTES(192, 168, 1, 113);
  static const ip_addr_t NETWORK_MASK = IPADDR4_INIT_BYTES(255, 255, 254, 0);
  STD_PRINTF("AppWiFiLwip::constructStationNetworkInterface()>\n")

  this->networkSTA.staticIpAddress = IP_ADDRESS_FIXED;
  this->networkSTA.staticGatewayAddress = IP_ADDRESS_FIXED;
  this->networkSTA.staticMaskAddress = NETWORK_MASK;
  this->networkSTA.dhcpInformFlag = true;
  this->networkSTA.dhcpReleaseOnLinkLost = false;
  this->networkSTA.driverAccessChannel = 8U;
  if (nullptr == netif_add_noaddr(&this->netifSTA, &this->networkSTA, &InitializeWiFiNetif, &tcpip_input)) {
    ErrorHandler();
    return;
  }
  netif_set_remove_callback(&this->netifSTA, &UnInitializeWiFiNetif);
  netif_set_default(&this->netifSTA);
  {
    const class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::mySelf(&this->netifSTA);
    std::printf("[%6" PRIu32 "] Wi-Fi driver ready (STATION):\n", HAL_GetTick());
    std::printf("          - Device Name    : %s.\n", wifi_network_ptr->deviceNameStringPtr);
    std::printf("          - Device ID      : %s.\n", wifi_network_ptr->deviceIdentifierStringPtr);
    std::printf("          - Device Version : %s.\n", wifi_network_ptr->deviceVersionString);
  }
  STD_PRINTF("AppWiFiLwip::constructStationNetworkInterface()<\n\n")
}

void AppWiFiLwip::enableSoftAp(void)
{
  STD_PRINTF("\nAppWiFiLwip::enableSoftAp()>\n")

  this->networkSOFTAP.credentials.ssidStringPtr = "MyHotSpot";
  this->networkSOFTAP.credentials.pskStringPtr = "";
  std::printf("\nStart Software enabled Access Point with \"%s\"\n", this->networkSOFTAP.credentials.ssidStringPtr);
  if (0 != this->networkSOFTAP.startSoftAccesPoint()) {
    std::printf("[%6" PRIu32 "] Failed to set up %s\n", HAL_GetTick(), this->networkSOFTAP.credentials.ssidStringPtr);
    ErrorHandler();
    return;
  }
  std::printf("[%6" PRIu32 "] Wi-Fi interface ready (SOFTAP):\n", HAL_GetTick());
  std::printf("          - name        : \"%c%c\".\n", this->netifSOFTAP.name[0], this->netifSOFTAP.name[1]);
  std::printf("          - hostname    : \"%s\".\n", this->netifSOFTAP.hostname);
  std::printf("          - mtu         : %" PRIu32 ".\n", static_cast<uint32_t>(this->netifSOFTAP.mtu));
  std::printf("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
              this->netifSOFTAP.hwaddr[0], this->netifSOFTAP.hwaddr[1],
              this->netifSOFTAP.hwaddr[2], this->netifSOFTAP.hwaddr[3],
              this->netifSOFTAP.hwaddr[4], this->netifSOFTAP.hwaddr[5]);

  netif_set_link_up(&this->netifSOFTAP);
  netif_set_up(&this->netifSOFTAP);

  netif_set_addr(&this->netifSOFTAP,
                 ip_2_ip4(&this->networkSOFTAP.staticIpAddress),
                 ip_2_ip4(&this->networkSOFTAP.staticMaskAddress),
                 ip_2_ip4(&this->networkSOFTAP.staticGatewayAddress));

  std::printf("\n[%6" PRIu32 "] Network Interface ready (SOFTAP):\n", HAL_GetTick());
  std::printf("          - IP address      : %s\n", ipaddr_ntoa(&this->netifSOFTAP.ip_addr));
  std::printf("          - Netmask         : %s\n", ipaddr_ntoa(&this->netifSOFTAP.netmask));
  std::printf("          - GW address      : %s\n", ipaddr_ntoa(&this->netifSOFTAP.gw));

  STD_PRINTF("AppWiFiLwip::enableSoftAp()<\n\n")
}

void AppWiFiLwip::connectAp(const char *ssidStringPtr, const char *passwordStringPtr)
{
  std::printf("\nAppWiFiLwip::connectAp()> joining \"%s\" with \"%s\" ...\n", ssidStringPtr, passwordStringPtr);

  this->networkSTA.credentials.ssidStringPtr = ssidStringPtr;
  this->networkSTA.credentials.pskStringPtr = passwordStringPtr;

  if (0 != this->networkSTA.startStation()) {
    std::printf("[%6" PRIu32 "] Cannot Join ... \"%s\"\n", HAL_GetTick(), this->networkSTA.credentials.ssidStringPtr);
    ErrorHandler();
    return;
  }

  std::printf("[%6" PRIu32 "] Wi-Fi interface ready (STATION):\n", HAL_GetTick());
  std::printf("          - name        : \"%c%c\".\n", this->netifSTA.name[0], this->netifSTA.name[1]);
  std::printf("          - hostname    : \"%s\".\n", this->netifSTA.hostname);
  std::printf("          - mtu         : %" PRIu32 ".\n", static_cast<uint32_t>(this->netifSTA.mtu));
  std::printf("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
              this->netifSTA.hwaddr[0], this->netifSTA.hwaddr[1], this->netifSTA.hwaddr[2],
              this->netifSTA.hwaddr[3], this->netifSTA.hwaddr[4], this->netifSTA.hwaddr[5]);

  netif_set_link_up(&this->netifSTA);
  netif_set_up(&this->netifSTA);
  std::printf("[%6" PRIu32 "] Setting IPv6 link-local address\n", HAL_GetTick());
  netif_create_ip6_linklocal_address(&this->netifSTA, 1);
#if LWIP_IPV6_MLD
  this->netifSTA.flags |= NETIF_FLAG_MLD6;
  if (this->netifSTA.mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    this->netifSTA.mld_mac_filter(&this->netifSTA, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6_MLD */

  std::printf("[%6" PRIu32 "] Calling dhcp_start()\n", HAL_GetTick());
  (void) dhcp_start(&this->netifSTA);
  while (!this->networkSTA.ipAcquired) {
    std::printf("c");
    /* Be cooperative. */
    vTaskDelay(100U);
  }

  std::printf("\n[%6" PRIu32 "] Network Interface connected (STATION):\n", HAL_GetTick());
  std::printf("          - IP address      : %s\n", ipaddr_ntoa(&this->netifSTA.ip_addr));
  std::printf("          - Netmask         : %s\n", ipaddr_ntoa(&this->netifSTA.netmask));
  std::printf("          - GW address      : %s\n", ipaddr_ntoa(&this->netifSTA.gw));
  std::printf("          - IP6 address (0) : %s [%" PRIu32 "]\n",
              ipaddr_ntoa(&this->netifSTA.ip6_addr[0]), static_cast<uint32_t>(this->netifSTA.ip6_addr_state[0]));
  std::printf("          - IP6 address (1) : %s [%" PRIu32 "]\n",
              ipaddr_ntoa(&this->netifSTA.ip6_addr[1]), static_cast<uint32_t>(this->netifSTA.ip6_addr_state[1]));
  std::printf("          - IP6 address (2) : %s [%" PRIu32 "]\n",
              ipaddr_ntoa(&this->netifSTA.ip6_addr[2]), static_cast<uint32_t>(this->netifSTA.ip6_addr_state[2]));
  {
    const ip_addr_t ip_addr_dns =
      /* 2001:4860:4860::8888 google */
      IPADDR6_INIT(PP_HTONL(0x20014860UL), PP_HTONL(0x48600000UL), PP_HTONL(0x00000000UL), PP_HTONL(0x00008888UL));
    //IPADDR4_INIT(PP_HTONL(0x01010101));

    dns_setserver(DNS_MAX_SERVERS - 1, &ip_addr_dns);
    for (uint8_t num_dns = 0U; num_dns < DNS_MAX_SERVERS; num_dns++) {
      const ip_addr_t *const ip_addr_ptr = dns_getserver(num_dns);
      std::printf("          - DNS_%" PRIu32 " address   : %s\n", static_cast<uint32_t>(num_dns), ipaddr_ntoa(ip_addr_ptr));
    }
  }
}

void AppWiFiLwip::tearDownSoftApNetworkInterface(void)
{
  netif_remove(&this->netifSOFTAP);
  netif_set_remove_callback(&this->netifSOFTAP, nullptr);
#if LWIP_STATS_DISPLAY
  stats_display();
#endif /* LWIP_STATS_DISPLAY */
}

void AppWiFiLwip::tearDownStationNetworkInterface(void)
{
  netif_remove(&this->netifSTA);
  netif_set_remove_callback(&this->netifSTA, nullptr);
#if LWIP_STATS_DISPLAY
  stats_display();
#endif /* LWIP_STATS_DISPLAY */
}

#if defined(LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS) && (LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS == 0)
u32_t sys_now(void)
{
  return HAL_GetTick();
}
#endif /* LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS == 0 */
