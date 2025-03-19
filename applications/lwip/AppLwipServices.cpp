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
#include "AppLwipServices.hpp"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include <stdio.h>

int32_t AppLwipService::getHostByName(struct sockaddr *sAddrPtr, const char *nameStringPtr)
{
  int32_t ret = -1;

  if (AF_INET6 == sAddrPtr->sa_family) {
    if (sAddrPtr->sa_len >= sizeof(struct sockaddr_storage)) {
      struct addrinfo *hostinfo_ptr = NULL;
      const struct addrinfo hints = {AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, NULL, NULL, NULL};

      if (0 == lwip_getaddrinfo(nameStringPtr, NULL, &hints, &hostinfo_ptr)) {
#if LWIP_IPV6
        if (AF_INET6 == hostinfo_ptr->ai_family) {
          struct sockaddr_in6 *const s_addr_in6_ptr = reinterpret_cast<struct sockaddr_in6 *>(sAddrPtr);
          memcpy(&s_addr_in6_ptr->sin6_addr, &(reinterpret_cast<struct sockaddr_in6 *>(hostinfo_ptr->ai_addr))->sin6_addr,
                 sizeof(s_addr_in6_ptr->sin6_addr));
          ret = 0;
        }
#endif /* LWIP_IPV6 */
        lwip_freeaddrinfo(hostinfo_ptr);
      }
    }
  }
  else {
    if (sAddrPtr->sa_len >= sizeof(struct sockaddr_in)) {
      struct addrinfo *hostinfo_ptr = NULL;
      const struct addrinfo hints = {AI_PASSIVE, AF_INET, SOCK_DGRAM, 0, 0, NULL, NULL, NULL};

      if (0 == lwip_getaddrinfo(nameStringPtr, NULL, &hints, &hostinfo_ptr)) {
        if (hostinfo_ptr->ai_family == AF_INET) {
          const uint8_t len = sAddrPtr->sa_len;
          struct sockaddr_in *const s_addr_in_ptr = reinterpret_cast<struct sockaddr_in *>(sAddrPtr);

          s_addr_in_ptr->sin_len = len;
          s_addr_in_ptr->sin_family = AF_INET;

          memcpy(&s_addr_in_ptr->sin_addr, &(reinterpret_cast<struct sockaddr_in *>(hostinfo_ptr->ai_addr))->sin_addr,
                 sizeof(s_addr_in_ptr->sin_addr));
          ret = 0;
        }
        lwip_freeaddrinfo(hostinfo_ptr);
      }
    }
  }
  return ret;
}
