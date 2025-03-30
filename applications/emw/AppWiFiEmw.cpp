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
#include "AppWiFiEmw.hpp"
#include "AppConsoleEcho.hpp"
#include "AppConsolePing.hpp"
#include "AppConsoleScan.hpp"
#include "AppConsoleStats.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "emw_conf.hpp"
#include "main.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <memory>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

extern "C" void AppTaskFunction(void *argumentPtr);
static bool CheckTimeout(std::uint32_t tickStart, std::uint32_t tickCount);


AppWiFiEmw::AppWiFiEmw(void) noexcept
  : emw()
{
  STD_PRINTF("AppWiFiEmw::AppWiFiEmw()>\n")
  STD_PRINTF("AppWiFiEmw::AppWiFiEmw()<\n\n")
}

AppWiFiEmw::~AppWiFiEmw(void)
{
  STD_PRINTF("AppWiFiEmw::~AppWiFiEmw()>\n")
  STD_PRINTF("AppWiFiEmw::~AppWiFiEmw()<\n\n")
}

extern "C" {
  void AppTaskFunction(void *argumentPtr)
  {
    AppWiFiEmw the_application;
    static_cast<void>(argumentPtr);

    STD_PRINTF("\nAppTaskFunction()>\n")

    (void) std::printf("%s\n", the_application.emw.getConfigurationString());
    (void) std::printf(" NETWORK_BUFFER_SIZE: %6" PRIu32 "\n\n",
                       static_cast<std::uint32_t>(EmwNetworkStack::NETWORK_BUFFER_SIZE));

    the_application.emw.resetHardware();
    the_application.emw.initialize();
    the_application.emw.registerStatusCallback(AppWiFiEmw::EmwInterfaceStatusChanged, nullptr, EmwApiBase::eSOFTAP);
    the_application.emw.registerStatusCallback(AppWiFiEmw::EmwInterfaceStatusChanged, nullptr, EmwApiBase::eSTATION);

    (void) std::printf(" - Device Name    : %s.\n", the_application.emw.systemInformations.productName);
    (void) std::printf(" - Device ID      : %s.\n", the_application.emw.systemInformations.productIdentifier);
    (void) std::printf(" - Device Version : %s.\n", the_application.emw.systemInformations.firmwareRevision);
    (void) std::printf(" - MAC address    : %02X.%02X.%02X.%02X.%02X.%02X\n\n",
                       the_application.emw.systemInformations.mac48bitsStation[0],
                       the_application.emw.systemInformations.mac48bitsStation[1],
                       the_application.emw.systemInformations.mac48bitsStation[2],
                       the_application.emw.systemInformations.mac48bitsStation[3],
                       the_application.emw.systemInformations.mac48bitsStation[4],
                       the_application.emw.systemInformations.mac48bitsStation[5]);
    {
      char version[25] = {""};

      std::uint32_t version_size = sizeof(version);
      the_application.emw.getVersion(version, version_size);
      (void) std::printf("\n Version: %s\n", version);
    }
    {
      EmwApiCore::MacAddress_t mac_address_48bits;

      the_application.emw.getStationMacAddress(mac_address_48bits);
      (void) std::printf("\n %s Station MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
                         the_application.emw.systemInformations.productIdentifier,
                         mac_address_48bits.bytes[0], mac_address_48bits.bytes[1],
                         mac_address_48bits.bytes[2], mac_address_48bits.bytes[3],
                         mac_address_48bits.bytes[4], mac_address_48bits.bytes[5]);
    }
    {
      std::uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};

      the_application.emw.getIPAddress(ip_address_bytes, EmwApiBase::eSTATION);
      (void) std::printf(" IP (STA)   : %02X.%02X.%02X.%02X\n",
                         ip_address_bytes[0], ip_address_bytes[1],
                         ip_address_bytes[2], ip_address_bytes[3]);
    }
    {
      std::uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};

      the_application.emw.getIPAddress(ip_address_bytes, EmwApiBase::eSOFTAP);
      (void) std::printf(" IP (SOFTAP): %02X.%02X.%02X.%02X\n",
                         ip_address_bytes[0], ip_address_bytes[1],
                         ip_address_bytes[2], ip_address_bytes[3]);
    }
    (void) std::printf("\n connected: %s\n", the_application.emw.isConnected() == 1U ? "true" : "false");
    the_application.checkIoSpeed();

    {
      static const char ssid[33] = {"MyHotSpot"};
      static const char psk[65] = {""};

      the_application.enableSoftAp(ssid, psk);
    }

    {
      (void) std::printf("\n Wi-Fi scan\n");
      class AppConsoleScan scan(the_application.emw);

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
        class AppConsoleEcho echo(the_application.emw);
        class AppConsolePing ping(the_application.emw);
        class AppConsoleScan scan(the_application.emw);
        class AppConsoleStats stats(the_application.emw);
        class Cmd *cmds[] = {&echo, &ping, &scan, &stats, nullptr};
        class Console the_console("app>", cmds);

        the_console.run();
      }
    }
    the_application.disconnectFromAp();
    the_application.disableSoftAp();
    the_application.emw.unInitialize();

    for (;;) {
      (void) std::printf(".");
#if defined(COMPILATION_WITH_FREERTOS)
      vTaskDelay(5000U);
#else
      HAL_Delay(5000U);
#endif /* COMPILATION_WITH_FREERTOS) */
    }
    STD_PRINTF("AppTaskFunction()<\n\n")
  }
}

