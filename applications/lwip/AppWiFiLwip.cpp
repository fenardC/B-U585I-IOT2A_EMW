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
#include "AppWiFiLwip.hpp"
#include "AppConsoleDownload.hpp"
#include "AppConsoleEcho.hpp"
#include "AppConsoleIperf.hpp"
#include "AppConsolePing.hpp"
#include "AppConsoleScan.hpp"
#include "AppConsoleStats.hpp"
#include "AppDhcpService.hpp"
#include "AppHttpSSE.hpp"
#include "EmwAddress.hpp"
#include "EmwNetworkStack.hpp"
#include "main.hpp"
#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS */
#include "wifi_emw.hpp"

#include "lwip/api.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

#define LWIP_PP_HTONL(x) \
  ((((x) & static_cast<u32_t>(0x000000ffUL)) << 24) | \
  (((x) & static_cast<u32_t>(0x0000ff00UL)) <<  8) |  \
  (((x) & static_cast<u32_t>(0x00ff0000UL)) >>  8) |  \
  (((x) & static_cast<u32_t>(0xff000000UL)) >> 24))

#define LWIP_IP6_ADDR_SET_ALLNODES_LINKLOCAL(ip6addr) \
do {                                                  \
  (ip6addr)->addr[0] = LWIP_PP_HTONL(0xff020000UL);   \
  (ip6addr)->addr[1] = 0;                             \
  (ip6addr)->addr[2] = 0;                             \
  (ip6addr)->addr[3] = LWIP_PP_HTONL(0x00000001UL);   \
  (ip6addr)->zone = IP6_NO_ZONE;                      \
} while(0)


static inline std::uint32_t MakeU32(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d);


