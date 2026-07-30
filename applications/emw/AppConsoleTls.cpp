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
#include "AppConsoleTls.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort);


AppConsoleTls::AppConsoleTls(EmwApiEmw& wifiEmw) noexcept
  : emw(wifiEmw)
{
  DEBUG_STD_PRINTF(" AppConsoleTls::AppConsoleTls()>\n")
  DEBUG_STD_PRINTF(" AppConsoleTls::AppConsoleTls()< %p\n", static_cast<const void*>(&this->emw))
}

AppConsoleTls::~AppConsoleTls(void) noexcept
{
  DEBUG_STD_PRINTF(" AppConsoleTls::~AppConsoleTls()>\n")
  DEBUG_STD_PRINTF(" AppConsoleTls::~AppConsoleTls()< %p\n", static_cast<const void*>(&this->emw))
}

std::int32_t AppConsoleTls::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status;

  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  DEBUG_STD_PRINTF("\n AppConsoleTls::execute()>\n")

  STD_PRINTF(" %s: <%s>\n", this->getName(), AppConsoleTls::HTTP_HOST);
  status = this->doTls(AppConsoleTls::HTTP_HOST, false);
  DEBUG_STD_PRINTF("\n AppConsoleTls::execute()<\n\n")
  return status;
}

std::int32_t AppConsoleTls::doTls(const char (&peerNameString)[128], bool checkCa) noexcept
{
  std::int32_t status = -1;
  EmwAddress::SockAddrIn_t sock_address_in(HostToNetworkShort(AppConsoleTls::HTTP_PORT), 0);

  DEBUG_STD_PRINTF("\n AppConsoleTls::doTls()>\n")

  try {
    if (0 > this->emw.socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t &>(sock_address_in),
                                          reinterpret_cast<const char(&)[255]>(peerNameString))) {
      throw std::runtime_error("socketGetHostByName() failed");
    }
    {
      char server_name_information_string[128] = {""};
      const EmwAddress::IpAddr_t ip4_addr(sock_address_in.inAddr.addr);
      char ip4_addr_string[16] = {"000.000.000.000"};
      const char (&ca_string)[] = {""};

      EmwAddress::NetworkToAscii(ip4_addr, ip4_addr_string);
      STD_PRINTF(" %s: connecting \"%s\": with \"%s\"|:%04" PRIx32 "\n", this->getName(), peerNameString,
                 ip4_addr_string, static_cast<std::uint32_t>(sock_address_in.port));

      if (emw.tlsSetVersion(EmwApiEmw::TLS_V1_2_MODE) < 0) {
        throw std::runtime_error("tlsSetVersion() failed");
      }

      std::strncpy(server_name_information_string, peerNameString, 127);
      {
        const EmwApiBase::Mtls_t tls_key \
          = reinterpret_cast<EmwApiBase::Mtls_t>(emw.tlsConnectSni(server_name_information_string,
            std::strlen(server_name_information_string),
            reinterpret_cast<EmwAddress::SockAddrStorage_t &>(sock_address_in), sizeof(sock_address_in),
            reinterpret_cast<const char (&)[2500]>(ca_string), checkCa ? std::strlen(ca_string) : 0));

        if (0U == tls_key) {
          throw std::runtime_error("tlsConnectSni() failed");
        }
        STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip4_addr_string);

        if (0 != this->doTlsExchanges(tls_key)) {
          static_cast<void>(this->emw.tlsClose(tls_key));
          throw std::runtime_error("doTlsExchanges() failed");
        }

        if (0 == this->emw.tlsClose(tls_key)) {
          STD_PRINTF(" %s: closed\n", this->getName());
        }
      }
    }
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
  }
  DEBUG_STD_PRINTF("\n AppConsoleTls::doTls()<\n\n")

  return status;
}

std::int32_t AppConsoleTls::doTlsExchanges(EmwApiBase::Mtls_t tlsMagic) noexcept
{
  std::int32_t status = 0;

  DEBUG_STD_PRINTF("\n AppConsoleTls::doTlsExchanges()>\n")

  try {
    if (this->emw.tlsSend(tlsMagic, AppConsoleTls::HTTP_REQUEST,
                          std::strlen(reinterpret_cast<const char *>(&AppConsoleTls::HTTP_REQUEST[0]))) <= 0) {
      throw std::runtime_error("tlsSend() failed");
    }

    {
      static std::uint8_t response[800] = {};
      const std::int32_t max_length = sizeof(response) - 1;
      std::int32_t response_count = 0;
      char *date_string_start_ptr = nullptr;

      response[0] = '\0';
      response_count = this->emw.tlsReceive(tlsMagic, response, max_length);
      if (response_count <= 0) {
        throw std::runtime_error("tlsReceive() failed");
      }
      response[response_count] = '\0';
      date_string_start_ptr = std::strstr(reinterpret_cast<char*>(response), "Date: ");

      if (nullptr == date_string_start_ptr) {
        throw std::runtime_error("Date header not found");
      }
      else {
        const char *const date_string_end_ptr = std::strstr(date_string_start_ptr, "\r\n");
        const std::size_t date_length = date_string_end_ptr \
                                        ? static_cast<std::size_t>(date_string_end_ptr - date_string_start_ptr) : std::strlen(date_string_start_ptr);
        std::string date_line(date_string_start_ptr, std::min<std::size_t>(date_length, 63));
        STD_PRINTF(" %s: ... %s\n", this->getName(), date_line.c_str());
      }
    }
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
  }
  DEBUG_STD_PRINTF("\n AppConsoleTls::doTlsExchanges()<\n\n")

  return status;
}

const char AppConsoleTls::HTTP_HOST[128] = {"www.google.com"};
const std::uint8_t AppConsoleTls::HTTP_REQUEST[] = {"HEAD / HTTP/1.1\r\nHost: www.google.com\r\n\r\n"};

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort)
{
  return (((static_cast<std::uint16_t>(hostShort) & 0xFF00U) >> 8U) |
          ((static_cast<std::uint16_t>(hostShort) & 0x00FFU) << 8U));
}
