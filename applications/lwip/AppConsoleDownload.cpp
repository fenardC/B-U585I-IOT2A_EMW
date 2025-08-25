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
#include "AppConsoleDownload.hpp"
#include "AppLwipServices.hpp"
#include "EmwNetworkStack.hpp"
#include "FreeRTOS.h"
#include "portable.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "stm32u5xx_hal.h"
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <system_error>

//#define STD_PRINTF(...) (void) std::printf(__VA_ARGS__);
#define STD_PRINTF(...)

AppConsoleDownload::AppConsoleDownload(void) noexcept
  : userDownloadLength(0)
{
  STD_PRINTF("AppConsoleDownload::AppConsoleDownload()>\n")
  STD_PRINTF("AppConsoleDownload::AppConsoleDownload(): %p\n", static_cast<const void*>(&emw))
  STD_PRINTF("AppConsoleDownload::AppConsoleDownload()<\n")
}

AppConsoleDownload::~AppConsoleDownload(void) noexcept
{
  STD_PRINTF("AppConsoleDownload::~AppConsoleDownload()>\n")
  STD_PRINTF("AppConsoleDownload::~AppConsoleDownload()< %p\n", static_cast<const void*>(&emw))
}

std::int32_t AppConsoleDownload::execute(std::int32_t argc, char *argvPtrs[]) noexcept
{
  std::int32_t status = -1;
  const char *host_request_string_ptr = AppConsoleDownload::HOST_REQUEST_STRING;
  struct sockaddr_in s_address_in = {sizeof(s_address_in), AF_INET, lwip_htons(AppConsoleDownload::REMOTE_TCP_PORT), {0}, {0}};
  struct sockaddr_in6 s_address_in6 = {sizeof(s_address_in6), AF_INET6, lwip_htons(AppConsoleDownload::REMOTE_TCP_PORT), 0, {{{0, 0, 0, 0}}}, 0};
  bool use_ipv6 = false;
  char *host_name_ptr = nullptr;
  const char *http_request_ptr;
  struct sockaddr *s_address_ptr = nullptr;

  STD_PRINTF("AppConsoleDownload::execute()>\n")

  this->userDownloadLength = AppConsoleDownload::DOWNLOAD_LIMITED_SIZE;

  if (argc == 1) {
    host_request_string_ptr = AppConsoleDownload::HOST_REQUEST_STRING;
  }
  else if (argc > 1) {
    for (std::int32_t i = 1; i < argc; i++) {
      if (nullptr != argvPtrs[i]) {
        if (0 == std::strncmp("-6", argvPtrs[i], 2)) {
          use_ipv6 = true;
        }
        else if (0 == std::strncmp("-l", argvPtrs[i], 2)) {
          this->userDownloadLength = static_cast<std::uint32_t>(std::atoi(argvPtrs[i] + 2));
        }
        else {
          host_request_string_ptr = argvPtrs[i];
        }
      }
    }
  }
  else {
    (void) std::printf("%s: error with bad argc %" PRId32 "!\n", this->getName(), argc);
    return -1;
  }

  (void) std::printf("%s: <%s>\n", this->getName(), host_request_string_ptr);
  this->splitHostRequest(host_request_string_ptr, host_name_ptr, http_request_ptr);

  if ((nullptr == host_name_ptr) || (nullptr == http_request_ptr)) {
    (void) std::printf("%s: invalid URL \"%s\"\n", this->getName(), host_request_string_ptr);
  }
  else {
    std::int32_t socket = -1;

    (void) std::printf("%s: host \"%s\"\n", this->getName(), host_name_ptr);
    (void) std::printf("%s: req \"%s\"\n", this->getName(), http_request_ptr);

    if (use_ipv6) {
      s_address_ptr = reinterpret_cast<struct sockaddr *>(&s_address_in6);
    }
    else {
      s_address_ptr = reinterpret_cast<struct sockaddr *>(&s_address_in);
    }

    try {
      if (AppLwipService::getHostByName(s_address_ptr, host_name_ptr) < 0) {
        throw std::runtime_error("getHostByName() failed");
      }
      if (use_ipv6) {
        ip_addr_t ip_addr = IPADDR6_INIT(0, 0, 0, 0);

        inet6_addr_to_ip6addr(&ip_addr.u_addr.ip6, &s_address_in6.sin6_addr);
        (void) std::printf("%s: -> \"%s\" with %s\n", this->getName(), host_name_ptr, ipaddr_ntoa(&ip_addr));
      }
      else {
        ip_addr_t ip_addr = IPADDR4_INIT(0);
        inet_addr_to_ip4addr(&ip_addr.u_addr.ip4, &s_address_in.sin_addr);
        (void) std::printf("%s: -> \"%s\" with %s\n", this->getName(), host_name_ptr, ipaddr_ntoa(&ip_addr));
      }

      socket = lwip_socket(s_address_ptr->sa_family, SOCK_STREAM, IPPROTO_TCP);
      if (-1 == socket) {
        throw std::runtime_error("lwip_socket() failed");
      }

      {
        const std::int32_t timeout_in_ms = AppConsoleDownload::TIMEOUT_10S_DEFINED;

        std::int32_t socket_status = lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
        if (socket_status == -1) {
          throw std::runtime_error("lwip_setsockopt() for receiving failed");
        }
        socket_status = lwip_setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout_in_ms, sizeof(timeout_in_ms));
        if (socket_status == -1) {
          throw std::runtime_error("lwip_setsockopt() for sending failed");
        }
      }
      {
        const std::int32_t connect_status = lwip_connect(socket, s_address_ptr, s_address_ptr->sa_len);
        if (0 != connect_status) {
          throw std::runtime_error("lwip_connect() failed");
        }
      }
      status = this->doDownloadFile(socket, host_name_ptr, http_request_ptr);
    }
    catch (const std::runtime_error &error) {
      (void) std::printf("%s: %s\n", this->getName(), error.what());
      status = -1;
      lwip_close(socket);
    }
  }
  STD_PRINTF("\nAppConsoleDownload::execute()<\n")
  return status;
}

