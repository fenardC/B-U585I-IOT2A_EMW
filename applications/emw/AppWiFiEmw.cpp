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
#include <cstdbool>
#include <cstdio>
#include <cstring>
#include <memory>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

extern "C" void AppTaskFunction(void *argumentPtr);

AppWiFiEmw::AppWiFiEmw(void)
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
    (void) argumentPtr;

    STD_PRINTF("\nAppTaskFunction()>\n")

    std::printf("%s\n", the_application.emw.getConfigurationString());
    std::printf(" NETWORK_BUFFER_SIZE: %6" PRIu32 "\n\n", (uint32_t)EmwNetworkStack::NETWORK_BUFFER_SIZE);

    the_application.emw.resetHardware();
    the_application.emw.initialize();
    the_application.emw.registerStatusCallback(AppWiFiEmw::deviceStatusChanged, NULL, EmwApiBase::eSTATION);
    the_application.emw.registerStatusCallback(AppWiFiEmw::deviceStatusChanged, NULL, EmwApiBase::eSOFTAP);

    std::printf(" - Device Name    : %s.\n", the_application.emw.systemInformations.productName);
    std::printf(" - Device ID      : %s.\n", the_application.emw.systemInformations.productIdentifier);
    std::printf(" - Device Version : %s.\n", the_application.emw.systemInformations.firmwareRevision);
    std::printf(" - MAC address    : %02X.%02X.%02X.%02X.%02X.%02X\n\n",
                the_application.emw.systemInformations.mac48bitsStation[0],
                the_application.emw.systemInformations.mac48bitsStation[1],
                the_application.emw.systemInformations.mac48bitsStation[2],
                the_application.emw.systemInformations.mac48bitsStation[3],
                the_application.emw.systemInformations.mac48bitsStation[4],
                the_application.emw.systemInformations.mac48bitsStation[5]);
    {
      char version[] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
      uint32_t version_size = sizeof(version);
      the_application.emw.getVersion(version, version_size);
      std::printf("\nVersion: %s\n", version);
    }
    {
      EmwApiCore::MacAddress_t mac_address_48bits;
      the_application.emw.getStationMacAddress(mac_address_48bits);
      std::printf("\n%s Station MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
                  the_application.emw.systemInformations.productIdentifier,
                  mac_address_48bits.bytes[0], mac_address_48bits.bytes[1],
                  mac_address_48bits.bytes[2], mac_address_48bits.bytes[3],
                  mac_address_48bits.bytes[4], mac_address_48bits.bytes[5]);
    }
    {
      uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};
      the_application.emw.getIPAddress(ip_address_bytes, EmwApiBase::eSTATION);
      std::printf("IP (STA)   : %02X.%02X.%02X.%02X\n",
                  ip_address_bytes[0], ip_address_bytes[1],
                  ip_address_bytes[2], ip_address_bytes[3]);
    }
    {
      uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};
      the_application.emw.getIPAddress(ip_address_bytes, EmwApiBase::eSOFTAP);
      std::printf("IP (SOFTAP): %02X.%02X.%02X.%02X\n",
                  ip_address_bytes[0], ip_address_bytes[1],
                  ip_address_bytes[2], ip_address_bytes[3]);
    }
    std::printf("\nconnected: %s\n", the_application.emw.isConnected() == 1U ? "true" : "false");
    the_application.checkIoSpeed();
    the_application.enableSoftAp();
    {
      std::printf("\nWi-Fi scan\n");
      class AppConsoleScan scan(&the_application.emw);
      if (EmwApiBase::eEMW_STATUS_OK != scan.execute(0, nullptr)) {
        std::printf("%s: Wi-Fi scan failed\n", scan.getName());
      }
    }
    std::printf("\nWi-Fi connection\n");
    the_application.connectAp(WIFI_SSID, WIFI_PASSWORD);

    {
      class AppConsoleEcho echo(&the_application.emw);
      class AppConsolePing ping(&the_application.emw);
      class AppConsoleScan scan(&the_application.emw);
      class AppConsoleStats stats(&the_application.emw);
      class Cmd *cmds[] = {&echo, &ping, &scan, &stats, nullptr};
      class Console the_console("app>", cmds);
      the_console.run();
    }

    the_application.disconnectAp();
    the_application.disableSoftAp();
    the_application.emw.unInitialize();
    for (;;) {
      std::printf(".");
#if defined(COMPILATION_WITH_FREERTOS)
      vTaskDelay(5000U);
#else
      HAL_Delay(5000U);
#endif /* COMPILATION_WITH_FREERTOS) */
    }
    STD_PRINTF("AppTaskFunction()<\n\n")
  }
}

