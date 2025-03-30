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
#include "AppConsoleScan.hpp"
#include "EmwApiEmw.hpp"
#include <inttypes.h>
#include <cstring>
#include <cstdio>
#include <cstring>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

static const char *SecurityToString(enum EmwApiBase::SecurityType security);


AppConsoleScan::AppConsoleScan(EmwApiEmw& emw) noexcept
  : emw(emw)
{
  STD_PRINTF("AppConsoleScan::AppConsoleScan()>\n")
  STD_PRINTF("AppConsoleScan::AppConsoleScan(): %p\n", static_cast<const void*>(&emw))
  STD_PRINTF("AppConsoleScan::AppConsoleScan()<\n")
}

AppConsoleScan::~AppConsoleScan(void)
{
  STD_PRINTF("AppConsoleScan::~AppConsoleScan()>\n")
  STD_PRINTF("AppConsoleScan::~AppConsoleScan()< %p\n", static_cast<const void*>(&emw))
}

std::int32_t AppConsoleScan::execute(std::int32_t argc, char *argvPtrs[]) noexcept
{
  static EmwApiBase::ApInfo_t APs[10];
  const EmwApiBase::ApInfo_t ap_default;
  std::int8_t count = 0;
  std::int32_t status = -1;
  const char ssid[33] = {""};

  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  STD_PRINTF("\nAppConsoleScan::execute()>\n")

  if (EmwApiBase::eEMW_STATUS_OK == this->emw.scan(EmwApiBase::ePASSIVE, ssid, 0)) {
    status = 0;

    for (auto &ap : APs) {
      ap = ap_default;
    }

    count \
      = this->emw.getScanResults(reinterpret_cast<std::uint8_t (&)[480]>(*reinterpret_cast<std::uint8_t *>(APs)),
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
    (void) std::printf("%s: operation failed (%" PRIi32 ")!\n", this->getName(), static_cast<std::int32_t>(status));
  }
  STD_PRINTF("\nAppConsoleScan::execute()<\n")
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
