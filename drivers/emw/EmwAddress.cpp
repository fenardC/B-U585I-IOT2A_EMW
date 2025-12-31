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
#include "EmwAddress.hpp"
#include <cstddef>
#include <cstdlib>

static bool IsDigit(std::uint8_t byte);
static bool IsHexaDigit(std::uint8_t byte);
static bool IsLower(std::uint8_t byte);
static bool IsSpace(std::uint8_t byte);
static char ToDecimalChar(std::uint8_t halfByte);


int32_t EmwAddress::AsciiToNetwork(const char *addressStringPtr, EmwAddress::IpAddr_t &address) noexcept
{
  std::uint32_t val = 0U;
  std::uint32_t base;
  const char *char_ptr = addressStringPtr;
  std::uint8_t byte_in_string;
  std::uint32_t parts[4] = {0U, 0U, 0U, 0U};
  const std::uint32_t *const parts_end_ptr = &parts[3];
  std::uint32_t *parts_ptr = parts;
  std::int32_t status = 1;
  bool done = false;

  byte_in_string = static_cast<std::uint8_t>(*char_ptr);
  for (;;) {
    /**
      * Collect number up to ``.''.
      * Values are specified as for C:
      * 0x=hex, 0=octal, 1-9=decimal.
      */
    if (done == true) {
      break;
    }
    if (!IsDigit(byte_in_string)) {
      status = 0;
      done = true;
    }
    else {
      val = 0U;
      base = 10U;
      if (byte_in_string == '0') {
        ++char_ptr;
        byte_in_string = static_cast<std::uint8_t>(*char_ptr);
        if ((byte_in_string == 'x') || (byte_in_string == 'X')) {
          base = 16U;
          ++char_ptr;
          byte_in_string = static_cast<std::uint8_t>(*char_ptr);
        }
        else {
          base = 8U;
        }
      }
      for (;;) {
        if (IsDigit(byte_in_string)) {
          val = (val * base) + byte_in_string - '0';
          ++char_ptr;
          byte_in_string = static_cast<std::uint8_t>(*char_ptr);
        }
        else if ((base == 16U) && IsHexaDigit(byte_in_string)) {
          val \
            = (val << 4) | ((byte_in_string + 10U - ((IsLower(byte_in_string)) ? 'a' : 'A')));
          ++char_ptr;
          byte_in_string = static_cast<std::uint8_t>(*char_ptr);
        }
        else {
          break;
        }
      }
      if (byte_in_string == '.') {
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
          byte_in_string = static_cast<std::uint8_t>(*char_ptr);
        }
      }
      else {
        done = true;
      }
    }
  }
  if ((byte_in_string != '\0') && (IsSpace((byte_in_string)) == false)) {
    status = 0;
  }
  else {
    switch (static_cast<ptrdiff_t>(parts_ptr - parts) + 1) {
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
    address.addr = EmwAddress::HostToNetworkLong(val);
  }
  return status;
}

