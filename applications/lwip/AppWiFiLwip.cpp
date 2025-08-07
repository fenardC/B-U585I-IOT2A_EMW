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
#include "AppDhcpService.hpp"
#include "EmwAddress.hpp"
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
#include <cstdio>
#include <cstring>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

extern "C" void AppTaskFunction(void *argumentPtr);
static inline std::uint32_t MakeU32(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d);


static inline std::uint32_t MakeU32(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
{
  std::uint32_t value = static_cast<std::uint32_t>(a << 24) \
                        | static_cast<std::uint32_t>(b << 16) \
                        | static_cast<std::uint32_t>(c << 8) \
                        | static_cast<std::uint32_t>(d);
  return value;
}


AppWiFiLwip::AppWiFiLwip(void) noexcept
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
    static_cast<void>(argumentPtr);

    STD_PRINTF("\nAppTaskFunction()>\n")

#if LWIP_IPV4 && LWIP_IPV6
    (void) std::printf("LWIP_IPV4, LWIP_IPV6\n");
#endif /* LWIP_IPV4 && LWIP_IPV6 */
    (void) std::printf("NETWORK_BUFFER_SIZE: %6" PRIu32 "\n", (uint32_t)EmwNetworkStack::NETWORK_BUFFER_SIZE);
    (void) std::printf("MEM_SIZE           : %6" PRIu32 "\n", (uint32_t)MEM_SIZE);
    (void) std::printf("PBUF_POOL_BUFSIZE  : %6" PRIu32 "\n\n", (uint32_t)PBUF_POOL_BUFSIZE);

    tcpip_init(NULL, NULL);
    lwip_socket_thread_init();

    (void) std::printf("\nWi-Fi network interface initialization (SOFTAP)\n");
    the_application.constructSoftApNetworkInterface();

    (void) std::printf("\nWi-Fi network interface initialization (STATION)\n");
    the_application.constructStationNetworkInterface();

    WiFiNetwork::CheckIoSpeed();

    {
      static const char ssid[33] = {"MyHotSpot"};
      static const char psk[65] = {""};

      the_application.enableSoftAp(ssid, psk);
    }

    {
      (void) std::printf("\n Wi-Fi scan\n");
      class AppConsoleScan scan;

      if (0 != scan.execute(0, nullptr)) {
        (void) std::printf("%s: Wi-Fi scan failed\n", scan.getName());
      }
    }

    {
      {
        static const char ssid[33] = {WIFI_SSID};
        static const char psk[65] = {WIFI_PASSWORD};

        (void) std::printf("\n Wi-Fi connection\n");
        the_application.connectToAp(ssid, psk);
      }
      {
        class AppConsoleEcho echo;
        class AppConsoleIperf iperf(the_application.netifSTA);
        class AppConsolePing ping(the_application.netifSTA);
        class AppConsoleScan scan;
        class AppConsoleStats stats;
        class Cmd *cmds[] = {&echo, &iperf, &ping, &scan, &stats, nullptr};

        class Console the_console("app>", cmds);

        the_console.run();
      }
    }

    (void) std::printf("\nWi-Fi network interface un-initialization (STATION)\n");
    the_application.tearDownStationNetworkInterface();

    (void) std::printf("\nWi-Fi network interface un-initialization (SOFTAP)\n");
    the_application.tearDownSoftApNetworkInterface();
    lwip_socket_thread_cleanup();

    for (;;) {
      (void) std::printf(".");
      vTaskDelay(5000U);
    }
    STD_PRINTF("AppTaskFunction()<\n\n")
  }
}

void AppWiFiLwip::constructSoftApNetworkInterface(void) noexcept
{
  static const ip_addr_t IP_ADDRESS_FIXED = IPADDR4_INIT(lwip_htonl(MakeU32(10U, 10U, 10U, 1U)));
  static const ip_addr_t NETWORK_MASK = IPADDR4_INIT(lwip_htonl(MakeU32(255U, 255U, 255U, 0U)));

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
    const class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::MySelf(&this->netifSOFTAP);

    (void) std::printf("[%6" PRIu32 "] Wi-Fi driver ready (SOFTAP):\n", HAL_GetTick());
    (void) std::printf("          - Device Name    : %s.\n", wifi_network_ptr->deviceNameStringPtr);
    (void) std::printf("          - Device ID      : %s.\n", wifi_network_ptr->deviceIdentifierStringPtr);
    (void) std::printf("          - Device Version : %s.\n", wifi_network_ptr->deviceVersionString);
  }
  STD_PRINTF("AppWiFiLwip::constructSoftApNetworkInterface()<\n\n")
}

