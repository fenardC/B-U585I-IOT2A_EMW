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
#include <stdbool.h>
#include <string.h>

//#define STD_PRINTF(...) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

int32_t AppConsoleScan::execute(int32_t argc, char *argvPtrs[])
{
  static EmwApiBase::ApInfo_t APs[10];
  const EmwApiBase::ApInfo_t ap_default;
  int8_t count = 0;
  class EmwApiEmw *emw_ptr = static_cast<EmwApiEmw *>(this->contextPtr);
  EmwApiBase::Status status;

  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  STD_PRINTF("\nAppConsoleScan::execute()>\n")
  status = emw_ptr->scan(EmwApiBase::ePASSIVE, nullptr, 0);

  if (EmwApiBase::eEMW_STATUS_OK == status) {
    for (auto i = 0U; i < (sizeof(APs) / sizeof(APs[0])); i++) {
      APs[i] = ap_default;
    }
    count = emw_ptr->getScanResults(reinterpret_cast<uint8_t *>(APs), 11);
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
                  AppConsoleScan::securityToString(emw_security[APs[i].security]), ".CN",
                  APs[i].bssid[0], APs[i].bssid[1], APs[i].bssid[2], APs[i].bssid[3], APs[i].bssid[4],
                  APs[i].bssid[5]);
    }
    std::printf("######### End of Scan ##########\n");
  }
  else {
    std::printf("%s: operation failed (%" PRIi32 ")!\n", this->getName(), (int32_t)status);
  }
  STD_PRINTF("\nAppConsoleScan::execute()<\n\n")
  return status;
}

const char *AppConsoleScan::securityToString(enum EmwApiBase::SecurityType security)
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
