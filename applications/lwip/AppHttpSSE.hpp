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

#include <cstddef>
#include <cstdint>

class AppHttpSSE final {
  public:
    explicit AppHttpSSE(const struct netif &networkInterface): netif(networkInterface) {}
  public:
    ~AppHttpSSE() {}
  public:
    void initializeServer(std::uint32_t ipAddressIn, std::uint16_t port) noexcept;
  public:
    static std::uint32_t convertToOsTicks(std::uint32_t milliseconds) noexcept;

  private:
    class Client final {
      public:
        enum ClientStatus {
          eSTATUS_UNKNOWN = 0,
          eSTATUS_NEW = 1,
          eSTATUS_COMM = 2,
          eSTATUS_COMM_STREAM = 3,
          eSTATUS_END = 4
        };
      public:
        explicit Client(std::int32_t connectedSocket)
          : sock(connectedSocket), info{0}, err(0), status(AppHttpSSE::Client::eSTATUS_NEW), message(0) {}
      public:
        ~Client(void) {}
      public:
        std::int32_t sock;
      public:
        char info[32 + 1];
      private:
        char err;
      private:
        volatile Client::ClientStatus status;
      private:
        std::int32_t message;

      public:
        void close(void) noexcept;
      public:
        Client::ClientStatus getStatus(void) noexcept;
      public:
        void infos() const noexcept;
      public:
        std::int32_t sendEvent(const char *eventNamePtr, char *bufferStringPtr) noexcept;
      public:
        void sendError404Html(void) noexcept;
      public:
        void sendEventStream(void) noexcept;
      public:
        void sendIndexHtml(void) noexcept;
      public:
        void sendMemoryIconPng(void) noexcept;
      public:
        void sendThreadsIconPng(void) noexcept;
      public:
        void sendWiFiIconPng(void) noexcept;
      public:
        void setInfo(std::uint32_t address, std::uint16_t port) noexcept;

      private:
        std::int32_t send(const char *bufferPtr, std::size_t bufferSize) noexcept;
    };

  public:
    static char *chop(char *stringPtr) noexcept;
  public:
    static std::int32_t encodeHttpEvent(char *outBufferPtr, std::size_t outBufferLength,
                                        const char *eventNamePtr, char *messagePtr) noexcept;
  public:
    static char *tokenize(char *stringPtr, const char *delimiterString, char * &context) noexcept;

  private:
    static void listRemainingFreeHeap(char *bufferStringPtr, std::size_t bufferStringSize) noexcept;
  private:
    static void listTasks(char *bufferStringPtr, std::size_t bufferStringSize) noexcept;
  private:
    void doListenService(void) noexcept;
  private:
    static void doListenService(void *THIS) noexcept
    {
      (reinterpret_cast<AppHttpSSE *>(THIS))->doListenService();
    }
  private:
    static void doAcceptService(void *argPtr) noexcept;

  private:
    const struct netif &netif;
  private:
    static std::uint32_t serverIpAddrIn;
  private:
    static std::uint16_t serverPort;
  private:
    static const std::uint32_t MAX_NUM_OF_CLIENTS = 5U;
  private:
    static const std::uint32_t WEBSERVER_CHILD_TASK_PRIORITY = 16U;
  private:
    static const std::uint32_t WEBSERVER_CHILD_TASK_STACK_SIZE = 1500U;
  private:
    static const std::uint32_t WEBSERVER_LISTEN_BACKLOG = 25U;
  private:
    static const std::uint32_t WEBSERVER_TASK_PRIORITY = 16U;
  private:
    static const std::uint32_t WEBSERVER_TASK_STACK_SIZE = 576U;

};
