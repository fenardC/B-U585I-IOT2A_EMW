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
#include "EmwAddress.hpp"
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

int32_t EmwAddress::asciiToNetwork(const char *addressStringPtr, EmwAddress::IpAddr_t &address)
{
  uint32_t val = 0U;
  uint32_t base;
  const char *char_ptr = addressStringPtr;
  uint8_t byte_in_string;
  uint32_t parts[4] = {0U, 0U, 0U, 0U};
  const uint32_t *const parts_end_ptr = &parts[3];
  uint32_t *parts_ptr = parts;
  int32_t status = 1;
  bool done = false;

  byte_in_string = (uint8_t)(*char_ptr);
  for (;;) {
    /**
      * Collect number up to ``.''.
      * Values are specified as for C:
      * 0x=hex, 0=octal, 1-9=decimal.
      */
    if (done == true) {
      break;
    }
    if (!EmwAddress::isDigit(byte_in_string)) {
      status = 0;
      done = true;
    }
    else {
      val = 0U;
      base = 10U;
      if (byte_in_string == (uint8_t)'0') {
        ++char_ptr;
        byte_in_string = (uint8_t)(*char_ptr);
        if ((byte_in_string == (uint8_t)'x') || (byte_in_string == (uint8_t)'X')) {
          base = 16U;
          ++char_ptr;
          byte_in_string = (uint8_t)(*char_ptr);
        }
        else {
          base = 8U;
        }
      }
      for (;;) {
        if (EmwAddress::isDigit(byte_in_string)) {
          val = (val * base) + (uint32_t)byte_in_string - (uint32_t)'0';
          ++char_ptr;
          byte_in_string = (uint8_t)(*char_ptr);
        }
        else if ((base == 16U) && EmwAddress::isHexaDigit(byte_in_string)) {
          val = (val << 4) \
                | (((uint32_t)byte_in_string + 10U \
                    - ((EmwAddress::isLower(byte_in_string)) ? (uint32_t)'a' : (uint32_t)'A')));
          ++char_ptr;
          byte_in_string = (uint8_t)(*char_ptr);
        }
        else {
          break;
        }
      }
      if (byte_in_string == (uint8_t)'.') {
        /**
          * Internet format:
          *  a.b.c.d
          *  a.b.c   (with c treated as 16 bits)
          *  a.b (with b treated as 24 bits)
          */
        if (parts_ptr >= parts_end_ptr) {
          status = 0;
          done = true;
        }
        else {
          *parts_ptr = val;
          parts_ptr++;
          ++char_ptr;
          byte_in_string = (uint8_t)(*char_ptr);
        }
      }
      else {
        done = true;
      }
    }
  }
  if ((byte_in_string != (uint8_t)'\0') && (EmwAddress::isSpace((byte_in_string)) == false)) {
    status = 0;
  }
  else {
    switch ((ptrdiff_t)(parts_ptr - parts) + 1) {
      case 0: {
          status = 0; /* initial non digit */
          break;
        }
      case 1: { /* a -- 32 bits */
          break;
        }
      case 2: { /* a.b -- 8.24 bits */
          if (val > 0xffffffUL) {
            status = 0;
          }
          val |= parts[0] << 24;
          break;
        }
      case 3: { /* a.b.c -- 8.8.16 bits */
          if (val > 0xffffU) {
            status = 0;
            break;
          }
          val |= (parts[0] << 24) | (parts[1] << 16);
          break;
        }
      case 4: { /* a.b.c.d -- 8.8.8.8 bits */
          if (val > 0xffU) {
            status = 0;
            break;
          }
          val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
          break;
        }
      default: {
          status = 0;
          break;
        }
    }
  }
  if (status == 1) {
    address.addr = EmwAddress::hostToNetworkLong(val);
  }
  return status;
}

