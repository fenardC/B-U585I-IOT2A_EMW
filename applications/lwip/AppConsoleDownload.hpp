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
    explicit AppConsoleDownload(void *contextPtr)
      : contextPtr(contextPtr), userDownloadLength(0) {}
  public:
    virtual ~AppConsoleDownload(void) override {}
  public:
    int32_t execute(int32_t argc, char *argvPtrs[]) override;
  public:
    const char *getComment(void) const override
    {
      return "http [-l<size>] [-6] <host>";
    }
  public:
    const char *getName(void) const override
    {
      return "http";
    }

  private:
    typedef struct HttpContext_s {
      constexpr HttpContext_s(void) : socket(0), status(-1), contentLength(0), posFile(0) {}
      int32_t socket;
      uint32_t status;
      uint64_t contentLength;
      uint64_t posFile;
    } HttpContext_t;

  private:
    int32_t checkResponse(AppConsoleDownload::HttpContext_t &context, unsigned char *bufferPtr) const;
  private:
    int32_t downloadFile(int32_t socket, const char *hostPtr, const char *requestPtr) const;
  private:
    int32_t readResponse(int32_t sock, unsigned char *headerPtr, uint32_t maximumSize, uint32_t *retSizePtr) const;
  private:
    char *seekTo(char *stringPtr, char key) const;
  private:
    char *seekWhile(char *stringPtr, char key) const;
  private:
    char *seekWhileNot(char *stringPtr, char key) const;
  private:
    uint32_t serviceLines(unsigned char *bufferPtr, char *linePtrs[], uint32_t maxLines) const;
  private:
    void splitHostRequest(const char *urlStringPtr, char * &hostStringPtr, const char * &requestStringPtr) const;

  private:
    void *contextPtr;
  private:
    uint32_t userDownloadLength;
  private:
    static const char DOWNLOAD_HEADER[];
  private:
    static const uint32_t DOWNLOAD_LIMITED_SIZE = 140000;
  private:
    static const uint32_t HTTP_RESPONSE_OK = 200U;
  private:
    static const uint32_t HTTP_RESPONSE_MULTIPLE_CHOICES = 300U;
  private:
    static const uint32_t HTTP_RESPONSE_BAD_REQUEST = 400U;
  private:
    static const char REMOTE_IP_ADDRESS_STRING[];
  private:
    static const uint32_t REPORT_TIMEPERIOD_MS = 10000U;
  private:
    static const uint16_t REMOTE_TCP_PORT = 80U;
  private:
    static const int32_t TIMEOUT_10S_DEFINED = 10000;
  private:
    static const uint32_t TRANSFER_SIZE = 2000U;
};