std::int32_t AppConsoleDownload::doDownloadFile(std::int32_t socket, const char *hostPtr,
    const char *requestPtr) const noexcept
{
  std::int32_t status = -1;
  std::unique_ptr<std::uint8_t, decltype(&vPortFree)> \
  buffer_ptr(static_cast<std::uint8_t *>(pvPortCalloc(EmwNetworkStack::NETWORK_BUFFER_SIZE, sizeof(*buffer_ptr))),
             &vPortFree);

  (void) std::printf("%s: downloading file %s from \"%s\"\n", this->getName(), requestPtr, hostPtr);

  if (nullptr == buffer_ptr) {
    (void) std::printf("%s: memory allocation failed\n", this->getName());
  }
  try {
    std::uint32_t start_time_in_ms = 0;
    std::uint32_t local_start_time_in_ms = 0;
    AppConsoleDownload::HttpContext_t http_ctx;
    const std::uint32_t buffer_size = EmwNetworkStack::NETWORK_BUFFER_SIZE - 1;
    std::uint32_t ret_size = 0;
    std::uint64_t length = 0;
    std::uint32_t cpt_count = 0;

    *(buffer_ptr.get() + buffer_size) = '\0';
    (void) std::snprintf(reinterpret_cast<char *>(buffer_ptr.get()), buffer_size,
                         "GET %s HTTP/1.1\r\nHost:%s\r\nUser-Agent:EMW\r\n\r\n", requestPtr, hostPtr);

    lwip_send(socket,
              buffer_ptr.get(),
              std::strlen(reinterpret_cast<char *>(buffer_ptr.get())), 0);

    if (this->readResponse(socket, buffer_ptr.get(), buffer_size, ret_size) != 0) {
      throw std::runtime_error("no answer from HTTP server");
    }

    if (this->testResponse(http_ctx, buffer_ptr.get()) != 0) {
      *(buffer_ptr.get() + (buffer_size / 8)) = 0; /* Arbitrary set a end of string. */
      (void) std::printf("%s  <--- \"%s\"\n\n", this->getName(), buffer_ptr.get());
      throw std::runtime_error("incorrect HTTP server response");
    }

    length = http_ctx.contentLength;
    (void) std::printf("%s: file size %" PRIu32 " bytes\n", this->getName(), static_cast<std::uint32_t>(length));

    if (length > this->userDownloadLength) {
      (void) std::printf("%s: limiting transfer to first %" PRIu32 " bytes with report time of %" PRIu32 " ms\n",
                         this->getName(), this->userDownloadLength, AppConsoleDownload::REPORT_TIMEPERIOD_MS);
      length = this->userDownloadLength;
    }

    local_start_time_in_ms = start_time_in_ms = HAL_GetTick();

    while (length) {
      std::uint32_t elapsed_time_in_ms;
      std::uint32_t size_in_bytes = (buffer_size < length) ? buffer_size : static_cast<std::uint32_t>(length);

      const std::int32_t count = lwip_recv(socket, buffer_ptr.get(),
                                           size_in_bytes, 0);
      if (count == -1) {
        break;
      }
      if (count == 0) {
        status = 0;
        break;
      }

      (void) std::printf(".");

      cpt_count += static_cast<std::uint32_t>(count);

      elapsed_time_in_ms = HAL_GetTick() - local_start_time_in_ms;

      if (elapsed_time_in_ms > REPORT_TIMEPERIOD_MS) {
        const std::uint32_t ref_band_width = (8 * cpt_count) / elapsed_time_in_ms;
        (void) std::printf("\n%s: transfer %" PRIu32 " bytes, remain %" PRIu32 " bytes, bitrate %" PRIu32 " kbit/s\n",
                           this->getName(), cpt_count, static_cast<std::uint32_t>(length), ref_band_width);
        cpt_count = 0;
        local_start_time_in_ms = start_time_in_ms = HAL_GetTick();
      }

      if (length >= static_cast<std::uint64_t>(count)) {
        length -= static_cast<std::uint64_t>(count);
      }
      else {
        length = 0;
      }
    }
    lwip_shutdown(socket, SHUT_RD);
    lwip_close(socket);
    {
      const std::uint32_t duration_in_ms = HAL_GetTick() - start_time_in_ms;
      if (0 != duration_in_ms) {
        const std::uint32_t ref_band_width = (8 * cpt_count) / duration_in_ms;
        (void) std::printf("\n%s: transfer %" PRIu32 " bytes, duration %" PRIu32 " ms, bitrate %" PRIu32 " kbit/s\n",
                           this->getName(), cpt_count, duration_in_ms, ref_band_width);
      }
    }
  }
  catch (const std::runtime_error &error) {
    (void) std::printf("%s: %s\n", this->getName(), error.what());
    status = -1;
    lwip_shutdown(socket, SHUT_RD);
    lwip_close(socket);
  }
  return status;
}

