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
#include "AppConsoleDownload.hpp"
#include "EmwAddress.hpp"
#include "EmwApiEmw.hpp"
#include "EmwNetworkStack.hpp"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <system_error>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort);
static const std::uint8_t *SeekTo(const std::uint8_t *stringPtr, std::uint8_t key) noexcept;
static const std::uint8_t *SeekWhile(const std::uint8_t *stringPtr, std::uint8_t key) noexcept;
static const std::uint8_t *SeekWhileNot(const std::uint8_t *stringPtr, std::uint8_t key) noexcept;
static std::uint32_t ServiceLines(const std::uint8_t *bufferPtr, const std::uint8_t *(&linePtrs)[10]) noexcept;
static void SplitHostRequest(const char *urlStringPtr,
                             char (&hostString)[255], const char * &requestStringPtr) noexcept;

AppConsoleDownload::AppConsoleDownload(EmwApiEmw& wifiEmw) noexcept
  : emw(wifiEmw)
  , socket(-1)
  , userDownloadLength(0)
{
  DEBUG_STD_PRINTF(" AppConsoleDownload::AppConsoleDownload()>\n")
  DEBUG_STD_PRINTF(" AppConsoleDownload::AppConsoleDownload()< %p\n", static_cast<const void*>(&this->emw))
}

AppConsoleDownload::~AppConsoleDownload(void) noexcept
{
  DEBUG_STD_PRINTF(" AppConsoleDownload::~AppConsoleDownload()>\n")
  DEBUG_STD_PRINTF(" AppConsoleDownload::~AppConsoleDownload()< %p\n", static_cast<const void*>(&this->emw))
}

std::int32_t AppConsoleDownload::execute(std::int32_t argc, const char *argvPtrs[]) noexcept
{
  std::int32_t status = -1;
  const char *host_request_string_ptr = nullptr;
  bool use_ipv6 = false;
  char host_name[255] = {};
  const char *http_request_ptr;

  DEBUG_STD_PRINTF(" AppConsoleDownload::execute()>\n")

  this->userDownloadLength = AppConsoleDownload::DOWNLOAD_LIMITED_SIZE;
  host_name[0] = '\0';
  for (std::int32_t i = 1; i < argc; ++i) {
    std::string_view argv = argvPtrs[i] ? argvPtrs[i] : "";

    if (argv == "-6") {
      use_ipv6 = true;
    }
    else if (argv.starts_with("-l")) {
      auto length = argv.substr(2);
      if (false == length.empty()) {
        this->userDownloadLength = static_cast<std::uint32_t>(std::atoi(length.data()));
      }
    }
    else if (false == argv.empty()) {
      if (host_request_string_ptr != nullptr) {
        STD_PRINTF(" %s: error: multiple hosts provided: \"%s\" and \"%s\"\n",
                   this->getName(), host_request_string_ptr, argv.data());
        DEBUG_STD_PRINTF("\n AppConsoleDownload::execute()<\n")
        return -1;
      }
      host_request_string_ptr = argv.data();
    }
  }

  if (nullptr == host_request_string_ptr) {
    host_request_string_ptr = AppConsoleDownload::HOST_REQUEST_STRING;
    STD_PRINTF(" %s: default request: \"%s\"\n", this->getName(), host_request_string_ptr);
  }

  STD_PRINTF(" %s: <%s>\n", this->getName(), host_request_string_ptr);
  SplitHostRequest(host_request_string_ptr, host_name, http_request_ptr);

  if (nullptr == http_request_ptr) {
    STD_PRINTF(" %s: invalid URL \"%s\"\n", this->getName(), host_request_string_ptr);
  }
  else {
    STD_PRINTF(" %s: host \"%s\"\n", this->getName(), host_name);
    STD_PRINTF(" %s: req \"%s\"\n", this->getName(), http_request_ptr);

    if (use_ipv6) {
      status = this->doDownload6(host_name, http_request_ptr);
    }
    else {
      status = this->doDownload(host_name, http_request_ptr);
    }
  }
  DEBUG_STD_PRINTF("\n AppConsoleDownload::execute()<\n\n")
  return status;
}