int32_t EmwAddress::asciiToNetwork(const char *addressStringPtr, EmwAddress::Ip6Addr_t &address)
{
  int32_t status = 1;
  uint32_t addr_index;
  uint32_t zero_blocks;
  uint32_t current_block_index;
  uint32_t current_block_value;
  int check_ipv4_mapped = 0;

  /**
    * Count the number of colons, to count the number of blocks in a "::" sequence
    * zero_blocks may be 1 even if there are no :: sequences
    */
  zero_blocks = 8;
  for (auto char_ptr = addressStringPtr; *char_ptr != 0; char_ptr++) {
    const auto char_value = *char_ptr;
    if (char_value == ':') {
      zero_blocks--;
    }
    else if (char_value == '.') {
      if ((zero_blocks == 5) || (zero_blocks == 2)) {
        check_ipv4_mapped = 1;
        /* last block could be the start of an IPv4 address */
        zero_blocks--;
      }
      else {
        /* invalid format */
        return 0;
      }
      break;
    }
    else if (!EmwAddress::isDigit(char_value)) {
      break;
    }
  }
  /* parse each block */
  addr_index = 0;
  current_block_index = 0;
  current_block_value = 0;
  for (auto char_ptr = addressStringPtr; *char_ptr != 0; char_ptr++) {
    if (*char_ptr == ':') {
      if (current_block_index & 0x1) {
        address.addr[addr_index++] |= current_block_value;
      }
      else {
        address.addr[addr_index] = current_block_value << 16;
      }
      current_block_index++;
      if (check_ipv4_mapped) {
        if (current_block_index == 6) {
          EmwAddress::IpAddr_t ip4;
          int ret = EmwAddress::asciiToNetwork(char_ptr + 1, ip4);
          if (ret) {
            address.addr[3] = EmwAddress::hostToNetworkLong(ip4.addr);
            current_block_index++;
            goto fix_byte_order_and_return;
          }
        }
      }
      current_block_value = 0;
      if (current_block_index > 7) {
        /* address too long! */
        return 0;
      }
      if (char_ptr[1] == ':') {
        if (char_ptr[2] == ':') {
          /* invalid format: three successive colons */
          return 0;
        }
        char_ptr++;
        /* "::" found, set zeros */
        while (zero_blocks > 0) {
          zero_blocks--;
          if (current_block_index & 0x1) {
            addr_index++;
          }
          else {
            address.addr[addr_index] = 0;
          }
          current_block_index++;
          if (current_block_index > 7) {
            /* address too long! */
            return 0;
          }
        }
      }
    }
    else if (EmwAddress::isHexaDigit(*char_ptr)) {
      /* add current digit */
      current_block_value = (current_block_value << 4) +
                            (EmwAddress::isDigit(*char_ptr) ? (uint32_t)(*char_ptr - '0') :
                             (uint32_t)(10 + (EmwAddress::isLower(*char_ptr) ? *char_ptr - 'a' : *char_ptr - 'A')));
    }
    else {
      /* unexpected digit, space? CRLF? */
      break;
    }
  }
  if (current_block_index & 0x1) {
    address.addr[addr_index++] |= current_block_value;
  }
  else {
    address.addr[addr_index] = current_block_value << 16;
  }

fix_byte_order_and_return:
  /* convert to network byte order. */
  for (addr_index = 0; addr_index < 4U; addr_index++) {
    address.addr[addr_index] = EmwAddress::hostToNetworkLong(address.addr[addr_index]);
  }
  address.zone = 0U;
  if (current_block_index != 7) {
    return 0;
  }
  return status;
}

char *EmwAddress::networkToAscii(const EmwAddress::IpAddr_t &address, char buffer[], const size_t bufferSize)
{
  uint32_t length = 0U;
  const uint8_t ip_addr_bytes[4] = {
    (uint8_t)(address.addr & 0x000000FFU),
    (uint8_t)((address.addr & 0x0000FF00U) >> 8),
    (uint8_t)((address.addr & 0x00FF0000U) >> 16),
    (uint8_t)((address.addr & 0xFF000000U) >> 24)
  };
  for (uint8_t n = 0U; n < 4U; n++) {
    char inv[3] = {0, 0, 0};
    uint8_t i = 0U;
    uint8_t val = ip_addr_bytes[n];
    do {
      const uint8_t rem = val % 10U;
      val /= 10U;
      inv[i] = '0' + rem;
      i++;
    }
    while ((val != 0U) && (i < (sizeof(inv) / sizeof(inv[0]))));

    while (i != 0U) {
      i--;
      if (length < bufferSize) {
        buffer[length] = inv[i];
        length++;
      }
    }
    if ((n < 3U) && (length < bufferSize)) {
      buffer[length] = '.';
      length++;
    }
  }
  if (length < bufferSize) {
    buffer[length] = '\0';
  }
  else {
    buffer[bufferSize - 1U] = '\0';
  }
  return buffer;
}