std::int32_t AppConsoleDownload::readResponse(std::int32_t socket, unsigned char *headerPtr, std::uint32_t maximumSize,
    std::uint32_t &retSize) const noexcept
{
  char *string_ptr = reinterpret_cast<char *>(headerPtr);
  std::uint32_t header_size = maximumSize;
  std::uint32_t count = 0;

  retSize = 0U;

  while (header_size > 0) {
    std::int32_t ret = lwip_recv(socket, string_ptr, 1, 0);
    if (ret < 0) {
      return ret;
    }

    string_ptr++;
    header_size--;
    count++;
    if ((count > 4) && (std::strncmp(string_ptr - 4, "\r\n\r\n", 4) == 0)) {
      break;
    }
  }

  if (header_size == 0) {
    return -1;
  }

  retSize = count;

  return 0;
}

std::uint32_t AppConsoleDownload::serviceLines(unsigned char *bufferPtr, char *linePtrs[],
    std::uint32_t maxLines) const noexcept
{
  std::uint32_t index = 0;

  if (*bufferPtr != 0) {
    while ((index < maxLines) && (0 != *bufferPtr)) {
      linePtrs[index] = reinterpret_cast<char *>(bufferPtr);
      while ((0 != *bufferPtr) && (*bufferPtr != '\n')) {
        bufferPtr++;
      }
      index++;
      if (*bufferPtr) {
        bufferPtr++;
      }
    }
  }
  return index;
}