void AppWiFiEmw::checkIoSpeed(void) const
{
  const uint16_t ipc_test_data_size = EmwNetworkStack::NETWORK_BUFFER_SIZE;
  std::unique_ptr<uint8_t[]> ipc_test_data_ptr(new uint8_t[ipc_test_data_size]);

  if (nullptr != ipc_test_data_ptr) {
    uint32_t send_count = 0U;
    uint32_t receive_count = 0U;
    uint32_t time_ms_count;

    std::printf("\nAppWiFiEmw::checkIoSpeed()> (80 x (%" PRIu32 " + %" PRIu32 ")) ...\n",
                static_cast<uint32_t>(ipc_test_data_size), static_cast<uint32_t>(ipc_test_data_size));
    time_ms_count = HAL_GetTick();
    for (uint32_t i = 0U; i < 80U; i++) {
      uint16_t echoed_length = ipc_test_data_size;
      const EmwApiBase::Status status = this->emw.testIpcEcho(ipc_test_data_ptr.get(), ipc_test_data_size,
                                        ipc_test_data_ptr.get(), &echoed_length, 5000U);

      if (EmwApiBase::eEMW_STATUS_OK != status) {
        std::printf("Failed to call testIpcEcho() (%" PRId32 ")\n", static_cast<int32_t>(status));
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
      std::printf("transferred: %" PRIu32 " bytes, time: %" PRIu32 " ms, Speed: %" PRIu32 " Kbps\n",
                  total_sent_and_received, time_ms_count, speed);
    }
  }
}

void AppWiFiEmw::connectAp(const char *ssidStringPtr, const char *passwordStringPtr)
{
  EmwApiBase::Status status;

  std::printf("\nAppWiFiEmw::connectAp()> joining \"%s\" with \"%s\" ...\n", ssidStringPtr, passwordStringPtr);
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
  status = this->emw.connect(ssidStringPtr, passwordStringPtr, EmwApiBase::eSEC_AUTO);
  if (EmwApiBase::eEMW_STATUS_OK != status) {
    std::printf("[%6" PRIu32 "] Cannot Join ... \"%s\" (%" PRId32 ")\n", HAL_GetTick(), ssidStringPtr,
                static_cast<int32_t>(status));
    ErrorHandler();
    return;
  }
  {
    const uint32_t tick_start = HAL_GetTick();
    while (!AppWiFiEmw::deviceStationUp) {
#if defined(COMPILATION_WITH_FREERTOS)
      vTaskDelay(10U);
#else
      this->emw.checkNotified(10U /* timeout */);
#endif /* COMPILATION_WITH_FREERTOS) */
      if (this->checkTimeout(tick_start, 10000U)) {
        std::printf("Not connected in the 10 last seconds.\n");
        break;
      }
    }
  }
  std::printf("\nConnected: %s\n", this->emw.isConnected() == 1U ? "true" : "false");
  if (1 == this->emw.isConnected()) {
    {
      uint8_t ip_address_bytes[4] = {0U, 0U, 0U, 0U};
      this->emw.getIPAddress(ip_address_bytes, EmwApiBase::eSTATION);
      if (EmwApiBase::eEMW_STATUS_OK != status) {
        std::printf("Failed to get IP address (%" PRId32 ")\n", static_cast<int32_t>(status));
      }
      std::printf("\nIP address      %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                  static_cast<uint32_t>(ip_address_bytes[0]), static_cast<uint32_t>(ip_address_bytes[1]),
                  static_cast<uint32_t>(ip_address_bytes[2]), static_cast<uint32_t>(ip_address_bytes[3]));
      std::printf("Gateway address %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                  static_cast<uint32_t>(this->emw.stationSettings.gatewayAddress[0]),
                  static_cast<uint32_t>(this->emw.stationSettings.gatewayAddress[1]),
                  static_cast<uint32_t>(this->emw.stationSettings.gatewayAddress[2]),
                  static_cast<uint32_t>(this->emw.stationSettings.gatewayAddress[3]));
      std::printf("DNS1 address    %02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 ".%02" PRIu32 "\n",
                  static_cast<uint32_t>(this->emw.stationSettings.dns1[0]), static_cast<uint32_t>(this->emw.stationSettings.dns1[1]),
                  static_cast<uint32_t>(this->emw.stationSettings.dns1[2]), static_cast<uint32_t>(this->emw.stationSettings.dns1[3]));
    }
    for (uint8_t interface = 0U; interface < EmwApiBase::eWIFI_INTERFACE_IDX_MAX; interface++) {
      for (uint8_t address_slot = 0U; address_slot < 3U; address_slot++) {
        uint8_t ip_addr[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
        const int32_t address_state = this->emw.getIP6AddressState(address_slot,
                                      static_cast<EmwApiBase::EmwInterface>(interface));
        std::printf("IPv6 address (%" PRIu32 ")(%" PRIu32 "): state %" PRIi32 "\n",
                    static_cast<uint32_t>(interface), static_cast<uint32_t>(address_slot), address_state);
        status = this->emw.getIP6Address(ip_addr, address_slot, static_cast<EmwApiBase::EmwInterface>(interface));
        if (EmwApiBase::eEMW_STATUS_OK != status) {
          std::printf("failed to get IPv6 address (%" PRId32 ")\n", static_cast<int32_t>(status));
        }
        std::printf("IPv6 address (%" PRIu32 ")(%" PRIu32 "): "
                    "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\n",
                    static_cast<uint32_t>(interface), static_cast<uint32_t>(address_slot),
                    ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3],
                    ip_addr[4], ip_addr[5], ip_addr[6], ip_addr[7],
                    ip_addr[8], ip_addr[9], ip_addr[10], ip_addr[11],
                    ip_addr[12], ip_addr[13], ip_addr[14], ip_addr[15]);
      }
    }
  }
  else {
    std::printf("Not connected\n\n");
    ErrorHandler();
    return;
  }
  STD_PRINTF("AppWiFiEmw::connectAp()<\n\n")
}

void AppWiFiEmw::disableSoftAp(void)
{
  STD_PRINTF("\nAppWiFiEmw::disableSoftAp()>\n")

  {
    const EmwApiBase::Status status = this->emw.stopSoftAp();
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("Failed to stop SoftAP (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  {
    const EmwApiBase::Status status = this->emw.unRegisterStatusCallback(EmwApiBase::eSOFTAP);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("Failed to unRegisterStatusCallback (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  STD_PRINTF("AppWiFiEmw::disableSoftAp()<\n\n")
}

void AppWiFiEmw::disconnectAp(void)
{
  STD_PRINTF("\nAppWiFiEmw::disconnect()>\n")

  {
    const EmwApiBase::Status status = this->emw.disconnect();
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("Failed to disconnect (%" PRId32 ")\n", static_cast<int32_t>(status));
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
      std::printf("Failed to unRegisterStatusCallback (%" PRId32 ")\n", static_cast<int32_t>(status));
      ErrorHandler();
      return;
    }
  }
  STD_PRINTF("AppWiFiEmw::disconnectAp()<\n\n")
}

void AppWiFiEmw::enableSoftAp(void)
{
  EmwApiBase::Status status;
  EmwApiBase::SoftApSettings_t access_point_settings;

  STD_PRINTF("\nAppWiFiEmw::enableSoftAp()>\n")

  (void)std::strncpy(access_point_settings.ssidString, "MyHotSpot",
                     sizeof(access_point_settings.ssidString) - 1);
  (void)std::strncpy(access_point_settings.passwordString, "",
                     sizeof(access_point_settings.passwordString) - 1);
  access_point_settings.channel = 8U;
  (void)std::strncpy(access_point_settings.ip.ipAddressLocal, "10.10.10.1",
                     sizeof(access_point_settings.ip.ipAddressLocal) - 1);
  (void)std::strncpy(access_point_settings.ip.gatewayAddress, "10.10.10.1",
                     sizeof(access_point_settings.ip.gatewayAddress) - 1);
  (void)std::strncpy(access_point_settings.ip.networkMask, "255.255.255.0",
                     sizeof(access_point_settings.ip.networkMask) - 1);
  AppWiFiEmw::deviceSoftApUp = false;
  std::printf("\nStart Software enabled Access Point with \"%s\"\n", access_point_settings.ssidString);
  status = this->emw.startSoftAp(&access_point_settings);
  if (EmwApiBase::eEMW_STATUS_OK != status) {
    std::printf("Failed to setup %s (%" PRId32 ")\n", access_point_settings.ssidString, static_cast<int32_t>(status));
    ErrorHandler();
    return;
  }
  {
    const uint32_t tick_start = HAL_GetTick();
    while (!AppWiFiEmw::deviceSoftApUp) {
#if defined(COMPILATION_WITH_FREERTOS)
      /* Be cooperative. */
      vTaskDelay(10U);
#else
      this->emw.checkNotified(10U /* timeout */);
#endif /* COMPILATION_WITH_FREERTOS) */
      if (this->checkTimeout(tick_start, 10000U)) {
        std::printf("Not connected in the 10 last seconds.\n");
        break;
      }
    }
  }
  {
    EmwApiCore::MacAddress_t mac_address_48bits;
    status = this->emw.getSoftApMacAddress(mac_address_48bits);
    if (EmwApiBase::eEMW_STATUS_OK != status) {
      std::printf("Failed to get SoftAP MAC address (%" PRId32 ")\n", static_cast<int32_t>(status));
    }
    else {
      std::printf("SoftAP MAC address %02X.%02X.%02X.%02X.%02X.%02X\n",
                  mac_address_48bits.bytes[0], mac_address_48bits.bytes[1],
                  mac_address_48bits.bytes[2], mac_address_48bits.bytes[3],
                  mac_address_48bits.bytes[4], mac_address_48bits.bytes[5]);
    }
  }
  STD_PRINTF("AppWiFiEmw::enableSoftAp()>\n\n")
}

void AppWiFiEmw::deviceStatusChanged(EmwApiBase::EmwInterface interface,
                                     enum EmwApiBase::WiFiEvent status,
                                     void *argPtr)
{
  (void)(argPtr);
  if (EmwApiBase::eSTATION == interface) {
    switch (status) {
      case EmwApiBase::eWIFI_EVENT_STA_DOWN: {
          AppWiFiEmw::deviceStationUp = false;
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_STA_DOWN\n\n")
          break;
        }
      case EmwApiBase::eWIFI_EVENT_STA_UP: {
          AppWiFiEmw::deviceStationUp = true;
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
          AppWiFiEmw::deviceSoftApUp = false;
          STD_PRINTF(" -> EmwApiBase::eWIFI_EVENT_AP_DOWN\n")
          break;
        }
      case EmwApiBase::eWIFI_EVENT_AP_UP: {
          AppWiFiEmw::deviceSoftApUp = true;
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

bool AppWiFiEmw::checkTimeout(uint32_t tickStart, uint32_t tickCount) const
{
  const uint32_t elapsed_ticks = HAL_GetTick() - tickStart;
  bool status = false;
  if (elapsed_ticks > tickCount) {
    status = true;
  }
  return status;
}

volatile bool AppWiFiEmw::deviceSoftApUp = false;
volatile bool AppWiFiEmw::deviceStationUp = false;
