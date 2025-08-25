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
#pragma once

#include "Console.hpp"

class AppConsoleDownload final : public Cmd {
  public:
    explicit AppConsoleDownload(void) noexcept;
  public:
    virtual ~AppConsoleDownload(void) noexcept override;
  public:
    std::int32_t execute(std::int32_t argc, char *argvPtrs[]) noexcept override;
  public:
    const char *getComment(void) const noexcept override
    {
      return "http [-l<size>] [-6] <req> (default is http://test-debit.free.fr/image.iso)";
    }
  public:
    const char *getName(void) const noexcept override
    {
      return "http";
    }

  private:
    typedef struct HttpContext_s {
      constexpr HttpContext_s(void) : socket(0), status(-1), contentLength(0), posFile(0) {}
      std::int32_t socket;
      std::uint32_t status;
      std::uint64_t contentLength;
      std::uint64_t posFile;
    } HttpContext_t;

  private:
    std::int32_t doDownloadFile(std::int32_t socket, const char *hostPtr, const char *requestPtr) const noexcept;
  private:
    std::int32_t readResponse(std::int32_t sock, unsigned char *headerPtr, std::uint32_t maximumSize,
                              std::uint32_t &retSize) const noexcept;
  private:
    char *seekTo(char *stringPtr, char key) const noexcept;
  private:
    char *seekWhile(char *stringPtr, char key) const noexcept;
  private:
    char *seekWhileNot(char *stringPtr, char key) const noexcept;
  private:
    std::uint32_t serviceLines(unsigned char *bufferPtr, char *linePtrs[], uint32_t maxLines) const noexcept;
  private:
    void splitHostRequest(const char *urlStringPtr, char * &hostStringPtr, const char * &requestStringPtr) const noexcept;
  private:
    std::int32_t testResponse(AppConsoleDownload::HttpContext_t &context, unsigned char *bufferPtr) const noexcept;

  private:
    std::uint32_t userDownloadLength;
  private:
    static const char DOWNLOAD_HEADER[];
  private:
    static const std::uint32_t DOWNLOAD_LIMITED_SIZE = 140000;
  private:
    static const std::uint32_t HTTP_RESPONSE_OK = 200U;
  private:
    static const std::uint32_t HTTP_RESPONSE_MULTIPLE_CHOICES = 300U;
  private:
    static const std::uint32_t HTTP_RESPONSE_BAD_REQUEST = 400U;
  private:
    static const char HOST_REQUEST_STRING[255];
  private:
    static const std::uint32_t REPORT_TIMEPERIOD_MS = 10000U;
  private:
    static const std::uint16_t REMOTE_TCP_PORT = 80U;
  private:
    static const std::int32_t TIMEOUT_10S_DEFINED = 10000;
  private:
    static const std::uint32_t TRANSFER_SIZE = 2000U;
};
