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
#include "AppWiFiEmw.hpp"
#include "AppConsoleDownload.hpp"
#include "AppConsoleEcho.hpp"
#include "AppConsolePing.hpp"
#include "AppConsoleScan.hpp"
#include "AppConsoleStats.hpp"
#include "AppConsoleTls.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "emw_conf.hpp"
#include "main.hpp"
#include "stm32u5xx_hal.h"
#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS */
#include "wifi_emw.hpp"
#include <cinttypes>
#include <cstdio>
#include <cstring>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static bool CheckTimeout(std::uint32_t tickStart, std::uint32_t tickCount);


AppWiFiEmw::AppWiFiEmw(void) noexcept
{
  DEBUG_STD_PRINTF(" AppWiFiEmw::AppWiFiEmw()>\n")
  DEBUG_STD_PRINTF(" AppWiFiEmw::AppWiFiEmw()<\n\n")
}

AppWiFiEmw::~AppWiFiEmw(void)
{
  DEBUG_STD_PRINTF(" AppWiFiEmw::~AppWiFiEmw()>\n")
  DEBUG_STD_PRINTF(" AppWiFiEmw::~AppWiFiEmw()<\n\n")
}

extern "C" {
  void AppTaskFunction(void *argumentPtr)
  {
    class EmwApiEmw &emw = *(reinterpret_cast<class EmwApiEmw*>(argumentPtr));
    AppWiFiEmw the_application;

    DEBUG_STD_PRINTF("\n AppTaskFunction()>\n")

    emw.registerStatusCallback(&AppWiFiEmw::EmwStatusChanged, nullptr, EmwApiBase::eSOFTAP);
    emw.registerStatusCallback(&AppWiFiEmw::EmwStatusChanged, nullptr, EmwApiBase::eSTATION);

    {
      EmwApiCore::MacAddress_t mac_address_48bits;

      emw.getStationMacAddress(mac_address_48bits);
      STD_PRINTF("\n %s Station MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
                 EmwApiCore::productIdentifier,
                 mac_address_48bits.bytes[0], mac_address_48bits.bytes[1],
                 mac_address_48bits.bytes[2], mac_address_48bits.bytes[3],
                 mac_address_48bits.bytes[4], mac_address_48bits.bytes[5]);
    }
    {
      std::uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};

      emw.getIPAddress(ip_address_bytes, EmwApiBase::eSTATION);
      STD_PRINTF(" IP (STA)   : %02X.%02X.%02X.%02X\n",
                 ip_address_bytes[0], ip_address_bytes[1],
                 ip_address_bytes[2], ip_address_bytes[3]);
    }
    {
      std::uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};

      emw.getIPAddress(ip_address_bytes, EmwApiBase::eSOFTAP);
      STD_PRINTF(" IP (SOFTAP): %02X.%02X.%02X.%02X\n",
                 ip_address_bytes[0], ip_address_bytes[1],
                 ip_address_bytes[2], ip_address_bytes[3]);
    }
    STD_PRINTF("\n connected: %s\n", emw.isConnected() == 1U ? "true" : "false");

    {
      static const char ssid[33] = {"MyHotSpot"};
      static const char psk[65] = {""};

      the_application.enableSoftAp(emw, ssid, psk);
    }

    STD_PRINTF("\n Wi-Fi scan\n");
    if (0 != Scan(emw)) {
      STD_PRINTF("Wi-Fi scan failed\n");
    }

    {
      {
        static const char ssid[33] = {WIFI_SSID};
        static const char psk[65] = {WIFI_PASSWORD};

        STD_PRINTF("\n Wi-Fi connection\n");
        the_application.connectToAp(emw, ssid, psk);
      }

      {
        class AppConsoleEcho echo(emw);
        class AppConsoleDownload http(emw);
        class AppConsolePing ping(emw);
        class AppConsoleScan scan;
        class AppConsoleStats stats(emw);
        class AppConsoleTls tls(emw);
        class Cmd *cmds[] = {&echo, &http, &ping, &scan, &stats, &tls, nullptr};
        class Console the_console("app>", cmds);

        the_console.run();
      }
    }
    the_application.disconnectFromAp(emw);
    the_application.disableSoftAp(emw);
    emw.unInitialize();

    for (;;) {
      STD_PRINTF(".");
#if defined(COMPILATION_WITH_FREERTOS)
      vTaskDelay(5000U);
#else
      HAL_Delay(5000U);
#endif /* COMPILATION_WITH_FREERTOS) */
    }
    DEBUG_STD_PRINTF(" AppTaskFunction()<\n\n")
  }
}