char *EmwAddress::networkToAscii(const EmwAddress::Ip6Addr_t &address, char buffer[], const size_t bufferSize)
{
  uint32_t current_block_index;
  uint32_t next_block_value;
  uint8_t zero_flag;
  uint8_t empty_block_flag = 0U;
  uint32_t length = 0U;
  for (current_block_index = 0U; current_block_index < 8U; current_block_index++) {
    uint32_t current_block_value = EmwAddress::hostToNetworkLong(address.addr[current_block_index >> 1]);
    if ((current_block_index & 0x1) == 0U) {
      current_block_value = current_block_value >> 16;
    }
    current_block_value &= 0x0000FFFF;
    if (current_block_value == 0U) {
      if ((current_block_index == 7) && (1 == empty_block_flag)) {
        buffer[length++] = ':';
        if (length >= bufferSize) {
          return nullptr;
        }
        break;
      }
      if (0 == empty_block_flag) {
        next_block_value = EmwAddress::hostToNetworkLong(address.addr[(current_block_index + 1) >> 1]);
        if ((current_block_index & 0x1) == 0x01) {
          next_block_value = next_block_value >> 16;
        }
        next_block_value &= 0x0000FFFF;
        if (next_block_value == 0U) {
          empty_block_flag = 1;
          buffer[length++] = ':';
          if (length >= bufferSize) {
            return nullptr;
          }
          continue;
        }
      }
      else if (empty_block_flag == 1) {
        continue;
      }
    }
    else if (empty_block_flag == 1) {
      empty_block_flag = 2U;
    }
    if (current_block_index > 0U) {
      buffer[length++] = ':';
      if (length >= bufferSize) {
        return nullptr;
      }
    }
    if ((current_block_value & 0xF000) == 0) {
      zero_flag = 1;
    }
    else {
      buffer[length++] = EmwAddress::toDecimalChar(((current_block_value & 0xF000) >> 12));
      zero_flag = 0U;
      if (length >= bufferSize) {
        return nullptr;
      }
    }
    if (((current_block_value & 0x0F00) == 0) && (zero_flag)) {
    }
    else {
      buffer[length++] = EmwAddress::toDecimalChar(((current_block_value & 0x0F00) >> 8));
      zero_flag = 0U;
      if (length >= bufferSize) {
        return nullptr;
      }
    }
    if (((current_block_value & 0xF0) == 0) && (zero_flag)) {
    }
    else {
      buffer[length++] = EmwAddress::toDecimalChar(((current_block_value & 0x00F0) >> 4));
      zero_flag = 0U;
      if (length >= bufferSize) {
        return nullptr;
      }
    }
    buffer[length++] = EmwAddress::toDecimalChar((current_block_value & 0x000F));
    if (length >= bufferSize) {
      return nullptr;
    }
  }
  if (length < bufferSize) {
    buffer[length] = '\0';
  }
  else {
    buffer[bufferSize - 1U] = '\0';
  }
  return buffer;
}

uint32_t EmwAddress::hostToNetworkLong(uint32_t hostLong)
{
  return ((((uint32_t)(hostLong) & 0xff000000U) >> 24) | \
          (((uint32_t)(hostLong) & 0x00ff0000U) >> 8) | \
          (((uint32_t)(hostLong) & 0x0000ff00U) << 8) | \
          (((uint32_t)(hostLong) & 0x000000ffU) << 24));
}

bool EmwAddress::isDigit(uint8_t byte)
{
  return (byte >= (uint8_t)'0') && (byte <= (uint8_t)'9');
}

bool EmwAddress::isHexaDigit(uint8_t byte)
{
  return ((byte >= (uint8_t)'0') && (byte <= (uint8_t)'9'))
         || ((byte >= (uint8_t)'a') && (byte <= (uint8_t)'z'))
         || ((byte >= (uint8_t)'A') && (byte <= (uint8_t)'F'));
}

bool EmwAddress::isLower(uint8_t byte)
{
  return (byte >= (uint8_t)'a') && (byte <= (uint8_t)'z');
}

bool EmwAddress::isSpace(uint8_t byte)
{
  return (byte == (uint8_t)' ') || (byte == (uint8_t)'\f')
         || (byte == (uint8_t)'\n') || (byte == (uint8_t)'\r')
         || (byte == (uint8_t)'\t') || (byte == (uint8_t)'\v');
}

char EmwAddress::toDecimalChar(uint8_t halfByte)
{
  return ((char)((halfByte) < 10 ? '0' + (halfByte) : 'A' + (halfByte) - 10));
}