void AppWiFiLwip::constructStationNetworkInterface(void) noexcept
{
  static const ip_addr_t IP_ADDRESS_FIXED = IPADDR4_INIT(lwip_htonl(MakeU32(192, 168, 1, 113)));
  static const ip_addr_t NETWORK_MASK = IPADDR4_INIT(lwip_htonl(MakeU32(255U, 255U, 254U, 0U)));

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
    const class WiFiNetwork *const wifi_network_ptr = WiFiNetwork::MySelf(&this->netifSTA);
    (void) std::printf("[%6" PRIu32 "] Wi-Fi driver ready (STATION):\n", HAL_GetTick());
    (void) std::printf("          - Device Name    : %s.\n", wifi_network_ptr->deviceNameStringPtr);
    (void) std::printf("          - Device ID      : %s.\n", wifi_network_ptr->deviceIdentifierStringPtr);
    (void) std::printf("          - Device Version : %s.\n", wifi_network_ptr->deviceVersionString);
  }
  STD_PRINTF("AppWiFiLwip::constructStationNetworkInterface()<\n\n")
}

void AppWiFiLwip::connectToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  (void) std::printf("\nAppWiFiLwip::connectToAp()> joining \"%s\" with \"%s\" ...\n", ssidString, passwordString);

  if (0 != this->networkSTA.startConnectionToAp(ssidString, passwordString)) {
    (void) std::printf("[%6" PRIu32 "] Cannot Join ... \"%s\"\n", HAL_GetTick(), ssidString);
    ErrorHandler();
    return;
  }

  (void) std::printf("[%6" PRIu32 "] Wi-Fi interface ready (STATION):\n", HAL_GetTick());
  (void) std::printf("          - name        : \"%c%c\".\n", this->netifSTA.name[0], this->netifSTA.name[1]);
  (void) std::printf("          - hostname    : \"%s\".\n", this->netifSTA.hostname);
  (void) std::printf("          - mtu         : %" PRIu32 ".\n", static_cast<uint32_t>(this->netifSTA.mtu));
  (void) std::printf("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
                     this->netifSTA.hwaddr[0], this->netifSTA.hwaddr[1], this->netifSTA.hwaddr[2],
                     this->netifSTA.hwaddr[3], this->netifSTA.hwaddr[4], this->netifSTA.hwaddr[5]);

  netif_set_link_up(&this->netifSTA);
  netif_set_up(&this->netifSTA);
  (void) std::printf("[%6" PRIu32 "] Setting IPv6 link-local address\n", HAL_GetTick());
  netif_create_ip6_linklocal_address(&this->netifSTA, 1);