void AppWiFiEmw::connectToAp(class EmwApiEmw &emw, const char (&ssidString)[33],
                             const char (&passwordString)[65]) noexcept
{
  EmwApiBase::Status status;
  const EmwApiBase::ConnectAttributes_t connect_attributes;

  STD_PRINTF("\n AppWiFiEmw::connectToAp()> joining \"%s\" with \"%s\" ...\n", ssidString, passwordString);

#if defined(WITH_NO_DHCP_OPTION)
  const EmwApiBase::IpAttributes_t ip_attributes = {

  };
  emw.stationSettings.dhcpIsEnabled = false;
  emw.stationSettings.ip_address[0] = 192;
  emw.stationSettings.ip_address[1] = 168;
  emw.stationSettings.ip_address[2] = 1;
  emw.stationSettings.ip_address[3] = 113;
  emw.stationSettings.ip_mask[0] = 0xFF;
  emw.stationSettings.ip_mask[1] = 0xFF;
  emw.stationSettings.ip_mask[2] = 0xFF;
  emw.stationSettings.ip_mask[3] = 0;
  emw.stationSettings.gatewayAddress[0] = 192;
  emw.stationSettings.gatewayAddress[1] = 168;
  emw.stationSettings.gatewayAddress[2] = 1;
  emw.stationSettings.gatewayAddress[3] = 254;
  emw.stationSettings.dns1[0] = 1;
  emw.stationSettings.dns1[1] = 1;
  emw.stationSettings.dns1[2] = 1;
  emw.stationSettings.dns1[3] = 1;
#else
  const EmwApiBase::IpAttributes_t ip_attributes;
#endif /* WITH_DHCP_OPTION */

  status = emw.connect(ssidString, passwordString, connect_attributes, ip_attributes);
  if (EmwApiBase::eEMW_STATUS_OK != status) {
    STD_PRINTF("[%6" PRIu32 "] Cannot Join ... \"%s\" (%" PRId32 ")\n", HAL_GetTick(), ssidString,
               static_cast<std::int32_t>(status));
    ErrorHandler();
    return;
  }
  {
    const std::uint32_t tick_start = HAL_GetTick();

    while (!AppWiFiEmw::EmwInterfaceStationUp) {
      STD_PRINTF("c");
#if defined(COMPILATION_WITH_FREERTOS)
      vTaskDelay(10U);
#else
      emw.checkNotified(10U /* timeout */);
#endif /* COMPILATION_WITH_FREERTOS) */
      if (CheckTimeout(tick_start, 10000U)) {
        STD_PRINTF(" Not connected in the 10 last seconds.\n");
        break;
      }
    }
  }
  STD_PRINTF("\n Connected: %s\n", emw.isConnected() == 1U ? "true" : "false");
  if (1 == emw.isConnected()) {
    {
      std::uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};

      status = emw.getIPAddress(ip_address_bytes, EmwApiBase::eSTATION);
      if (EmwApiBase::eEMW_STATUS_OK != status) {
        STD_PRINTF(" Failed to get IP address (%" PRId32 ")\n", static_cast<std::int32_t>(status));
      }
      STD_PRINTF("\n IP address      %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                 static_cast<std::uint32_t>(ip_address_bytes[0]), static_cast<std::uint32_t>(ip_address_bytes[1]),
                 static_cast<std::uint32_t>(ip_address_bytes[2]), static_cast<std::uint32_t>(ip_address_bytes[3]));
// (void) std::printf("Gateway address %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
// static_cast<std::uint32_t>(emw.stationSettings.gatewayAddress[0]),
// static_cast<std::uint32_t>(emw.stationSettings.gatewayAddress[1]),
// static_cast<std::uint32_t>(emw.stationSettings.gatewayAddress[2]),
// static_cast<std::uint32_t>(emw.stationSettings.gatewayAddress[3]));
// (void) std::printf("DNS1 address    %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
// static_cast<std::uint32_t>(emw.stationSettings.dns1[0]),
// static_cast<std::uint32_t>(emw.stationSettings.dns1[1]),
// static_cast<std::uint32_t>(emw.stationSettings.dns1[2]),
// static_cast<std::uint32_t>(emw.stationSettings.dns1[3]));
    }
    for (std::uint8_t interface = 0U; interface < EmwApiBase::eWIFI_INTERFACE_IDX_MAX; interface++) {
      for (std::uint8_t address_slot = 0U; address_slot < 3U; address_slot++) {
        std::uint8_t ip6_addr_bytes[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
        const std::int32_t address_state = emw.getIP6AddressState(address_slot,
                                           static_cast<EmwApiBase::EmwInterface>(interface));

        STD_PRINTF(" IPv6 address (%" PRIu32 ")(%" PRIu32 "): state %" PRIi32 "\n",
                   static_cast<std::uint32_t>(interface), static_cast<std::uint32_t>(address_slot), address_state);
        status = emw.getIP6Address(ip6_addr_bytes, address_slot, static_cast<EmwApiBase::EmwInterface>(interface));
        if (EmwApiBase::eEMW_STATUS_OK != status) {
          STD_PRINTF(" Failed to get IPv6 address (%" PRId32 ")\n", static_cast<std::int32_t>(status));
        }
        STD_PRINTF(" IPv6 address (%" PRIu32 ")(%" PRIu32 "): "
                   "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\n",
                   static_cast<std::uint32_t>(interface), static_cast<std::uint32_t>(address_slot),
                   ip6_addr_bytes[0], ip6_addr_bytes[1], ip6_addr_bytes[2], ip6_addr_bytes[3],
                   ip6_addr_bytes[4], ip6_addr_bytes[5], ip6_addr_bytes[6], ip6_addr_bytes[7],
                   ip6_addr_bytes[8], ip6_addr_bytes[9], ip6_addr_bytes[10], ip6_addr_bytes[11],
                   ip6_addr_bytes[12], ip6_addr_bytes[13], ip6_addr_bytes[14], ip6_addr_bytes[15]);
      }
    }
  }
  else {
    STD_PRINTF(" Not connected\n\n");
    ErrorHandler();
    return;
  }
  DEBUG_STD_PRINTF(" AppWiFiEmw::connectToAp()<\n\n")
}