static inline std::uint32_t MakeU32(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
{
  std::uint32_t value \
    = static_cast<std::uint32_t>(a << 24) | static_cast<std::uint32_t>(b << 16) \
      | static_cast<std::uint32_t>(c << 8) | static_cast<std::uint32_t>(d);
  return value;
}


AppWiFiLwip::AppWiFiLwip(class EmwApiEmwBypass &wifiEmw) noexcept
  : lwipNetifSoftAp()
  , lwipNetifStation()
  , emwNetworkSoftAp(lwipNetifSoftAp)
  , emwNetworkStation(lwipNetifStation)
{
  DEBUG_STD_PRINTF("AppWiFiLwip::AppWiFiLwip()>\n")

  this->lwipNetifSoftAp.name[0] = 'M';
  this->lwipNetifSoftAp.name[1] = 'A';
  this->lwipNetifSoftAp.state = &wifiEmw;
  this->lwipNetifStation.name[0] = 'M';
  this->lwipNetifStation.name[1] = 'S';
  this->lwipNetifStation.state = &wifiEmw;

  netif_set_hostname(&this->lwipNetifSoftAp, "lwip-softap");
  netif_set_hostname(&this->lwipNetifStation, "lwip-sta");
  DEBUG_STD_PRINTF("AppWiFiLwip::AppWiFiLwip()<\n\n")
}

AppWiFiLwip::~AppWiFiLwip(void)
{
  DEBUG_STD_PRINTF("AppWiFiLwip::~AppWiFiLwip()>\n")
  DEBUG_STD_PRINTF("AppWiFiLwip::~AppWiFiLwip()<\n\n")
}

extern "C" {
  void AppTaskFunction(void *argumentPtr)
  {
    class EmwApiEmwBypass &emw = *(reinterpret_cast<class EmwApiEmwBypass*>(argumentPtr));
    AppWiFiLwip the_application(emw);

    DEBUG_STD_PRINTF("\nAppTaskFunction()>\n")

    lwip_socket_thread_init();

    STD_PRINTF("\nWi-Fi network interface initialization (SOFTAP)\n");
    the_application.constructSoftApNetworkInterface();

    STD_PRINTF("\nWi-Fi network interface initialization (STATION)\n");
    the_application.constructStationNetworkInterface();

    {
      static const char ssid[33] = {"MyHotSpot"};
      static const char psk[65] = {""};

      the_application.enableSoftAp(ssid, psk);
    }

    STD_PRINTF("\n Wi-Fi scan\n");
    if (0 != Scan(emw)) {
      STD_PRINTF("Wi-Fi scan failed\n");
    }
    {
      static const char ssid[33] = {WIFI_SSID};
      static const char psk[65] = {WIFI_PASSWORD};

      class AppHttpSSE sse(the_application.lwipNetifSoftAp);
      sse.initializeServer(the_application.lwipNetifSoftAp.ip_addr.u_addr.ip4.addr, 80);
      STD_PRINTF("\nSSE Web server started (SOFTAP)\n");

      STD_PRINTF("\n Wi-Fi connection\n");
      the_application.connectToAp(ssid, psk);

      {
        class AppConsoleEcho echo;
        class AppConsoleDownload http;
        class AppConsoleIperf iperf(the_application.lwipNetifStation);
        class AppConsolePing ping(the_application.lwipNetifStation);
        class AppConsoleScan scan;
        class AppConsoleStats stats;
        class Cmd *cmds[] = {&echo, &http, &iperf, &ping, &scan, &stats, nullptr};
        class Console the_console("app>", cmds);

        the_console.run();
      }
    }

    STD_PRINTF("\nWi-Fi network interface un-initialization (STATION)\n");
    the_application.tearDownStationNetworkInterface();

    STD_PRINTF("\nWi-Fi network interface un-initialization (SOFTAP)\n");
    the_application.tearDownSoftApNetworkInterface();
    lwip_socket_thread_cleanup();

    for (;;) {
      STD_PRINTF(".");
      vTaskDelay(5000U);
    }
    DEBUG_STD_PRINTF("AppTaskFunction()<\n\n")
  }
}

void AppWiFiLwip::connectToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  STD_PRINTF("\nAppWiFiLwip::connectToAp()> joining \"%s\" with \"%s\" ...\n", ssidString, passwordString);

  if (0 != this->emwNetworkStation.activate(ssidString, passwordString)) {
    STD_PRINTF("[%6" PRIu32 "] Cannot Join ... \"%s\"\n", HAL_GetTick(), ssidString);
    ErrorHandler();
    return;
  }

  STD_PRINTF("[%6" PRIu32 "] Wi-Fi interface ready (STATION):\n", HAL_GetTick());
  STD_PRINTF("          - name        : \"%c%c\".\n", this->lwipNetifStation.name[0], this->lwipNetifStation.name[1]);
  STD_PRINTF("          - hostname    : \"%s\".\n", this->lwipNetifStation.hostname);
  STD_PRINTF("          - mtu         : %" PRIu32 ".\n", static_cast<uint32_t>(this->lwipNetifStation.mtu));
  STD_PRINTF("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
             this->lwipNetifStation.hwaddr[0], this->lwipNetifStation.hwaddr[1], this->lwipNetifStation.hwaddr[2],
             this->lwipNetifStation.hwaddr[3], this->lwipNetifStation.hwaddr[4], this->lwipNetifStation.hwaddr[5]);

  netif_set_link_up(&this->lwipNetifStation);
  netif_set_up(&this->lwipNetifStation);
  STD_PRINTF("[%6" PRIu32 "] Setting IPv6 link-local address\n", HAL_GetTick());
  netif_create_ip6_linklocal_address(&this->lwipNetifStation, 1);

#if LWIP_IPV6_MLD
  this->lwipNetifStation.flags |= NETIF_FLAG_MLD6;
  if (this->lwipNetifStation.mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    LWIP_IP6_ADDR_SET_ALLNODES_LINKLOCAL(&ip6_allnodes_ll);
    this->lwipNetifStation.mld_mac_filter(&this->lwipNetifStation, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6_MLD */

  STD_PRINTF("[%6" PRIu32 "] Calling dhcp_start()\n", HAL_GetTick());
  static_cast<void>(dhcp_start(&this->lwipNetifStation));
  while (!this->emwNetworkStation.ipAcquired) {
    STD_PRINTF("c");
    /* Be cooperative. */
    vTaskDelay(100U);
  }

  STD_PRINTF("\n[%6" PRIu32 "] Network interface connected (STATION):\n", HAL_GetTick());
  STD_PRINTF("          - IP address      : %s\n", ipaddr_ntoa(&this->lwipNetifStation.ip_addr));
  STD_PRINTF("          - Netmask         : %s\n", ipaddr_ntoa(&this->lwipNetifStation.netmask));
  STD_PRINTF("          - GW address      : %s\n", ipaddr_ntoa(&this->lwipNetifStation.gw));
  STD_PRINTF("          - IP6 address (0) : %s [%" PRIu32 "]\n",
             ipaddr_ntoa(&this->lwipNetifStation.ip6_addr[0]),
             static_cast<std::uint32_t>(this->lwipNetifStation.ip6_addr_state[0]));
  STD_PRINTF("          - IP6 address (1) : %s [%" PRIu32 "]\n",
             ipaddr_ntoa(&this->lwipNetifStation.ip6_addr[1]),
             static_cast<std::uint32_t>(this->lwipNetifStation.ip6_addr_state[1]));
  STD_PRINTF("          - IP6 address (2) : %s [%" PRIu32 "]\n",
             ipaddr_ntoa(&this->lwipNetifStation.ip6_addr[2]),
             static_cast<std::uint32_t>(this->lwipNetifStation.ip6_addr_state[2]));
  {
    const ip_addr_t ip_addr_dns =
      /* 2001:4860:4860::8888 google */
      IPADDR6_INIT(lwip_htonl(0x20014860UL), lwip_htonl(0x48600000UL), lwip_htonl(0x00000000UL), lwip_htonl(0x00008888UL));
    /* IPADDR4_INIT(PP_HTONL(0x01010101)); */

    dns_setserver(DNS_MAX_SERVERS - 1, &ip_addr_dns);
    for (std::uint8_t num_dns = 0U; num_dns < DNS_MAX_SERVERS; num_dns++) {
      const ip_addr_t *const ip_addr_ptr = dns_getserver(num_dns);
      STD_PRINTF("          - DNS_%" PRIu32 " address   : %s\n", static_cast<std::uint32_t>(num_dns),
                 ipaddr_ntoa(ip_addr_ptr));
    }
  }
  DEBUG_STD_PRINTF("AppWiFiLwip::connectToAp()<\n\n")
}

void AppWiFiLwip::constructSoftApNetworkInterface(void) noexcept
{
  static const ip_addr_t IP_ADDRESS_FIXED = IPADDR4_INIT(lwip_htonl(MakeU32(10U, 10U, 10U, 1U)));
  static const ip_addr_t NETWORK_MASK = IPADDR4_INIT(lwip_htonl(MakeU32(255U, 255U, 255U, 0U)));

  DEBUG_STD_PRINTF("AppWiFiLwip::constructSoftApNetworkInterface()>\n")

  this->emwNetworkSoftAp.staticIpAddress = IP_ADDRESS_FIXED;
  this->emwNetworkSoftAp.staticGatewayAddress = IP_ADDRESS_FIXED;
  this->emwNetworkSoftAp.staticMaskAddress = NETWORK_MASK;
  this->emwNetworkSoftAp.dhcpInformFlag = false;
  this->emwNetworkSoftAp.dhcpReleaseOnLinkLost = false;
  this->emwNetworkSoftAp.driverAccessChannel = 6U;
  if (nullptr == netif_add_noaddr(&this->lwipNetifSoftAp, &this->emwNetworkSoftAp,
                                  &WiFiNetworkSoftAp::InitializeNetif, &tcpip_input)) {
    ErrorHandler();
    return;
  }
  netif_set_remove_callback(&this->lwipNetifSoftAp, &WiFiNetworkSoftAp::UnInitializeNetif);

  STD_PRINTF("[%6" PRIu32 "] Wi-Fi driver ready (SOFTAP):\n", HAL_GetTick());

  DEBUG_STD_PRINTF("AppWiFiLwip::constructSoftApNetworkInterface()<\n\n")
}

void AppWiFiLwip::constructStationNetworkInterface(void) noexcept
{
  static const ip_addr_t IP_ADDRESS_FIXED = IPADDR4_INIT(lwip_htonl(MakeU32(192, 168, 1, 113)));
  static const ip_addr_t NETWORK_MASK = IPADDR4_INIT(lwip_htonl(MakeU32(255U, 255U, 254U, 0U)));

  DEBUG_STD_PRINTF("AppWiFiLwip::constructStationNetworkInterface()>\n")

  this->emwNetworkStation.staticIpAddress = IP_ADDRESS_FIXED;
  this->emwNetworkStation.staticGatewayAddress = IP_ADDRESS_FIXED;
  this->emwNetworkStation.staticMaskAddress = NETWORK_MASK;
  this->emwNetworkStation.dhcpInformFlag = true;
  this->emwNetworkStation.dhcpReleaseOnLinkLost = false;
  this->emwNetworkStation.driverAccessChannel = 8U;
  if (nullptr == netif_add_noaddr(&this->lwipNetifStation, &this->emwNetworkStation,
                                  &WiFiNetworkStation::InitializeNetif, &tcpip_input)) {
    ErrorHandler();
    return;
  }
  netif_set_remove_callback(&this->lwipNetifStation, &WiFiNetworkStation::UnInitializeNetif);
  netif_set_default(&this->lwipNetifStation);

  STD_PRINTF("[%6" PRIu32 "] Wi-Fi driver ready (STATION):\n", HAL_GetTick());

  DEBUG_STD_PRINTF("AppWiFiLwip::constructStationNetworkInterface()<\n\n")
}


void AppWiFiLwip::enableSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  STD_PRINTF("\nStart Software enabled Access Point with \"%s\"\n", ssidString);

  if (0 != this->emwNetworkSoftAp.activate(ssidString, passwordString)) {
    STD_PRINTF("[%6" PRIu32 "] Failed to set up %s\n", HAL_GetTick(), ssidString);
    ErrorHandler();
    return;
  }
  STD_PRINTF("[%6" PRIu32 "] Wi-Fi interface ready (SOFTAP):\n", HAL_GetTick());
  STD_PRINTF("          - name        : \"%c%c\".\n", this->lwipNetifSoftAp.name[0],
             this->lwipNetifSoftAp.name[1]);
  STD_PRINTF("          - hostname    : \"%s\".\n", this->lwipNetifSoftAp.hostname);
  STD_PRINTF("          - mtu         : %" PRIu32 ".\n", static_cast<uint32_t>(this->lwipNetifSoftAp.mtu));
  STD_PRINTF("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
             this->lwipNetifSoftAp.hwaddr[0], this->lwipNetifSoftAp.hwaddr[1],
             this->lwipNetifSoftAp.hwaddr[2], this->lwipNetifSoftAp.hwaddr[3],
             this->lwipNetifSoftAp.hwaddr[4], this->lwipNetifSoftAp.hwaddr[5]);

  netif_set_link_up(&this->lwipNetifSoftAp);
  netif_set_up(&this->lwipNetifSoftAp);

  netif_set_addr(&this->lwipNetifSoftAp,
                 ip_2_ip4(&this->emwNetworkSoftAp.staticIpAddress),
                 ip_2_ip4(&this->emwNetworkSoftAp.staticMaskAddress),
                 ip_2_ip4(&this->emwNetworkSoftAp.staticGatewayAddress));

  STD_PRINTF("\n[%6" PRIu32 "] Network interface ready (SOFTAP):\n", HAL_GetTick());
  STD_PRINTF("          - IP address      : %s\n", ipaddr_ntoa(&this->lwipNetifSoftAp.ip_addr));
  STD_PRINTF("          - Netmask         : %s\n", ipaddr_ntoa(&this->lwipNetifSoftAp.netmask));
  STD_PRINTF("          - GW address      : %s\n", ipaddr_ntoa(&this->lwipNetifSoftAp.gw));

  STD_PRINTF("Starting the DHCP server ...\n");
  {
    static AppDhcpService dhcp_server(&this->lwipNetifSoftAp);
    if (0 != dhcp_server.createService()) {
      STD_PRINTF("Cannot start the DHCP server\n");
      ErrorHandler();
      return;
    }
  }
  DEBUG_STD_PRINTF("AppWiFiLwip::enableSoftAp()<\n\n")
}

void AppWiFiLwip::tearDownSoftApNetworkInterface(void) noexcept
{
  netif_remove(&this->lwipNetifSoftAp);
  netif_set_remove_callback(&this->lwipNetifSoftAp, nullptr);
#if LWIP_STATS_DISPLAY
  stats_display();
#endif /* LWIP_STATS_DISPLAY */
}

void AppWiFiLwip::tearDownStationNetworkInterface(void) noexcept
{
  netif_remove(&this->lwipNetifStation);
  netif_set_remove_callback(&this->lwipNetifStation, nullptr);
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
