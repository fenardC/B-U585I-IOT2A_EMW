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
    void initializeServer(uint32_t ipAddressIn, uint16_t port);
  public:
    static uint32_t convertToOsTicks(uint32_t milliseconds);

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
        explicit Client(int32_t connectedSocket)
          : sock(connectedSocket), info{0}, err(0), status(AppHttpSSE::Client::eSTATUS_NEW), message(0) {}
      public:
        ~Client(void) {}
      public:
        int32_t sock;
      public:
        char info[32 + 1];
      private:
        char err;
      private:
        volatile Client::ClientStatus status;
      private:
        int32_t message;

      public:
        int32_t close(void);
      public:
        Client::ClientStatus getStatus(void);
      public:
        void infos() const;
      public:
        int32_t sendEvent(const char *eventNamePtr, char *bufferStringPtr);
      public:
        void sendError404Html(void);
      public:
        void sendEventStream(void);
      public:
        void sendIndexHtml(void);
      public:
        void sendMemoryIconPng(void);
      public:
        void sendThreadsIconPng(void);
      public:
        void sendWiFiIconPng(void);
      public:
        void setInfo(uint32_t address, uint16_t port);

      private:
        int32_t send(const char *bufferPtr, size_t bufferSize);
    };

  public:
    static char *chop(char *stringPtr);
  public:
    static int32_t encodeHttpEvent(char *outBufferPtr, size_t outBufferLength, const char *eventNamePtr, char *messagePtr);
  public:
    static char *tokenize(char *stringPtr, const char *delimiterString, char * &context);

  private:
    static void listRemainingFreeHeap(char *bufferStringPtr, size_t bufferStringSize);
  private:
    static void listTasks(char *bufferStringPtr, size_t bufferStringSize);
  private:
    void doListenService(void);
  private:
    static void doListenService(void *THIS)
    {
      (reinterpret_cast<AppHttpSSE *>(THIS))->doListenService();
    }
  private:
    static void doAcceptService(void *argPtr);

  private:
    const struct netif &netif;
  private:
    static uint32_t serverIpAddrIn;
  private:
    static uint16_t serverPort;
  private:
    static const uint32_t MAX_NUM_OF_CLIENTS = 5U;
  private:
    static const uint32_t WEBSERVER_CHILD_TASK_PRIORITY = 16U;
  private:
    static const uint32_t WEBSERVER_CHILD_TASK_STACK_SIZE = 1500U;
  private:
    static const uint32_t WEBSERVER_LISTEN_BACKLOG = 25U;
  private:
    static const uint32_t WEBSERVER_TASK_PRIORITY = 16U;
  private:
    static const uint32_t WEBSERVER_TASK_STACK_SIZE = 576U;

};