void AppWiFiEmw::checkIoSpeed(void) noexcept
{
  constexpr std::uint16_t ipc_test_data_size = 2500U /*EmwNetworkStack::NETWORK_BUFFER_SIZE*/;
  std::unique_ptr<std::uint8_t[]> ipc_test_data_ptr(new std::uint8_t[ipc_test_data_size]);

  if (nullptr != ipc_test_data_ptr) {
    std::uint32_t send_count = 0U;
    std::uint32_t receive_count = 0U;
    std::uint32_t time_ms_count;

    std::memset(&ipc_test_data_ptr[0], 0x00, ipc_test_data_size);

    (void) std::printf("\nAppWiFiEmw::checkIoSpeed()> (80 x (%" PRIu32 " + %" PRIu32 ")) ...\n",
                       static_cast<std::uint32_t>(ipc_test_data_size), static_cast<std::uint32_t>(ipc_test_data_size));

    time_ms_count = HAL_GetTick();
    for (std::uint32_t i = 0U; i < 80U; i++) {
      std::uint16_t echoed_length = ipc_test_data_size;
      const EmwApiBase::Status status \
        = this->emw.testIpcEcho(reinterpret_cast<std::uint8_t (&)[]>(* &ipc_test_data_ptr[0]), ipc_test_data_size,
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

void AppWiFiEmw::connectToAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  EmwApiBase::Status status;

  (void) std::printf("\nAppWiFiEmw::connectToAp()> joining \"%s\" with \"%s\" ...\n", ssidString, passwordString);

#if defined(WITH_NO_DHCP_OPTION)
  this->emw.stationSettings.dhcpIsEnabled = false;
  this->emw.stationSettings.ip_address[0] = 192;
  this->emw.stationSettings.ip_address[1] = 168;
  this->emw.stationSettings.ip_address[2] = 1;
  this->emw.stationSettings.ip_address[3] = 113;
  this->emw.stationSettings.ip_mask[0] = 0xFF;
  this->emw.stationSettings.ip_mask[1] = 0xFF;
  this->emw.stationSettings.ip_mask[2] = 0xFF;
  this->emw.stationSettings.ip_mask[3] = 0;
  this->emw.stationSettings.gatewayAddress[0] = 192;
  this->emw.stationSettings.gatewayAddress[1] = 168;
  this->emw.stationSettings.gatewayAddress[2] = 1;
  this->emw.stationSettings.gatewayAddress[3] = 254;
  this->emw.stationSettings.dns1[0] = 1;
  this->emw.stationSettings.dns1[1] = 1;
  this->emw.stationSettings.dns1[2] = 1;
  this->emw.stationSettings.dns1[3] = 1;
#else
  this->emw.stationSettings.dhcpIsEnabled = true;
#endif /* WITH_DHCP_OPTION */

  status = this->emw.connect(ssidString, passwordString, EmwApiBase::eSEC_AUTO);
  if (EmwApiBase::eEMW_STATUS_OK != status) {
    (void) std::printf("[%6" PRIu32 "] Cannot Join ... \"%s\" (%" PRId32 ")\n", HAL_GetTick(), ssidString,
                       static_cast<std::int32_t>(status));
    ErrorHandler();
    return;
  }
  {
    const std::uint32_t tick_start = HAL_GetTick();

    while (!AppWiFiEmw::EmwInterfaceStationUp) {
      (void) std::printf("c");
#if defined(COMPILATION_WITH_FREERTOS)
      vTaskDelay(10U);
#else
      this->emw.checkNotified(10U /* timeout */);
#endif /* COMPILATION_WITH_FREERTOS) */
      if (CheckTimeout(tick_start, 10000U)) {
        (void) std::printf("Not connected in the 10 last seconds.\n");
        break;
      }
    }
  }
  (void) std::printf("\n Connected: %s\n", this->emw.isConnected() == 1U ? "true" : "false");
  if (1 == this->emw.isConnected()) {
    {
      std::uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};

      status = this->emw.getIPAddress(ip_address_bytes, EmwApiBase::eSTATION);
      if (EmwApiBase::eEMW_STATUS_OK != status) {
        (void) std::printf("Failed to get IP address (%" PRId32 ")\n", static_cast<std::int32_t>(status));
      }
      (void) std::printf("\nIP address      %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                         static_cast<std::uint32_t>(ip_address_bytes[0]), static_cast<std::uint32_t>(ip_address_bytes[1]),
                         static_cast<std::uint32_t>(ip_address_bytes[2]), static_cast<std::uint32_t>(ip_address_bytes[3]));
      (void) std::printf("Gateway address %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                         static_cast<std::uint32_t>(this->emw.stationSettings.gatewayAddress[0]),
                         static_cast<std::uint32_t>(this->emw.stationSettings.gatewayAddress[1]),
                         static_cast<std::uint32_t>(this->emw.stationSettings.gatewayAddress[2]),
                         static_cast<std::uint32_t>(this->emw.stationSettings.gatewayAddress[3]));
      (void) std::printf("DNS1 address    %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                         static_cast<std::uint32_t>(this->emw.stationSettings.dns1[0]),
                         static_cast<std::uint32_t>(this->emw.stationSettings.dns1[1]),
                         static_cast<std::uint32_t>(this->emw.stationSettings.dns1[2]),
                         static_cast<std::uint32_t>(this->emw.stationSettings.dns1[3]));
    }
    for (std::uint8_t interface = 0U; interface < EmwApiBase::eWIFI_INTERFACE_IDX_MAX; interface++) {
      for (std::uint8_t address_slot = 0U; address_slot < 3U; address_slot++) {
        std::uint8_t ip6_addr_bytes[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
        const std::int32_t address_state = this->emw.getIP6AddressState(address_slot,
                                           static_cast<EmwApiBase::EmwInterface>(interface));

        (void) std::printf("IPv6 address (%" PRIu32 ")(%" PRIu32 "): state %" PRIi32 "\n",
                           static_cast<std::uint32_t>(interface), static_cast<std::uint32_t>(address_slot), address_state);
        status = this->emw.getIP6Address(ip6_addr_bytes, address_slot, static_cast<EmwApiBase::EmwInterface>(interface));
        if (EmwApiBase::eEMW_STATUS_OK != status) {
          (void) std::printf("failed to get IPv6 address (%" PRId32 ")\n", static_cast<std::int32_t>(status));
        }
        (void) std::printf("IPv6 address (%" PRIu32 ")(%" PRIu32 "): "
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
    (void) std::printf("Not connected\n\n");
    ErrorHandler();
    return;
  }
  STD_PRINTF("AppWiFiEmw::connectToAp()<\n\n")
}

void AppWiFiEmw::disableSoftAp(void) noexcept
{
  STD_PRINTF("\nAppWiFiEmw::disableSoftAp()>\n")

  {
    const EmwApiBase::Status status = this->emw.stopSoftAp();
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      (void) std::printf("Failed to stop SoftAP (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  {
    const EmwApiBase::Status status = this->emw.unRegisterStatusCallback(EmwApiBase::eSOFTAP);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      (void) std::printf("Failed to unRegisterStatusCallback (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  STD_PRINTF("AppWiFiEmw::disableSoftAp()<\n\n")
}

void AppWiFiEmw::disconnectFromAp(void) noexcept
{
  STD_PRINTF("\nAppWiFiEmw::disconnectFromAp()>\n")

  {
    const EmwApiBase::Status status = this->emw.disconnect();
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      (void) std::printf("Failed to disconnect (%" PRId32 ")\n", static_cast<std::int32_t>(status));
      ErrorHandler();
      return;
    }
    else {
      std::printf("\nHas disconnect Ap\n\n");
    }
  }
  {
    const EmwApiBase::Status status = this->emw.unRegisterStatusCallback(EmwApiBase::eSTATION);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("Failed to unRegisterStatusCallback (%" PRId32 ")\n", static_cast<std::int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  STD_PRINTF("AppWiFiEmw::disconnectFromAp()<\n\n")
}
void AppWiFiEmw::enableSoftAp(const char (&ssidString)[33], const char (&passwordString)[65]) noexcept
{
  EmwApiBase::Status status;
  EmwApiBase::SoftApSettings_t access_point_settings;

  STD_PRINTF("\nAppWiFiEmw::enableSoftAp()>\n")

  (void) std::strncpy(access_point_settings.ssidString, ssidString,
                      sizeof(access_point_settings.ssidString) - 1);
  (void) std::strncpy(access_point_settings.passwordString, passwordString,
                      sizeof(access_point_settings.passwordString) - 1);
  access_point_settings.channel = 8U;
  (void) std::strncpy(access_point_settings.ip.ipAddressLocal, "10.10.10.1",
                      sizeof(access_point_settings.ip.ipAddressLocal) - 1);
  (void) std::strncpy(access_point_settings.ip.gatewayAddress, "10.10.10.1",
                      sizeof(access_point_settings.ip.gatewayAddress) - 1);
  (void) std::strncpy(access_point_settings.ip.networkMask, "255.255.255.0",
                      sizeof(access_point_settings.ip.networkMask) - 1);
  AppWiFiEmw::EmwInterfaceSoftApUp = false;
  (void) std::printf("\nStart Software enabled Access Point with \"%s\"\n", access_point_settings.ssidString);
  status = this->emw.startSoftAp(access_point_settings);
  if (EmwApiBase::eEMW_STATUS_OK != status) {
    (void) std::printf("Failed to setup %s (%" PRId32 ")\n", access_point_settings.ssidString,
                       static_cast<std::int32_t>(status));
    ErrorHandler();
    return;
  }
  {
    const std::uint32_t tick_start = HAL_GetTick();

    do {
      (void) std::printf("c");
#if defined(COMPILATION_WITH_FREERTOS)
      /* Be cooperative. */
      vTaskDelay(10U);
#else
      this->emw.checkNotified(10U /* timeout */);
#endif /* COMPILATION_WITH_FREERTOS) */
      if (CheckTimeout(tick_start, 10000U)) {
        (void) std::printf("Not connected in the 10 last seconds.\n");
        break;
      }
    }
    while (!AppWiFiEmw::EmwInterfaceSoftApUp);
  }
  {
    EmwApiCore::MacAddress_t mac_address_48bits;
    status = this->emw.getSoftApMacAddress(mac_address_48bits);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      (void) std::printf("Failed to get SoftAP MAC address (%" PRId32 ")\n", static_cast<std::int32_t>(status));
    }
    else {
      (void) std::printf("SoftAP MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
                         mac_address_48bits.bytes[0], mac_address_48bits.bytes[1],
                         mac_address_48bits.bytes[2], mac_address_48bits.bytes[3],
                         mac_address_48bits.bytes[4], mac_address_48bits.bytes[5]);
    }
  }
  STD_PRINTF("AppWiFiEmw::enableSoftAp()>\n\n")
}

void AppWiFiEmw::EmwInterfaceStatusChanged(EmwApiBase::EmwInterface interface,
    enum EmwApiBase::WiFiEvent status,
    void *argPtr) noexcept
{
  static_cast<void>(argPtr);
  if (EmwApiBase::eSTATION == interface) {
    switch (status) {
      case EmwApiBase::eWIFI_EVENT_STA_DOWN: {
          AppWiFiEmw::EmwInterfaceStationUp = false;
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_STA_DOWN\n\n")
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_UP: {
          AppWiFiEmw::EmwInterfaceStationUp = true;
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_STA_UP\n\n")
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_GOT_IP: {
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_STA_GOT_IP\n")
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
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_AP_DOWN\n")
          break;
        }
      case EmwApiBase::eWIFI_EVENT_AP_UP: {
          AppWiFiEmw::EmwInterfaceSoftApUp = true;
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_AP_UP\n")
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
