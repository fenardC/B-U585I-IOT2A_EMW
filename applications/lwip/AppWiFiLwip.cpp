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
#include "main.hpp"
#include "lwip/api.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "stm32u5xx_hal.h"
#include "WiFiNetwork.hpp"
#include <inttypes.h>
#include <cstring>
#include <cstdio>
#include <stdbool.h>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

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
#if LWIP_NETIF_HOSTNAME
  netif_set_hostname(&this->netifSOFTAP, "lwip-softap");
  netif_set_hostname(&this->netifSTA, "lwip-sta");
#endif /* LWIP_NETIF_HOSTNAME */
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

    tcpip_init(NULL, NULL);

#if defined(OPTION_WITH_IPV6)
    std::printf("\n\nOPTION_WITH_IPV6\n\n");
#endif /* OPTION_WITH_IPV6 */
    std::printf("NETWORK_BUFFER_SIZE: %6" PRIu32 "\n", (uint32_t)EmwNetworkStack::NETWORK_BUFFER_SIZE);
    std::printf("MEM_SIZE           : %6" PRIu32 "\n", (uint32_t)MEM_SIZE);
    std::printf("PBUF_POOL_BUFSIZE  : %6" PRIu32 "\n\n", (uint32_t)PBUF_POOL_BUFSIZE);
    std::printf("\nWi-Fi network interface initialization (SOFTAP)\n");
    the_application.constructSoftApNetworkInterface();
    std::printf("\nWi-Fi network interface initialization (STATION)\n");
    the_application.constructStationNetworkInterface();
    WiFiNetwork::checkIoSpeed();
    the_application.enableSoftAccessPoint();
    {
      std::printf("\nWi-Fi scan\n");
      class AppConsoleScan scan(nullptr);

      if (EmwApiBase::eEMW_STATUS_OK != scan.execute(0, nullptr)) {
        std::printf("%s: Wi-Fi scan failed\n", scan.getName());
      }
    }
    std::printf("\nWi-Fi connection\n");
    the_application.joinAccessPoint(WIFI_SSID, WIFI_PASSWORD);
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
    std::printf("\nWi-Fi network interface un-initialization (STATION)\n");
    the_application.tearDownStationNetworkInterface();
    std::printf("\nWi-Fi network interface un-initialization (SOFTAP)\n");
    the_application.tearDownSoftApNetworkInterface();
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

    std::printf("[%6" PRIu32 "] Wi-Fi driver ready:\n", HAL_GetTick());
    std::printf("          - Device Name    : %s.\n", wifi_network_ptr->deviceNameString);
    std::printf("          - Device ID      : %s.\n", wifi_network_ptr->deviceIdentifierString);
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
    std::printf("[%6" PRIu32 "] Wi-Fi driver ready:\n", HAL_GetTick());
    std::printf("          - Device Name    : %s.\n", wifi_network_ptr->deviceNameString);
    std::printf("          - Device ID      : %s.\n", wifi_network_ptr->deviceIdentifierString);
    std::printf("          - Device Version : %s.\n", wifi_network_ptr->deviceVersionString);
  }
  STD_PRINTF("AppWiFiLwip::constructStationNetworkInterface()<\n\n")
}

void AppWiFiLwip::enableSoftAccessPoint(void)
{
  STD_PRINTF("\nAppWiFiLwip::enableSoftAccessPoint()>\n")

  this->networkSOFTAP.credentials.ssidStringPtr = "MyHotSpot";
  this->networkSOFTAP.credentials.pskStringPtr = "";
  std::printf("\nStart Software enabled Access Point with \"%s\"\n", this->networkSOFTAP.credentials.ssidStringPtr);
  if (0 != this->networkSOFTAP.startSoftAccesPoint()) {
    std::printf("[%6" PRIu32 "] Failed to set up %s\n", HAL_GetTick(), this->networkSOFTAP.credentials.ssidStringPtr);
    ErrorHandler();
    return;
  }
  std::printf("SoftAP MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
              this->netifSOFTAP.hwaddr[0], this->netifSOFTAP.hwaddr[1],
              this->netifSOFTAP.hwaddr[2], this->netifSOFTAP.hwaddr[3],
              this->netifSOFTAP.hwaddr[4], this->netifSOFTAP.hwaddr[5]);
  STD_PRINTF("AppWiFiLwip::enableSoftAccessPoint()<\n\n")
}