#if LWIP_IPV6_MLD
  this->netifSTA.flags |= NETIF_FLAG_MLD6;
  if (this->netifSTA.mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    this->netifSTA.mld_mac_filter(&this->netifSTA, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6_MLD */

  (void) std::printf("[%6" PRIu32 "] Calling dhcp_start()\n", HAL_GetTick());
  (void) dhcp_start(&this->netifSTA);
  while (!this->networkSTA.ipAcquired) {
    std::printf("c");
    /* Be cooperative. */
    vTaskDelay(100U);
  }

  (void) std::printf("\n[%6" PRIu32 "] Network interface connected (STATION):\n", HAL_GetTick());
  (void) std::printf("          - IP address      : %s\n", ipaddr_ntoa(&this->netifSTA.ip_addr));
  (void) std::printf("          - Netmask         : %s\n", ipaddr_ntoa(&this->netifSTA.netmask));
  (void) std::printf("          - GW address      : %s\n", ipaddr_ntoa(&this->netifSTA.gw));
  (void) std::printf("          - IP6 address (0) : %s [%" PRIu32 "]\n",
                     ipaddr_ntoa(&this->netifSTA.ip6_addr[0]), static_cast<std::uint32_t>(this->netifSTA.ip6_addr_state[0]));
  (void) std::printf("          - IP6 address (1) : %s [%" PRIu32 "]\n",
                     ipaddr_ntoa(&this->netifSTA.ip6_addr[1]), static_cast<std::uint32_t>(this->netifSTA.ip6_addr_state[1]));
  (void) std::printf("          - IP6 address (2) : %s [%" PRIu32 "]\n",
                     ipaddr_ntoa(&this->netifSTA.ip6_addr[2]), static_cast<std::uint32_t>(this->netifSTA.ip6_addr_state[2]));
  {
    const ip_addr_t ip_addr_dns =
      /* 2001:4860:4860::8888 google */
      IPADDR6_INIT(lwip_htonl(0x20014860UL), lwip_htonl(0x48600000UL), lwip_htonl(0x00000000UL), lwip_htonl(0x00008888UL));
    /* IPADDR4_INIT(PP_HTONL(0x01010101)); */

    dns_setserver(DNS_MAX_SERVERS - 1, &ip_addr_dns);
    for (std::uint8_t num_dns = 0U; num_dns < DNS_MAX_SERVERS; num_dns++) {
      const ip_addr_t *const ip_addr_ptr = dns_getserver(num_dns);
      (void) std::printf("          - DNS_%" PRIu32 " address   : %s\n", static_cast<std::uint32_t>(num_dns),
                         ipaddr_ntoa(ip_addr_ptr));
    }
  }
  STD_PRINTF("AppWiFiLwip::connectToAp()<\n\n")
}

void AppWiFiLwip::enableSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  (void) std::printf("\nStart Software enabled Access Point with \"%s\"\n", ssidString);

  if (0 != this->networkSOFTAP.startSoftAp(ssidString, passwordString)) {
    (void) std::printf("[%6" PRIu32 "] Failed to set up %s\n", HAL_GetTick(), ssidString);
    ErrorHandler();
    return;
  }
  (void) std::printf("[%6" PRIu32 "] Wi-Fi interface ready (SOFTAP):\n", HAL_GetTick());
  (void) std::printf("          - name        : \"%c%c\".\n", this->netifSOFTAP.name[0], this->netifSOFTAP.name[1]);
  (void) std::printf("          - hostname    : \"%s\".\n", this->netifSOFTAP.hostname);
  (void) std::printf("          - mtu         : %" PRIu32 ".\n", static_cast<uint32_t>(this->netifSOFTAP.mtu));
  (void) std::printf("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
                     this->netifSOFTAP.hwaddr[0], this->netifSOFTAP.hwaddr[1],
                     this->netifSOFTAP.hwaddr[2], this->netifSOFTAP.hwaddr[3],
                     this->netifSOFTAP.hwaddr[4], this->netifSOFTAP.hwaddr[5]);

  netif_set_link_up(&this->netifSOFTAP);
  netif_set_up(&this->netifSOFTAP);

  netif_set_addr(&this->netifSOFTAP,
                 ip_2_ip4(&this->networkSOFTAP.staticIpAddress),
                 ip_2_ip4(&this->networkSOFTAP.staticMaskAddress),
                 ip_2_ip4(&this->networkSOFTAP.staticGatewayAddress));

  (void) std::printf("\n[%6" PRIu32 "] Network interface ready (SOFTAP):\n", HAL_GetTick());
  (void) std::printf("          - IP address      : %s\n", ipaddr_ntoa(&this->netifSOFTAP.ip_addr));
  (void) std::printf("          - Netmask         : %s\n", ipaddr_ntoa(&this->netifSOFTAP.netmask));
  (void) std::printf("          - GW address      : %s\n", ipaddr_ntoa(&this->netifSOFTAP.gw));

  (void) std::printf("Starting the DHCP server ...\n");
  static AppDhcpService dhcp_server(&this->netifSOFTAP);
  if (0 != dhcp_server.createService()) {
    (void) std::printf("Cannot start the DHCP server\n");
    ErrorHandler();
    return;
  }
  STD_PRINTF("AppWiFiLwip::enableSoftAp()<\n\n")
}

void AppWiFiLwip::tearDownSoftApNetworkInterface(void) noexcept
{
  netif_remove(&this->netifSOFTAP);
  netif_set_remove_callback(&this->netifSOFTAP, nullptr);
#if LWIP_STATS_DISPLAY
  stats_display();
#endif /* LWIP_STATS_DISPLAY */
}

void AppWiFiLwip::tearDownStationNetworkInterface(void) noexcept
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
