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
#include "wifi_emw.hpp"
#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#endif /* COMPILATION_WITH_FREERTOS */
#include "stm32u5xx_hal.h"

#include <cinttypes>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#if defined(COMPILATION_WITH_EMW)
class EmwApiEmw Emw;
#elif defined(COMPILATION_WITH_LWIP)
class EmwApiEmwBypass EmwBypass;
#endif /* COMPILATION_WITH_EMW) */

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static void CheckEmwIoSpeed(EmwApiCore &emw) noexcept;
static const char *SecurityToString(enum EmwApiBase::SecurityType security);

void InitializeEmw(EmwApiCore &emw) noexcept
{
  char version[25] = {""};
  const std::uint32_t version_size = sizeof(version);
  EmwApiCore::MacAddress_t mac_address;

  STD_PRINTF("%s\n", EmwApiCore::getConfigurationString());
  STD_PRINTF("NETWORK_BUFFER_SIZE: %6" PRIu32 "\n\n",
             static_cast<std::uint32_t>(EmwNetworkStack::NETWORK_BUFFER_SIZE));

  STD_PRINTF("[%6" PRIu32 "] InitializeEmw(): REBOOT(HW) ...\n", HAL_GetTick());
  emw.resetHardware();
  emw.initialize();
  emw.getVersion(version, version_size);
  emw.getStationMacAddress(mac_address);

  STD_PRINTF(" - Device Name    : %s.\n", EmwApiCore::productName);
  STD_PRINTF(" - Device ID      : %s.\n", EmwApiCore::productIdentifier);
  STD_PRINTF(" - Device Version : %s.\n", version);
  STD_PRINTF(" - MAC address    : %02X.%02X.%02X.%02X.%02X.%02X\n\n",
             mac_address.bytes[0], mac_address.bytes[1], mac_address.bytes[2],
             mac_address.bytes[3], mac_address.bytes[4], mac_address.bytes[5]);

  CheckEmwIoSpeed(emw);
}

static void CheckEmwIoSpeed(EmwApiCore &emw) noexcept
{
#if defined(COMPILATION_WITH_NO_OS)
  constexpr std::uint16_t ipc_test_data_size = 2500U /*EmwNetworkStack::NETWORK_BUFFER_SIZE*/;
  std::unique_ptr<std::uint8_t[]> ipc_test_data_ptr(new (std::nothrow) std::uint8_t[ipc_test_data_size]);
  const std::uint32_t loop_count = 80U;
#elif defined(COMPILATION_WITH_FREERTOS)
  const std::uint16_t ipc_test_data_size = EmwNetworkStack::NETWORK_BUFFER_SIZE;
  std::unique_ptr<std::uint8_t[], decltype(&vPortFree)> \
  ipc_test_data_ptr(static_cast<std::uint8_t*>(pvPortMalloc(ipc_test_data_size)), &vPortFree);
  const std::uint32_t loop_count = 130U;
#endif /* COMPILATION_WITH_NO_OS */

  if (nullptr != ipc_test_data_ptr) {
    std::uint32_t send_count = 0U;
    std::uint32_t receive_count = 0U;
    std::uint32_t time_ms_count;

    std::memset(&ipc_test_data_ptr[0], 0x00, ipc_test_data_size);

    STD_PRINTF("\n[%6" PRIu32 "] Checking Emw Io Speed (%" PRIu32 " x (%" PRIu32 " + %" PRIu32 ")) ...\n",
               HAL_GetTick(), loop_count,
               static_cast<std::uint32_t>(ipc_test_data_size), static_cast<std::uint32_t>(ipc_test_data_size));

    time_ms_count = HAL_GetTick();

    for (std::uint32_t i = 0U; i < loop_count; i++) {
      std::uint16_t echoed_length = ipc_test_data_size;
      const EmwApiBase::Status status \
        = emw.testIpcEcho(reinterpret_cast<std::uint8_t (&)[]>(* &ipc_test_data_ptr[0]), ipc_test_data_size,
                          reinterpret_cast<std::uint8_t (&)[]>(* &ipc_test_data_ptr[0]), echoed_length);
      if (EmwApiBase::eEMW_STATUS_OK != status) {
        STD_PRINTF("Failed to call testIpcEcho() (%" PRId32 ")\n", static_cast<std::int32_t>(status));
        return;
      }
      else {
        send_count += ipc_test_data_size;
        receive_count += echoed_length;
      }
    }
    time_ms_count = HAL_GetTick() - time_ms_count;
    if (0U != time_ms_count) {
      const std::uint32_t total_sent_and_received = send_count + receive_count;
      const std::uint32_t speed = (8 * total_sent_and_received) / time_ms_count;
      STD_PRINTF("[%6" PRIu32 "] ... transferred: %" PRIu32 " bytes, "
                 "time: %" PRIu32 " ms, Speed: %" PRIu32 " Kbps\n",
                 HAL_GetTick(), total_sent_and_received, time_ms_count, speed);
    }
  }
}

std::int32_t Scan(EmwApiCore &emw) noexcept
{
  static EmwApiBase::ApInfo_t APs[10];
  const EmwApiBase::ApInfo_t ap_default;
  std::int8_t count = 0;
  std::int32_t status = -1;
  const char ssid[33] = {""};

  DEBUG_STD_PRINTF("\n Scan()>\n");

  if (EmwApiBase::eEMW_STATUS_OK == emw.scan(EmwApiBase::ePASSIVE, ssid, 0)) {
    status = 0;

    for (auto &ap : APs) {
      ap = ap_default;
    }

    count = emw.getScanResults(reinterpret_cast<std::uint8_t (&)[480]>(*reinterpret_cast<std::uint8_t *>(APs)), 11);
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

    STD_PRINTF(" ######### Scan %" PRIi32 " BSS ##########\n", static_cast<std::int32_t>(count));
    for (auto i = 0; i < count; i++) {
#define AP_SSID_LENGTH sizeof((static_cast<EmwApiBase::ApInfo_t *>(nullptr))->ssid)

      APs[i].ssid[AP_SSID_LENGTH - 1] = '\0';

      STD_PRINTF(" %2" PRIi32 "\t%32s ch %2" PRIi32 " rss %" PRIi32 " Security %10s"
                 " bssid %02x.%02x.%02x.%02x.%02x.%02x\n",
                 static_cast<std::int32_t>(i), APs[i].ssid, APs[i].channel, APs[i].rssi,
                 SecurityToString(emw_security[APs[i].security]),
                 APs[i].bssid[0], APs[i].bssid[1], APs[i].bssid[2], APs[i].bssid[3], APs[i].bssid[4],
                 APs[i].bssid[5]);
    }
    STD_PRINTF(" ######### End of Scan ##########\n");
  }
  else {
    STD_PRINTF(" Scan failed (%" PRIi32 ")!\n", static_cast<std::int32_t>(status));
  }
  DEBUG_STD_PRINTF("\n Scan()<\n");
  return status;
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
