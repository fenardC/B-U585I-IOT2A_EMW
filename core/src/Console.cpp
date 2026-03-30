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
#include "Console.hpp"
#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS) */
#include "stdio_uart.h"
#include "stm32u5xx_hal.h"
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <cstdio>
#include <iostream>

//#define DEBUG_STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__));
#define DEBUG_STD_PRINTF(...)

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

#define ARRAY_COUNT_OF(array)  sizeof(array)/sizeof(array[0])

static const unsigned char KEY_UP = 128U;
static const unsigned char KEY_DOWN = 129U;
static const unsigned char KEY_LEFT = 130U;
static const unsigned char KEY_RIGHT = 131U;
static const unsigned char KEY_BACKSPACE = 8U;
static const unsigned char KEY_DELETE_MODE = 127U;
static const unsigned char KEY_INSERT_MODE = 126U;
static const unsigned char ESC = 0x1B;

static const char TERM_LINE_ERASE_STRING[] = {'\r', ESC, '[', '2', 'K', '\0'};
static const char TERM_NO_LOCAL_ECHO_STRING[] = {ESC, '[', '1', '2', 'h', '\0'};
static const char TERM_BLINKING_CURSOR_STRING[] = {ESC, '[', '?', '1', '2', 'h', '\0'};
static const char TERM_NO_BLINKING_CURSOR_STRING[] = {ESC, '[', '?', '1', '2', 'l', '\0'};
static const char TERM_CURSOR_LEFT_STRING[] = {ESC, '[', '1', 'D', '\0'};
static const char TERM_CURSOR_RIGHT_STRING[] = {ESC, '[', '1', 'C', '\0'};

static char *DuplicateString(const char *stringPtr);
static unsigned char GetUserInputChar(void);
static unsigned char GetUserInputCharWithFiltering(void);
static char *Tokenize(char *stringPtr, const char *delimiterStringPtr, char * &ptr);