void AppWiFiEmw::disableSoftAp(class EmwApiEmw &emw) noexcept
{
  DEBUG_STD_PRINTF("\nAppWiFiEmw::disableSoftAp()>\n")

  {
    const EmwApiBase::Status status = emw.stopSoftAp();
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      STD_PRINTF(" Failed to stop SoftAP (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  {
    const EmwApiBase::Status status = emw.unRegisterStatusCallback(EmwApiBase::eSOFTAP);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      STD_PRINTF("Failed to unRegisterStatusCallback (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  DEBUG_STD_PRINTF(" AppWiFiEmw::disableSoftAp()<\n\n")
}

void AppWiFiEmw::disconnectFromAp(class EmwApiEmw &emw) noexcept
{
  DEBUG_STD_PRINTF("\n AppWiFiEmw::disconnectFromAp()>\n")

  {
    const EmwApiBase::Status status = emw.disconnect();
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      STD_PRINTF(" Failed to disconnect (%" PRId32 ")\n", static_cast<std::int32_t>(status));
      ErrorHandler();
      return;
    }
    else {
      STD_PRINTF("\nHas disconnected Ap\n\n");
    }
  }
  {
    const EmwApiBase::Status status = emw.unRegisterStatusCallback(EmwApiBase::eSTATION);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      STD_PRINTF("Failed to unRegisterStatusCallback (%" PRId32 ")\n", static_cast<std::int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  DEBUG_STD_PRINTF(" AppWiFiEmw::disconnectFromAp()<\n\n")
}

void AppWiFiEmw::enableSoftAp(class EmwApiEmw &emw,
                              const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  EmwApiBase::Status status;
  EmwApiBase::SoftApSettings_t access_point_settings;

  DEBUG_STD_PRINTF("\n AppWiFiEmw::enableSoftAp()>\n")

  static_cast<void>(std::strncpy(access_point_settings.ssidString, ssidString,
                                 sizeof(access_point_settings.ssidString) - 1));
  static_cast<void>(std::strncpy(access_point_settings.passwordString, passwordString,
                                 sizeof(access_point_settings.passwordString) - 1));
  access_point_settings.channel = 8U;
  static_cast<void>(std::strncpy(access_point_settings.ip.ipAddressLocal, "10.10.10.1",
                                 sizeof(access_point_settings.ip.ipAddressLocal) - 1));
  static_cast<void>(std::strncpy(access_point_settings.ip.gatewayAddress, "10.10.10.1",
                                 sizeof(access_point_settings.ip.gatewayAddress) - 1));
  static_cast<void>(std::strncpy(access_point_settings.ip.networkMask, "255.255.255.0",
                                 sizeof(access_point_settings.ip.networkMask) - 1));
  AppWiFiEmw::EmwInterfaceSoftApUp = false;

  STD_PRINTF("\n Start Software enabled Access Point with \"%s\"\n", access_point_settings.ssidString);
  status = emw.startSoftAp(access_point_settings);
  if (EmwApiBase::eEMW_STATUS_OK != status) {
    STD_PRINTF(" Failed to setup %s (%" PRId32 ")\n", access_point_settings.ssidString,
               static_cast<std::int32_t>(status));
    ErrorHandler();
    return;
  }
  {
    const std::uint32_t tick_start = HAL_GetTick();

    do {
      STD_PRINTF("u");
#if defined(COMPILATION_WITH_FREERTOS)
      /* Be cooperative. */
      vTaskDelay(10U);
#else
      emw.checkNotified(10U /* timeout */);
#endif /* COMPILATION_WITH_FREERTOS) */
      if (CheckTimeout(tick_start, 10000U)) {
        STD_PRINTF("\n Not connected in the 10 last seconds.\n");
        break;
      }
    }
    while (!AppWiFiEmw::EmwInterfaceSoftApUp);
  }
  {
    EmwApiCore::MacAddress_t mac_address_48bits;

    status = emw.getSoftApMacAddress(mac_address_48bits);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      STD_PRINTF("\n Failed to get SoftAP MAC address (%" PRId32 ")\n", static_cast<std::int32_t>(status));
    }
    else {
      STD_PRINTF("\n SoftAP MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
                 mac_address_48bits.bytes[0], mac_address_48bits.bytes[1],
                 mac_address_48bits.bytes[2], mac_address_48bits.bytes[3],
                 mac_address_48bits.bytes[4], mac_address_48bits.bytes[5]);
    }
  }
  DEBUG_STD_PRINTF(" AppWiFiEmw::enableSoftAp()>\n\n")
}

void AppWiFiEmw::EmwStatusChanged(EmwApiBase::EmwInterface interface,
                                  enum EmwApiBase::WiFiEvent status, void *argumentPtr) noexcept
{
  static_cast<void>(argumentPtr);

  if (EmwApiBase::eSTATION == interface) {
    switch (status) {
      case EmwApiBase::eWIFI_EVENT_STA_DOWN: {
          AppWiFiEmw::EmwInterfaceStationUp = false;
          STD_PRINTF("\n -> EmwApiBase::eWIFI_EVENT_STA_DOWN\n\n");
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_UP: {
          AppWiFiEmw::EmwInterfaceStationUp = true;
          STD_PRINTF("\n -> EmwApiBase::eWIFI_EVENT_STA_UP\n\n");
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_GOT_IP: {
          STD_PRINTF("\n -> EmwApiBase::eWIFI_EVENT_STA_GOT_IP\n");
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
          AppWiFiEmw::EmwInterfaceSoftApUp = false;
          STD_PRINTF("\n -> EmwApiBase::eWIFI_EVENT_AP_DOWN\n");
          break;
        }
      case EmwApiBase::eWIFI_EVENT_AP_UP: {
          AppWiFiEmw::EmwInterfaceSoftApUp = true;
          STD_PRINTF("\n -> EmwApiBase::eWIFI_EVENT_AP_UP\n");
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

volatile bool AppWiFiEmw::EmwInterfaceSoftApUp = false;
volatile bool AppWiFiEmw::EmwInterfaceStationUp = false;

static bool CheckTimeout(std::uint32_t tickStart, std::uint32_t tickCount)
{
  const std::uint32_t elapsed_ticks = HAL_GetTick() - tickStart;
  bool status = false;

  if (elapsed_ticks > tickCount) {
    status = true;
  }
  return status;
}