char *AppConsoleDownload::seekTo(char *stringPtr, char key) const noexcept
{
  while (*stringPtr && (*stringPtr != key)) {
    stringPtr++;
  }
  if (*stringPtr) {
    stringPtr++;
  }
  return stringPtr;
}

char *AppConsoleDownload::seekWhile(char *stringPtr, char key) const noexcept
{
  if (stringPtr) {
    while (*stringPtr && (*stringPtr == key)) {
      stringPtr++;
    }
  }
  return stringPtr;
}

char *AppConsoleDownload::seekWhileNot(char *stringPtr, char key) const noexcept
{
  if (stringPtr) {
    while (*stringPtr && (*stringPtr != key)) {
      stringPtr++;
    }
  }
  return stringPtr;
}

void AppConsoleDownload::splitHostRequest(const char *urlStringPtr, char * &hostStringPtr,
    const char *&requestStringPtr) const noexcept
{
  static char host_string[255];
  const std::size_t host_size = sizeof(host_string);
  const char *string_ptr = nullptr;

  std::memset(host_string, 0, sizeof(host_string));

  hostStringPtr = nullptr;
  requestStringPtr = nullptr;

  if (std::strncmp(urlStringPtr, "http://", std::strlen("http://")) == 0) {
    string_ptr = urlStringPtr + std::strlen("http://");
  }
  else {
    string_ptr = nullptr;
  }

  if (nullptr != string_ptr) {
    std::size_t i = 0U;
    while ((*string_ptr != 0) && (*string_ptr != '/') && (i < (host_size - 1))) {
      host_string[i++] = *string_ptr++;
    }
    host_string[i] = 0;
    hostStringPtr = host_string;
    requestStringPtr = string_ptr;
  }
}

std::int32_t AppConsoleDownload::testResponse(AppConsoleDownload::HttpContext_t &context,
    unsigned char *bufferPtr) const noexcept
{
  char *line_ptrs[10] = {nullptr};
  const std::uint32_t line_ptrs_count = this->serviceLines(bufferPtr, &line_ptrs[0],
                                        sizeof(line_ptrs) / sizeof(line_ptrs[0]));

  context.contentLength = -1;
  context.status = AppConsoleDownload::HTTP_RESPONSE_BAD_REQUEST;
  context.posFile = 0;

  if (line_ptrs_count < 1) {
    return -1;
  }
  {
    char *string_ptr = line_ptrs[0];
    string_ptr = this->seekWhileNot(string_ptr, ' ');
    string_ptr = this->seekWhile(string_ptr, ' ');
    std::sscanf(string_ptr, "%" PRIu32 "", &context.status);
  }
  for (std::uint32_t a = 0U ; a < line_ptrs_count ; a++) {
    if (std::strncmp(line_ptrs[a], "Content-Length", strlen("Content-Length")) == 0) {
      char *param_ptr = this->seekTo(line_ptrs[a], ':');

      param_ptr = this->seekWhile(param_ptr, ' ');
      if (NULL != param_ptr) {
        std::uint64_t length = 0;
        std::sscanf(param_ptr, "%" PRIu64 "", &length);
        /* Fill the length */
        context.contentLength = length;
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