Console::Console(const char *promptStringPtr, class Cmd *cmdPtrs[])
  : cmdsMatch{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
  , cmdsPtr(cmdPtrs)
  , history{nullptr, nullptr, nullptr, nullptr, nullptr}
  , historyCount(0U)
  , insertMode(1)
  , promptPtr(promptStringPtr)
  , quitFlag(false)
{
  DEBUG_STD_PRINTF("Console::Console()>\n")
  DEBUG_STD_PRINTF("Console::Console()<\n\n")
}

Console::~Console(void)
{
  DEBUG_STD_PRINTF("Console::~Console()>\n")
  DEBUG_STD_PRINTF("Console::~Console()<\n\n")
}

void Console::run(void) noexcept
{
  DEBUG_STD_PRINTF("Console::run()>\n")

  std::setbuf(stdout, nullptr);
  std::setbuf(stdin, nullptr);
  this->resetInternals();
  static_cast<void>(ReinitializeStdinWithUart());
  STD_PRINTF("%s\n", TERM_NO_LOCAL_ECHO_STRING);
  STD_PRINTF("##### Please enter one of the following command:\n\n");
  this->help();

  do {
    char s_cmd[Console::MAX_INPUT_LINE];

    std::memset(s_cmd, 0, sizeof(s_cmd));
    STD_PRINTF("%s", this->promptPtr);
    this->getCommand(s_cmd);

    if ('\0' != s_cmd[0]) {
      char s_cmd_copy[Console::MAX_INPUT_LINE];
      const char *args[Console::MAX_ARGS];
      std::int32_t i = 0;
      char *tokenize_worker_ptr = nullptr;

      std::memset(args, 0, sizeof(args));
      std::memset(s_cmd_copy, 0, sizeof(s_cmd_copy));
      std::strncpy(s_cmd_copy, s_cmd, sizeof(s_cmd_copy));
      args[i] = Tokenize(s_cmd, " \n\t", tokenize_worker_ptr);;
      {
        class Cmd *const cmd_ptr = this->matchCommand(args[i]);

        if (nullptr != cmd_ptr) {
          while (args[i]) {
            i++;
            args[i] = Tokenize(nullptr, " \n\t", tokenize_worker_ptr);
            if (i == static_cast<std::int32_t>(Console::MAX_ARGS - 1)) {
              break;
            }
          }
          cmd_ptr->execute(i, args);
          this->addInHistory(s_cmd_copy);
        }
      }
#if 0
      else {
        STD_PRINTF("%s: command not found\n", s_cmd_copy);
      }
#endif /* 0 */
    }
  }
  while (!this->quitFlag);
  this->quitFlag = false;
  this->freeHistory();
  DEBUG_STD_PRINTF("Console::run()<\n\n")
}

void Console::addInHistory(const char *cmdPtr) noexcept
{
  if (this->historyCount == 0U) {
    std::memset(this->history, 0, sizeof(this->history));
  }
  for (std::size_t i = 0U; i < ARRAY_COUNT_OF(this->history); i++) {
    if ((nullptr != this->history[i]) && (0 == std::strcmp(this->history[i], cmdPtr))) {
      if (i == 0U) {
        return;
      }
      {
        const char *const cmd_tmp_ptr = this->history[i];
        for (; i > 0U; i--) {
          history[i] = this->history[i - 1];
        }
        this->history[0] = cmd_tmp_ptr;
      }
      return;
    }
    if (nullptr == this->history[i]) {
      for (; i > 0U; i--) {
        this->history[i] = this->history[i - 1];
      }
      this->history[0] = DuplicateString(cmdPtr);
      if (this->historyCount < (ARRAY_COUNT_OF(this->history) - 1)) {
        this->historyCount++;
      }
      return;
    }
  }
  delete[](this->history[ARRAY_COUNT_OF(this->history) - 1]);
  for (std::size_t i = ARRAY_COUNT_OF(this->history) - 1; i > 0U; i--) {
    this->history[i] = history[i - 1];
  }
  this->history[0] = DuplicateString(cmdPtr);
}

void Console::freeHistory(void) noexcept
{
  for (std::size_t i = 0U; i < ARRAY_COUNT_OF(this->history); i++) {
    if (nullptr != this->history[i]) {
      delete[](this->history[i]);
      this->history[i] = nullptr;
    }
  }
  this->historyCount = 0U;
}

void Console::getCommand(char s_cmd[]) noexcept
{
  while (true) {
    this->getUserInputString(s_cmd);
    if ('!' == s_cmd[0]) {
      const std::int32_t number = std::atoi(&s_cmd[1]);

      if ((number >= 0) && (number < static_cast<std::int32_t>(this->historyCount))) {
        static_cast<void>(std::strncpy(s_cmd, this->history[number], Console::MAX_INPUT_LINE - 1));
      }
      else {
        s_cmd[0] = '\0';
      }
      return;
    }
    else {
      if (0 == std::strcmp(s_cmd, "help")) {
        this->help();
      }
      if (0 == std::strcmp(s_cmd, "history")) {
        this->printHistory();
      }
      if (0 == std::strcmp(s_cmd, "quit")) {
        this->quit();
        return;
      }
      else {
        return;
      }
    }
  }
}

void Console::help(void) noexcept
{
  class Cmd **cmds_ptr = this->cmdsPtr;

  while (nullptr != *cmds_ptr) {
    STD_PRINTF("%-10s  %s\n", (*cmds_ptr)->getName(), (*cmds_ptr)->getComment());
    cmds_ptr++;
  }
}

void Console::quit(void) noexcept
{
  DEBUG_STD_PRINTF("Console::quit()>\n")

  this->quitFlag = true;
  while (!this->quitFlag);
  DEBUG_STD_PRINTF("Console::quit()<\n\n")
}

class Cmd **Console::listMatchingCommands(const char *nameStringPtr, std::int32_t &matchCount) noexcept {
    matchCount = 0;
    if (nullptr != nameStringPtr)
    {
      const std::size_t name_length = std::strlen(nameStringPtr);

      if (0 == name_length) {
        return nullptr;
      }
      else {
        class Cmd **cmds_ptr = this->cmdsPtr;

        while (nullptr != *cmds_ptr) {
          const char *const cmd_name_ptr = (*cmds_ptr)->getName();

          if (std::strncmp(cmd_name_ptr, nameStringPtr, name_length) == 0) {
            if (std::strlen(cmd_name_ptr) == name_length) {
              matchCount = 1;
              this->cmdsMatch[0] = *cmds_ptr;
              return this->cmdsMatch;
            }
            this->cmdsMatch[matchCount] = *cmds_ptr;
            matchCount++;
          }
          cmds_ptr++;
        }
      }
    }
    if (matchCount > 0)
    {
      return this->cmdsMatch;
    }
    return nullptr;
}

class Cmd *Console::matchCommand(const char *cmdNameStringPtr) noexcept {
    std::int32_t match_count = 0;
    class Cmd **const cmd_ptr = this->listMatchingCommands(cmdNameStringPtr, match_count);

    if (1 == match_count)
    {
      return *cmd_ptr;
    }
    return nullptr;
}

void Console::getUserInputString(char s_cmd[]) noexcept
{
  std::uint32_t i = 0;
  std::uint32_t cursor_x = 0;
  unsigned char ch = 0;
  std::int32_t history_count = -1;

  s_cmd[0] = 0;
  do {
    ch = GetUserInputCharWithFiltering();

    if ('\0' == ch) {
#if defined(COMPILATION_WITH_FREERTOS)
      /* Be cooperative. */
      vTaskDelay(50);
#endif /* COMPILATION_WITH_FREERTOS) */
      continue;
    }
    if (KEY_DELETE_MODE == ch) {
      ch = KEY_BACKSPACE;
    }
    if (KEY_INSERT_MODE == ch) {
      this->insertMode = 1 - this->insertMode;
      if (this->insertMode) {
        STD_PRINTF("%s", TERM_BLINKING_CURSOR_STRING);
      }
      else {
        STD_PRINTF("%s", TERM_NO_BLINKING_CURSOR_STRING);
      }
    }
    else if (KEY_UP == ch) {
      if (history_count < static_cast<std::int32_t>(ARRAY_COUNT_OF(this->history) - 1)) {
        history_count++;
      }
      if (nullptr != this->history[history_count]) {
        STD_PRINTF("%s%s%s", TERM_LINE_ERASE_STRING, this->promptPtr, this->history[history_count]);
        i = static_cast<std::uint32_t>(std::strlen(this->history[history_count]));
        static_cast<void>(std::strncpy(s_cmd, this->history[history_count], Console::MAX_INPUT_LINE - 1));
        s_cmd[Console::MAX_INPUT_LINE - 1] = '\0';
        cursor_x = i;
      }
    }
    else if (KEY_DOWN == ch) {
      if (history_count > 0) {
        history_count--;
      }
      if (history_count < 0) {
        history_count = 0;
      }
      if (nullptr != this->history[history_count]) {
        STD_PRINTF("%s%s%s", TERM_LINE_ERASE_STRING, this->promptPtr, this->history[history_count]);
        i = static_cast<std::uint32_t>(std::strlen(this->history[history_count]));
        cursor_x = i;
        static_cast<void>(std::strncpy(s_cmd, this->history[history_count], Console::MAX_INPUT_LINE - 1));
        s_cmd[Console::MAX_INPUT_LINE - 1] = '\0';
      }
    }
    else if (KEY_LEFT == ch) {
      if (cursor_x > 0) {
        STD_PRINTF("%s", TERM_CURSOR_LEFT_STRING);
        cursor_x--;
      }
    }
    else if (KEY_RIGHT == ch) {
      if (cursor_x < i) {
        cursor_x++;
        STD_PRINTF("%s", TERM_CURSOR_RIGHT_STRING);
      }
    }
    else if (KEY_BACKSPACE == ch) {
      if (0 < cursor_x) {
        std::memmove(&s_cmd[cursor_x - 1], &s_cmd[cursor_x], 1 + i - cursor_x);
        s_cmd[i] = 0;
        STD_PRINTF("%s%s%s", TERM_LINE_ERASE_STRING, this->promptPtr, s_cmd);
        cursor_x--;
        i--;
        if (cursor_x < i) {
          STD_PRINTF("%c[%" PRIu32 "D", ESC, i - cursor_x);
        }
      }
    }
    else if ('\t' == ch) {
      class Cmd **match_list_ptr;
      const class Cmd *cmd_ptr;
      std::int32_t match_count = 0;

      s_cmd[i] = 0;
      match_list_ptr = this->listMatchingCommands(s_cmd, match_count);

      if (match_count == 1) {
        cmd_ptr = match_list_ptr[0];
        static_cast<void>(std::strncpy(s_cmd, cmd_ptr->getName(), Console::MAX_INPUT_LINE - 1));
        s_cmd[Console::MAX_INPUT_LINE - 1] = '\0';
        STD_PRINTF("%s%s%s", TERM_LINE_ERASE_STRING, this->promptPtr, s_cmd);
        i = cursor_x = static_cast<std::uint32_t>(std::strlen(s_cmd));
      }
      else if (1 < match_count) {
        STD_PRINTF("\n");
        for (std::int32_t j = 0; j < match_count ; j++) {
          cmd_ptr = match_list_ptr[j];
          STD_PRINTF("%s\n", cmd_ptr->getName());
        }
        STD_PRINTF("%s%s%s", TERM_LINE_ERASE_STRING, this->promptPtr, s_cmd);
        i = cursor_x = static_cast<std::uint32_t>(strlen(s_cmd));
      }
    }
    else {
      if (('\n' != ch) && ('\r' != ch)) {
        if (this->insertMode & (cursor_x != i)) {
          /* shift right the string from cursor */
          /* j MUST be signed. */
          for (std::int32_t j = static_cast<std::int32_t>(i); j >= static_cast<std::int32_t>(cursor_x); j--) {
            if ((j + 1) < static_cast<std::int32_t>(Console::MAX_INPUT_LINE)) {
              s_cmd[j + 1] = s_cmd[j];
            }
          }
          s_cmd[cursor_x++] = ch;
          i++;
          STD_PRINTF("%s%s%s", TERM_LINE_ERASE_STRING, this->promptPtr, s_cmd);
          if (cursor_x < i) {
            STD_PRINTF("%c[%" PRIu32 "D", ESC, i - cursor_x);
          }
        }
        else {
          s_cmd[cursor_x++] = ch;
          STD_PRINTF("%c", ch);
          if (cursor_x > i) {
            i = cursor_x;
          }
        }
      }
    }
    if (Console::MAX_INPUT_LINE == i) {
      i = static_cast<std::uint32_t>(Console::MAX_INPUT_LINE - 1);
      break;
    }
  }
  while ((ch != '\n') && (ch != '\r'));
  s_cmd[i] = 0;
  STD_PRINTF("\n");
}

void Console::printHistory(void) const noexcept
{
  for (std::uint32_t i = 0U; (i < this->historyCount) && (i < ARRAY_COUNT_OF(this->history)); i++) {
    STD_PRINTF("%" PRIu32 "\t%s\n", i, this->history[i]);
  }
}

void Console::resetInternals(void) noexcept
{
  DEBUG_STD_PRINTF("Console::resetInternals()>\n")

  this->quitFlag = false;
  this->insertMode = 1;
  for (std::size_t i = 0U; i < ARRAY_COUNT_OF(this->history); i++) {
    this->history[i] = nullptr;
  }
  this->historyCount = 0U;
  for (std::size_t i = 0U; i < ARRAY_COUNT_OF(this->cmdsMatch); i++) {
    this->cmdsMatch[i] = nullptr;
  }
  DEBUG_STD_PRINTF("Console::resetInternals()<\n\n")
}

static char *DuplicateString(const char *stringPtr)
{
  char *duplicated_string_ptr = nullptr;

  if (nullptr != stringPtr) {
    const std::size_t string_length = std::strlen(stringPtr) + 1;

    duplicated_string_ptr = new char[string_length];
    if (nullptr != duplicated_string_ptr) {
      std::memcpy(duplicated_string_ptr, stringPtr, string_length);
    }
  }
  return duplicated_string_ptr;
}

static unsigned char GetUserInputChar(void)
{
  unsigned char ret = '\0';
  const int c = std::cin.get();

  if (0 < c) {
    ret = static_cast<unsigned char>(c);
  }
  return ret;
}

static unsigned char GetUserInputCharWithFiltering(void)
{
  const unsigned char one_char = GetUserInputChar();

  if (ESC == one_char) {
    const unsigned char first = GetUserInputChar();
    const unsigned char second = GetUserInputChar();

    if ((first == '[') && (second == 50)) {
      /* discard 0x7E */
      static_cast<void>(GetUserInputChar());
      return KEY_INSERT_MODE;
    }
    if ((first == 79) && (second == 83)) {
      return '-';
    }
    if ((first == 79) && (second == 82)) {
      return '*';
    }
    if ((first == 79) && (second == 81)) {
      return '/';
    }
    if ((first == '[') && (second == 65)) {
      return KEY_UP;
    }
    if ((first == '[') && (second == 66)) {
      return KEY_DOWN;
    }
    if ((first == '[') && (second == 67)) {
      return KEY_RIGHT;
    }
    if ((first == '[') && (second == 68)) {
      return KEY_LEFT;
    }
  }
  return one_char;
}

static char *Tokenize(char *stringPtr, const char *delimiterStringPtr, char * &ptr)
{
  char *token_begin_ptr;

  if (nullptr != stringPtr) {
    ptr = stringPtr;
  }
  ptr += std::strspn(ptr, delimiterStringPtr);
  if ('\0' == *ptr) {
    return nullptr;
  }
  {
    token_begin_ptr = ptr;

    ptr += std::strcspn(ptr, delimiterStringPtr);
    if ('\0' != *ptr) {
      *ptr++ = '\0';
    }
  }
  return token_begin_ptr;
}