void AppWiFiLwip::joinAccessPoint(const char *ssidStringPtr, const char *passwordStringPtr)
{
  std::printf("\nAppWiFiLwip::joinAccessPoint()> joining \"%s\" with \"%s\" ...\n", ssidStringPtr, passwordStringPtr);
  this->networkSTA.credentials.ssidStringPtr = ssidStringPtr;
  this->networkSTA.credentials.pskStringPtr = passwordStringPtr;
  if (0 != this->networkSTA.startStation()) {
    std::printf("[%6" PRIu32 "] Cannot Join ... \"%s\"\n", HAL_GetTick(), this->networkSTA.credentials.ssidStringPtr);
    ErrorHandler();
    return;
  }
  std::printf("[%6" PRIu32 "] Wi-Fi interface ready:\n", HAL_GetTick());
  std::printf("          - name        : \"%c%c\".\n", this->netifSTA.name[0], this->netifSTA.name[1]);
  std::printf("          - hostname    : \"%s\".\n", this->netifSTA.hostname);
  std::printf("          - mtu         : %" PRIu32 ".\n", (uint32_t)this->netifSTA.mtu);
  std::printf("          - MAC         : %02X.%02X.%02X.%02X.%02X.%02X\n",
              this->netifSTA.hwaddr[0], this->netifSTA.hwaddr[1], this->netifSTA.hwaddr[2],
              this->netifSTA.hwaddr[3], this->netifSTA.hwaddr[4], this->netifSTA.hwaddr[5]);
  netif_set_link_up(&this->netifSTA);
  netif_set_up(&this->netifSTA);

  std::printf("[%6" PRIu32 "] Calling dhcp_start()\n", HAL_GetTick());
  (void) dhcp_start(&this->netifSTA);
  while (!this->networkSTA.ipAcquired) {
    /* Be cooperative */
    vTaskDelay(10U);
  }
  std::printf("\n[%6" PRIu32 "] Network Interface connected:\n", HAL_GetTick());
  std::printf("          - IP address      : %s\n", ipaddr_ntoa(&this->netifSTA.ip_addr));
  std::printf("          - GW address      : %s\n\n", ipaddr_ntoa(&this->netifSTA.gw));
  {
    const ip_addr_t ip_addr_dns = IPADDR4_INIT(PP_HTONL(0x01010101));

    dns_setserver(DNS_MAX_SERVERS - 1, &ip_addr_dns);
    for (uint8_t num_dns = 0U; num_dns < DNS_MAX_SERVERS; num_dns++) {
      const ip_addr_t *const ip_addr_ptr = dns_getserver(num_dns);
      std::printf("          - DNS_%" PRIu32 " address   : %s\n", (uint32_t)num_dns, ipaddr_ntoa(ip_addr_ptr));
    }
  }
}

void AppWiFiLwip::tearDownSoftApNetworkInterface(void)
{
  netif_remove(&this->netifSOFTAP);
  netif_set_remove_callback(&this->netifSOFTAP, nullptr);
#if LWIP_STATS_DISPLAY
  stats_display();
#endif
}

void AppWiFiLwip::tearDownStationNetworkInterface(void)
{
  netif_remove(&this->netifSTA);
  netif_set_remove_callback(&this->netifSTA, nullptr);
#if LWIP_STATS_DISPLAY
  stats_display();
#endif
}

#if defined(LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS) && (LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS == 0)
u32_t sys_now(void);
u32_t sys_now(void)
{
  return HAL_GetTick();
}
#endif /* LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS == 0 */