std::int32_t EmwAddress::AsciiToNetwork(const char *addressStringPtr, EmwAddress::Ip6Addr_t &address) noexcept
{
  std::int32_t status = 1;
  std::uint32_t addr_index;
  std::uint32_t zero_blocks;
  std::uint32_t current_block_index;
  std::uint32_t current_block_value;
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
    else if (!IsDigit(char_value)) {
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
          int ret = EmwAddress::AsciiToNetwork(char_ptr + 1, ip4);
          if (ret) {
            address.addr[3] = EmwAddress::HostToNetworkLong(ip4.addr);
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
    else if (IsHexaDigit(*char_ptr)) {
      /* add current digit */
      current_block_value = (current_block_value << 4) +
                            (IsDigit(*char_ptr) ? (*char_ptr - '0') :
                             (10 + (IsLower(*char_ptr) ? *char_ptr - 'a' : *char_ptr - 'A')));
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
    address.addr[addr_index] = EmwAddress::HostToNetworkLong(address.addr[addr_index]);
  }
  address.zone = 0U;
  if (current_block_index != 7) {
    return 0;
  }
  return status;
}

char *EmwAddress::NetworkToAscii(const EmwAddress::IpAddr_t &address, char (&addressString)[16]) noexcept
{
  const std::size_t address_string_size = 16;
  const std::uint8_t ip_addr_bytes[4] = {
    static_cast<std::uint8_t>(address.addr & 0x000000FFU),
    static_cast<std::uint8_t>((address.addr & 0x0000FF00U) >> 8),
    static_cast<std::uint8_t>((address.addr & 0x00FF0000U) >> 16),
    static_cast<std::uint8_t>((address.addr & 0xFF000000U) >> 24)
  };
  std::size_t pos = 0U;

  for (std::uint8_t i = 0U; i < 4U; ++i) {
    const std::uint8_t working_byte = ip_addr_bytes[i];
    const std::uint8_t hundreds = working_byte / 100U;
    const std::uint8_t tens = (working_byte / 10U) % 10U;
    const std::uint8_t ones = working_byte % 10U;
    const std::uint8_t mask2 = (hundreds != 0U);
    const std::uint8_t mask1 = mask2 | (tens != 0U);

    if ((mask2 != 0U) && (pos < (address_string_size - 1U))) {
      addressString[pos] = static_cast<char>('0' + hundreds);
      pos++;
    }
    if ((mask1 != 0U) && (pos < (address_string_size - 1U))) {
      addressString[pos] = static_cast<char>('0' + tens);
      pos++;
    }
    if (pos < (address_string_size - 1U)) {
      addressString[pos] = static_cast<char>('0' + ones);
      pos++;
    }
    if ((i < 3U) && (pos < (address_string_size - 1U))) {
      addressString[pos] = '.';
      pos++;
    }
  }

  addressString[(pos < address_string_size) ? pos : (address_string_size - 1U)] = '\0';
  return addressString;
}

char *EmwAddress::NetworkToAscii(const EmwAddress::Ip6Addr_t &address, char (&addressString)[40]) noexcept
{
  const std::size_t address_string_size = 40;
  std::uint32_t current_block_index;
  std::uint32_t next_block_value;
  std::uint8_t zero_flag;
  std::uint8_t empty_block_flag = 0U;
  std::uint32_t length = 0U;

  for (current_block_index = 0U; current_block_index < 8U; current_block_index++) {
    std::uint32_t current_block_value = EmwAddress::HostToNetworkLong(address.addr[current_block_index >> 1]);
    if ((current_block_index & 0x1) == 0U) {
      current_block_value = current_block_value >> 16;
    }
    current_block_value &= 0x0000FFFF;
    if (current_block_value == 0U) {
      if ((current_block_index == 7) && (1 == empty_block_flag)) {
        addressString[length++] = ':';
        if (length >= address_string_size) {
          return nullptr;
        }
        break;
      }
      if (0 == empty_block_flag) {
        next_block_value = EmwAddress::HostToNetworkLong(address.addr[(current_block_index + 1) >> 1]);
        if ((current_block_index & 0x1) == 0x01) {
          next_block_value = next_block_value >> 16;
        }
        next_block_value &= 0x0000FFFF;
        if (next_block_value == 0U) {
          empty_block_flag = 1;
          addressString[length++] = ':';
          if (length >= address_string_size) {
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
      addressString[length++] = ':';
      if (length >= address_string_size) {
        return nullptr;
      }
    }
    if ((current_block_value & 0xF000) == 0) {
      zero_flag = 1;
    }
    else {
      addressString[length++] = ToDecimalChar(((current_block_value & 0xF000) >> 12));
      zero_flag = 0U;
      if (length >= address_string_size) {
        return nullptr;
      }
    }
    if (((current_block_value & 0x0F00) == 0) && (zero_flag)) {
    }
    else {
      addressString[length++] = ToDecimalChar(((current_block_value & 0x0F00) >> 8));
      zero_flag = 0U;
      if (length >= address_string_size) {
        return nullptr;
      }
    }
    if (((current_block_value & 0xF0) == 0) && (zero_flag)) {
    }
    else {
      addressString[length++] = ToDecimalChar(((current_block_value & 0x00F0) >> 4));
      zero_flag = 0U;
      if (length >= address_string_size) {
        return nullptr;
      }
    }
    addressString[length++] = ToDecimalChar((current_block_value & 0x000F));
    if (length >= address_string_size) {
      return nullptr;
    }
  }
  if (length < address_string_size) {
    addressString[length] = '\0';
  }
  else {
    addressString[address_string_size - 1U] = '\0';
  }
  return addressString;
}

uint32_t EmwAddress::HostToNetworkLong(std::uint32_t hostLong)
{
  return (((static_cast<std::uint32_t>(hostLong) & 0xFF000000U) >> 24) | \
          ((static_cast<std::uint32_t>(hostLong) & 0x00FF0000U) >> 8) | \
          ((static_cast<std::uint32_t>(hostLong) & 0x0000FF00U) << 8) | \
          ((static_cast<std::uint32_t>(hostLong) & 0x000000FFU) << 24));
}

static bool IsDigit(std::uint8_t byte)
{
  return (byte >= '0') && (byte <= '9');
}

static bool IsHexaDigit(std::uint8_t byte)
{
  return ((byte >= '0') && (byte <= '9'))
         || ((byte >= 'a') && (byte <= 'z'))
         || ((byte >= 'A') && (byte <= 'F'));
}

static bool IsLower(std::uint8_t byte)
{
  return (byte >= 'a') && (byte <= 'z');
}

static bool IsSpace(std::uint8_t byte)
{
  return (byte == ' ') || (byte == '\f')
         || (byte == '\n') || (byte == '\r')
         || (byte == '\t') || (byte == '\v');
}

static char ToDecimalChar(std::uint8_t halfByte)
{
  return (halfByte < 10) ? ('0' + halfByte) : ('A' + halfByte - 10);
}