std::int32_t AppConsoleDownload::doDownload(const char *hostPtr, const char *requestPtr) noexcept
{
  std::int32_t status = 0;
  EmwAddress::SockAddrIn_t sock_address_in(HostToNetworkShort(AppConsoleDownload::REMOTE_TCP_PORT), 0);

  DEBUG_STD_PRINTF("\n AppConsoleDownload::doDownload()>\n")

  if (0 > this->emw.socketGetHostByName(reinterpret_cast<EmwAddress::SockAddr_t &>(sock_address_in),
                                        reinterpret_cast<const char(&)[255]>(*hostPtr))) {
    STD_PRINTF(" %s: failed to find the host name \"%s\"\n", this->getName(), hostPtr);
  }
  else {
    const EmwAddress::IpAddr_t ip4_addr(sock_address_in.inAddr.addr);
    char ip4_address_string[16] = {"000.000.000.000"};
    std::int32_t socket = -1;
    EmwAddress::NetworkToAscii(ip4_addr, ip4_address_string);

    STD_PRINTF(" %s: downloading \"%s\" %" PRIu32 " with \"%s\"\n",
               this->getName(), hostPtr, this->userDownloadLength, ip4_address_string);
    try {
      this->socket = this->emw.socketCreate(EMW_AF_INET, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
      if (this->socket < 0) {
        throw std::runtime_error("socketCreate() failed");
      }
      {
        const std::int32_t timeout_in_ms = AppConsoleDownload::TIMEOUT_10S_DEFINED;

        std::int32_t socket_status = this->emw.socketSetSockOpt(this->socket,
                                     EMW_SOL_SOCKET, EmwSockOptVal::eEMW_SO_RCVTIMEO,
                                     &timeout_in_ms, sizeof(timeout_in_ms));
        if (socket_status == -1) {
          throw std::runtime_error("socketSetSockOpt() for receiving failed");
        }
        socket_status = this->emw.socketSetSockOpt(this->socket,
                        EMW_SOL_SOCKET, EmwSockOptVal::eEMW_SO_SNDTIMEO,
                        &timeout_in_ms, sizeof(timeout_in_ms));
        if (socket_status == -1) {
          throw std::runtime_error("socketGetSockOpt() for sending failed");
        }
      }
      {
        const std::int32_t connect_status = this->emw.socketConnect(this->socket,
                                            reinterpret_cast<const EmwAddress::SockAddr_t &>(sock_address_in),
                                            sizeof(sock_address_in));
        if (0 != connect_status) {
          throw std::runtime_error("socketConnect() failed");
        }
      }
      STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip4_address_string);
      status = this->doDownloadFile(hostPtr, requestPtr);
    }
    catch (const std::runtime_error &error) {
      STD_PRINTF(" %s: %s\n", this->getName(), error.what());
      status = -1;
      if (0 <= socket) {
        static_cast<void>(this->emw.socketClose(this->socket));
      }
    }
  }
  return status;
}

