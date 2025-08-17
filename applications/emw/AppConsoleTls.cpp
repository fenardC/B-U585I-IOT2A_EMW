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
#include "AppConsoleTls.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <system_error>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort);


AppConsoleTls::AppConsoleTls(EmwApiEmw& emw) noexcept
  : emw(emw)
{
  STD_PRINTF("AppConsoleTls::AppConsoleTls()>\n")
  STD_PRINTF("AppConsoleTls::AppConsoleTls(): %p\n", static_cast<const void*>(&emw))
  STD_PRINTF("AppConsoleTls::AppConsoleTls()<\n")
}

AppConsoleTls::~AppConsoleTls(void) noexcept
{
  STD_PRINTF("AppConsoleTls::~AppConsoleTls()>\n")
  STD_PRINTF("AppConsoleTls::~AppConsoleTls()< %p\n", static_cast<const void*>(&emw))
}

std::int32_t AppConsoleTls::execute(std::int32_t argc, char *argvPtrs[]) noexcept
{
  std::int32_t status;

  static_cast<void>(argc);
  static_cast<void>(argvPtrs);

  STD_PRINTF("\nAppConsoleTls::execute()>\n")

  (void) std::printf("%s: <%s>\n", this->getName(), AppConsoleTls::HTTP_HOST);
  status = this->doTls(AppConsoleTls::HTTP_HOST, false);
  STD_PRINTF("\nAppConsoleTls::execute()<\n\n")
  return status;
}

std::int32_t AppConsoleTls::doTls(const char (&peerNameString)[128], bool checkCa) noexcept
{
  std::int32_t status = -1;
  EmwAddress::SockAddrIn_t s_address_in(HostToNetworkShort(AppConsoleTls::HTTP_PORT), 0);
  EmwApiBase::Mtls_t tls_magic = nullptr;

  STD_PRINTF("\nAppConsoleTls::doTls()>\n")

  try {
    if (0 > this->emw.socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t &>(s_address_in),
                                          reinterpret_cast<const char(&)[255]>(peerNameString))) {
      throw std::runtime_error("socketGetHostByName() failed");
    }
    {
      char server_name_information_string[128] = {""};
      const EmwAddress::IpAddr_t ip4_addr(s_address_in.inAddr.addr);
      char ip4_addr_string[] = {"000.000.000.000"};
      const char (&ca_string)[] = {""};

      EmwAddress::NetworkToAscii(ip4_addr, ip4_addr_string, sizeof(ip4_addr_string));
      (void) std::printf("%s: connecting \"%s\": with \"%s\"|:%04" PRIx32 "\n", this->getName(), peerNameString,
                         ip4_addr_string, static_cast<std::uint32_t>(s_address_in.port));

      if (emw.tlsSetVersion(EmwApiEmw::TLS_V1_2_MODE) < 0) {
        throw std::runtime_error("tlsSetVersion() failed");
      }

      std::strncpy(server_name_information_string, peerNameString, 127);

      tls_magic \
        = reinterpret_cast<EmwApiBase::Mtls_t>(emw.tlsConnectSni(server_name_information_string,
          std::strlen(server_name_information_string),
          reinterpret_cast<EmwAddress::SockAddrStorage_t &>(s_address_in), sizeof(s_address_in),
          reinterpret_cast<const char (&)[2500]>(ca_string), checkCa ? std::strlen(ca_string) : 0));

      if (nullptr == tls_magic) {
        throw std::runtime_error("tlsConnectSni() failed");
      }
      (void) std::printf("%s: device connected to %s\n", this->getName(), ip4_addr_string);

      if (0 != this->doTlsExchanges(tls_magic)) {
        throw std::runtime_error("doTlsExchanges() failed");
      }

      if (0 == this->emw.tlsClose(tls_magic)) {
        (void) std::printf("%s: closed\n", this->getName());
      }
    }
  }
  catch (const std::runtime_error &error) {
    (void) std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
    (void) this->emw.tlsClose(tls_magic);
  }
  STD_PRINTF("\nAppConsoleTls::doTls()<\n\n")

  return status;
}

std::int32_t AppConsoleTls::doTlsExchanges(EmwApiBase::Mtls_t tlsMagic) noexcept
{
  std::int32_t status = 0;

  STD_PRINTF("\nAppConsoleTls::doTlsExchanges()>\n")

  try {
    if (emw.tlsSend(tlsMagic, AppConsoleTls::HTTP_REQUEST,
                    std::strlen(reinterpret_cast<const char *>(&AppConsoleTls::HTTP_REQUEST[0]))) <= 0) {
      throw std::runtime_error("tlsSend() failed");
    }

    {
      static std::uint8_t response[800] = {""};
      const std::int32_t response_length = sizeof(response) - 1;
      std::int32_t read_count = 0;
      std::int32_t response_count = 0;
      char *date_string_ptr = nullptr;

      do {
        response_count = this->emw.tlsReceive(tlsMagic, reinterpret_cast<std::uint8_t(&)[]>(* &response[read_count]),
                                              response_length - read_count);
        read_count += response_count;
        date_string_ptr = std::strstr(reinterpret_cast<char *>(&response[0]), "Date: ");
      }
      while ((nullptr == date_string_ptr) \
             && ((response_count >= 0) \
                 || (response_count == -2)) && (read_count < response_length));

      date_string_ptr[36] = '\0';
      (void) std::printf("%s: ... %s\n", this->getName(), date_string_ptr);
    }
  }
  catch (const std::runtime_error &error) {
    (void) std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
  }
  STD_PRINTF("\nAppConsoleTls::doTlsExchanges()<\n\n")

  return status;
}

const char AppConsoleTls::HTTP_HOST[128] = {"www.google.com"};
const std::uint8_t AppConsoleTls::HTTP_REQUEST[] = {"HEAD / HTTP/1.1\r\nHost: www.google.com\r\n\r\n"};

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort)
{
  return (((static_cast<std::uint16_t>(hostShort) & 0xFF00U) >> 8U) |
          ((static_cast<std::uint16_t>(hostShort) & 0x00FFU) << 8U));
}