std::int32_t AppConsoleDownload::doDownload6(const char *hostPtr, const char *requestPtr) noexcept
{
  std::int32_t status = 0;
  const EmwAddress::AddrInfo_t hints(0, EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
  const char (&empty)[] = {""};

  EmwAddress::AddrInfo_t result;

  DEBUG_STD_PRINTF("\n AppConsoleDownload::doDownload6()>\n")

  if (0 > this->emw.socketGetAddrInfo(reinterpret_cast<const char(&)[255]>(*hostPtr),
                                      reinterpret_cast<const char (&)[255]>(empty), hints, result)) {
    STD_PRINTF("%s: failed to find the host name \"%s\"\n", this->getName(), hostPtr);
  }
  else {
    char ip6_address_string[40] = {"0000:0000:0000:0000:0000:0000:0000:0000"};
    const EmwAddress::Ip6Addr_t ip6_addr(result.sAddr.data2[1], result.sAddr.data2[2],
                                         result.sAddr.data3[0], result.sAddr.data3[1]);

    EmwAddress::NetworkToAscii(ip6_addr, ip6_address_string);

    STD_PRINTF("%s: downloading \"%s\" %" PRIu32 " with \"%s\"\n",
               this->getName(), hostPtr, this->userDownloadLength, ip6_address_string);
    try {
      EmwAddress::SockAddrIn6_t sock_address_in6;

      this->socket = this->emw.socketCreate(EMW_AF_INET6, EMW_SOCK_STREAM, EMW_IPPROTO_TCP);
      if (this->socket < 0) {
        throw std::runtime_error("socketCreate() failed");
      }
      sock_address_in6.port = HostToNetworkShort(AppConsoleDownload::REMOTE_TCP_PORT);
      sock_address_in6.in6Addr.un.u32Addr[0] = result.sAddr.data2[1];
      sock_address_in6.in6Addr.un.u32Addr[1] = result.sAddr.data2[2];
      sock_address_in6.in6Addr.un.u32Addr[2] = result.sAddr.data2[0];
      sock_address_in6.in6Addr.un.u32Addr[3] = result.sAddr.data3[1];

      {
        const std::int32_t connect_status = this->emw.socketConnect(this->socket,
                                            reinterpret_cast<const EmwAddress::SockAddr_t &>(sock_address_in6),
                                            sizeof(sock_address_in6));
        if (0 != connect_status) {
          throw std::runtime_error("socketConnect() failed");
        }
      }
      STD_PRINTF(" %s: device connected to %s\n", this->getName(), ip6_address_string);
      status = this->doDownloadFile(hostPtr, requestPtr);
    }
    catch (const std::runtime_error &error) {
      STD_PRINTF(" %s: %s\n", this->getName(), error.what());
      status = -1;
      static_cast<void>(this->emw.socketClose(this->socket));
    }
  }
  DEBUG_STD_PRINTF("\n AppConsoleDownload::doDownload6()<\n")
  return status;
}

std::int32_t AppConsoleDownload::doDownloadFile(const char *hostPtr,
    const char *requestPtr) const noexcept
{
  std::int32_t status = -1;
  const std::uint32_t buffer_size = EmwNetworkStack::NETWORK_BUFFER_SIZE;
  std::unique_ptr<std::uint8_t[]> buffer_ptr(new std::uint8_t[buffer_size]);

  STD_PRINTF(" %s: downloading file %s from \"%s\"\n", this->getName(), requestPtr, hostPtr);

  if (nullptr == buffer_ptr) {
    STD_PRINTF(" %s: memory allocation failed\n", this->getName());
  }
  try {
    std::uint32_t start_time_in_ms = 0;
    std::uint32_t local_start_time_in_ms = 0;
    AppConsoleDownload::HttpContext_t http_ctx;
    std::uint32_t ret_size = 0;
    std::uint64_t length = 0;
    std::uint32_t cpt_count = 0;

    const std::string http_request =
      "GET " + std::string(requestPtr) + " HTTP/1.1\r\n"
      "Host: " + std::string(hostPtr) + "\r\n"
      "User-Agent: EMW\r\n"
      "Connection: close\r\n\r\n";

    *(buffer_ptr.get() + buffer_size - 1) = '\0';

    this->emw.socketSend(this->socket,
                         reinterpret_cast<const std::uint8_t (&)[]>(*http_request.data()),
                         http_request.size(), 0);

    if (this->readResponse(buffer_ptr.get(), buffer_size, ret_size) != 0) {
      throw std::runtime_error("no answer from HTTP server");
    }

    if (this->testResponse(http_ctx, buffer_ptr.get()) != 0) {
      *(buffer_ptr.get() + (buffer_size / 8U)) = '\0'; /* Arbitrary set a end of string. */
      STD_PRINTF(" %s  <--- \"%s\"\n\n", this->getName(), buffer_ptr.get());
      throw std::runtime_error("incorrect HTTP server response");
    }

    length = http_ctx.contentLength;
    STD_PRINTF(" %s: file size %" PRIu32 " bytes\n", this->getName(), static_cast<std::uint32_t>(length));

    if (length > this->userDownloadLength) {
      STD_PRINTF(" %s: limiting transfer to first %" PRIu32 " bytes with report time of %" PRIu32 " ms\n",
                 this->getName(), this->userDownloadLength, AppConsoleDownload::REPORT_TIMEPERIOD_MS);
      length = this->userDownloadLength;
    }

    local_start_time_in_ms = start_time_in_ms = HAL_GetTick();

    while (length) {
      std::uint32_t elapsed_time_in_ms;
      const std::uint32_t size_in_bytes = (buffer_size < length) ? buffer_size : static_cast<std::uint32_t>(length);

      const std::int32_t count = this->emw.socketReceive(this->socket,
                                 reinterpret_cast<std::uint8_t (&)[]>(* buffer_ptr.get()),
                                 size_in_bytes, 0);
      if (count == -1) {
        break;
      }
      if (count == 0) {
        status = 0;
        break;
      }

      STD_PRINTF(".");

      cpt_count += static_cast<std::uint32_t>(count);

      elapsed_time_in_ms = HAL_GetTick() - local_start_time_in_ms;

      if (elapsed_time_in_ms > REPORT_TIMEPERIOD_MS) {
        const std::uint32_t ref_band_width = (8U * cpt_count) / elapsed_time_in_ms;
        STD_PRINTF("\n %s: transfer %" PRIu32 " bytes, remain %" PRIu32 " bytes, bitrate %" PRIu32 " kbit/s\n",
                   this->getName(), cpt_count, static_cast<std::uint32_t>(length), ref_band_width);
        cpt_count = 0U;
        local_start_time_in_ms = start_time_in_ms = HAL_GetTick();
      }

      if (length >= static_cast<std::uint64_t>(count)) {
        length -= static_cast<std::uint64_t>(count);
      }
      else {
        length = 0;
      }
    }
    static_cast<void>(this->emw.socketShutDown(this->socket, 2));
    static_cast<void>(this->emw.socketClose(this->socket));
    {
      const std::uint32_t duration_in_ms = HAL_GetTick() - start_time_in_ms;
      if (0 != duration_in_ms) {
        const std::uint32_t ref_band_width = (8 * cpt_count) / duration_in_ms;

        STD_PRINTF("\n %s: transfer %" PRIu32 " bytes, duration %" PRIu32 " ms, bitrate %" PRIu32 " kbit/s\n",
                   this->getName(), cpt_count, duration_in_ms, ref_band_width);
      }
    }
  }
  catch (const std::runtime_error &error) {
    STD_PRINTF(" %s: %s\n", this->getName(), error.what());
    status = -1;
    static_cast<void>(this->emw.socketShutDown(this->socket, 2));
    static_cast<void>(this->emw.socketClose(this->socket));
  }
  return status;
}

std::int32_t AppConsoleDownload::readResponse(std::uint8_t *headerPtr, std::uint32_t maximumSize,
    std::uint32_t &retSize) const noexcept
{
  char *string_ptr = reinterpret_cast<char *>(headerPtr);
  std::uint32_t header_size = maximumSize;
  std::uint32_t count = 0U;
  char last_4_chars[4] = {0, 0, 0, 0};

  retSize = 0U;

  while (header_size > 0) {
    const std::int32_t ret = this->emw.socketReceive(this->socket,
                             reinterpret_cast<std::uint8_t (&)[]>(* string_ptr), 1, 0);
    if (ret < 0) {
      return ret;
    }
// if (ret == 0) {
// return ret;
// }

    last_4_chars[0] = last_4_chars[1];
    last_4_chars[1] = last_4_chars[2];
    last_4_chars[2] = last_4_chars[3];
    last_4_chars[3] = *string_ptr;

    string_ptr++;
    header_size--;
    count++;

    if (std::memcmp(last_4_chars, "\r\n\r\n", 4) == 0) {
      break;
    }
  }

  if (header_size == 0) {
    return -1;
  }

  retSize = count;

  return 0;
}

std::int32_t AppConsoleDownload::testResponse(AppConsoleDownload::HttpContext_t &context,
    std::uint8_t *bufferPtr) const noexcept
{
  const std::uint8_t *line_ptrs[10] = {nullptr};
  const std::uint32_t line_ptr_count = ServiceLines(bufferPtr, line_ptrs);

  context.contentLength = -1;
  context.status = AppConsoleDownload::HTTP_RESPONSE_BAD_REQUEST;
  context.posFile = 0;

  if (line_ptr_count < 1) {
    return -1;
  }
  {
    const std::uint8_t *string_ptr = line_ptrs[0];
    string_ptr = SeekWhileNot(string_ptr, ' ');
    string_ptr = SeekWhile(string_ptr, ' ');
    std::sscanf(reinterpret_cast<const char*>(string_ptr), "%" PRIu32 "", &context.status);
  }
  for (std::uint32_t i = 0U ; i < line_ptr_count ; i++) {
    const char *line_ptr = reinterpret_cast<const char *>(line_ptrs[i]);
    const char *key = "Content-Length:";
    const std::size_t key_length = std::strlen(key);

    if (std::strncmp(line_ptr, key, key_length) == 0) {
      const std::uint8_t *param_ptr = SeekTo(line_ptrs[i], ':');

      param_ptr++;
      param_ptr = SeekWhile(param_ptr, ' ');
      if (nullptr != param_ptr) {
        std::uint64_t length = 0U;
        std::sscanf(reinterpret_cast<const char*>(param_ptr), "%" PRIu64 "", &length);
        context.contentLength = static_cast<std::int64_t>(length);
      }
    }
  }
  if ((context.status < AppConsoleDownload::HTTP_RESPONSE_OK) \
      || (context.status >= AppConsoleDownload::HTTP_RESPONSE_MULTIPLE_CHOICES)) {
    return -1;
  }
  return 0;
}

const char AppConsoleDownload::HOST_REQUEST_STRING[255] = {"http://test-debit.free.fr/image.iso"};


static const std::uint8_t *SeekTo(const std::uint8_t *stringPtr, std::uint8_t key) noexcept
{
  if (nullptr != stringPtr) {
    while (*stringPtr && (*stringPtr != key)) {
      stringPtr++;
    }
    if (*stringPtr) {
      stringPtr++;
    }
  }
  return stringPtr;
}

static const std::uint8_t *SeekWhile(const std::uint8_t *stringPtr, std::uint8_t key) noexcept
{
  if (nullptr != stringPtr) {
    while (*stringPtr && (*stringPtr == key)) {
      stringPtr++;
    }
  }
  return stringPtr;
}

static const std::uint8_t *SeekWhileNot(const std::uint8_t *stringPtr, std::uint8_t key) noexcept
{
  if (nullptr != stringPtr) {
    while (*stringPtr && (*stringPtr != key)) {
      stringPtr++;
    }
  }
  return stringPtr;
}

static std::uint32_t ServiceLines(const std::uint8_t *bufferPtr, const std::uint8_t *(&linePtrs)[10]) noexcept
{
  std::uint32_t index = 0U;

  if (nullptr != bufferPtr) {
    const std::uint32_t max_lines = std::size(linePtrs);

    while ((0U != *bufferPtr) && (index < max_lines)) {
      linePtrs[index++] = bufferPtr;
      while ((0U != *bufferPtr) && ('\n' != *bufferPtr)) {
        bufferPtr++;
      }
      if ('\n' == *bufferPtr) {
        bufferPtr++;
      }
    }
  }
  return index;
}

static void SplitHostRequest(const char *urlStringPtr,
                             char (&hostString)[255], const char * &requestStringPtr) noexcept
{
  if ((nullptr == urlStringPtr) || (*urlStringPtr == '\0')) {
    hostString[0] = '\0';
    requestStringPtr = nullptr;
  }
  else {
    const char *url_string_ptr = urlStringPtr;

    if (std::strncmp(url_string_ptr, "http://", 7) == 0) {
      url_string_ptr += 7;
    }
    else if (std::strncmp(url_string_ptr, "https://", 8) == 0) {
      url_string_ptr += 8;
    }

    {
      const char *const slash_ptr = std::strchr(url_string_ptr, '/');

      if (nullptr == slash_ptr) {
        std::size_t length = std::min(std::strlen(url_string_ptr), sizeof(hostString) - 1);
        std::memcpy(hostString, url_string_ptr, length);
        hostString[length] = '\0';

        requestStringPtr = "/";
      }
      else {
        std::size_t host_length = std::min<std::size_t>(slash_ptr - url_string_ptr, sizeof(hostString) - 1);
        std::memcpy(hostString, url_string_ptr, host_length);
        hostString[host_length] = '\0';

        requestStringPtr = slash_ptr;
      }
    }
  }
}

static std::uint16_t HostToNetworkShort(std::uint16_t hostShort)
{
  return (((static_cast<std::uint16_t>(hostShort) & 0xFF00U) >> 8U) |
          ((static_cast<std::uint16_t>(hostShort) & 0x00FFU) << 8U));
}
