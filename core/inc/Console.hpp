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

class Cmd {
  public:
    constexpr Cmd(void) noexcept {};
  public:
    virtual ~Cmd(void) noexcept {};

  public:
    virtual std::int32_t execute(std::int32_t argc, char *argvPtrs[]) noexcept = 0;
  public:
    virtual const char *getComment(void) const noexcept = 0;
  public:
    virtual const char *getName(void) const noexcept = 0;
};

class Console final {
  public:
    explicit Console(const char *promptStringPtr, class Cmd *cmdPtrs[]);
  public:
    ~Console(void);
  public:
    void run(void) noexcept;

  private:
    void addInHistory(const char *cmdPtr) noexcept;
  private:
    char *duplicateString(const char *stringPtr) const noexcept;
  private:
    void freeHistory(void) noexcept;
  private:
    void getCommand(char s_cmd[]) noexcept;
  private:
    unsigned char getUserInputChar(void) noexcept;
  private:
    unsigned char getUserInputCharWithFiltering(void) noexcept;
  private:
    void getUserInputString(char s_cmd[]) noexcept;
  private:
    void help(void) noexcept;
  private:
    class Cmd **listMatchingCommands(const char *nameStringPtr, int32_t &matchCount) noexcept;
  private:
    class Cmd *matchCommand(const char *cmdNameStringPtr) noexcept;
  private:
    void printHistory(void) const noexcept;
  private:
    void resetInternals(void) noexcept;
  private:
    char *tokenize(char *stringPtr, const char *delimiterSting, char * &context) noexcept;
  private:
    void quit(void) noexcept;
  private:
    class Cmd *cmdsMatch[10U];
  private:
    class Cmd **cmdsPtr;
  private:
    const char *history[5U];
  private:
    std::uint32_t historyCount = 0;
  private:
    std::int32_t insertMode;
  private:
    const char *promptPtr;
  private:
    volatile bool quitFlag;
  private:
    static const std::size_t MAX_ARGS = 10U;
  private:
    static const std::size_t MAX_INPUT_LINE = 80U;
  private:
    static const unsigned char KEY_UP;
  private:
    static const unsigned char KEY_DOWN;
  private:
    static const unsigned char KEY_LEFT;
  private:
    static const unsigned char KEY_RIGHT;
  private:
    static const unsigned char KEY_BACKSPACE;
  private:
    static const unsigned char KEY_DELETE_MODE;
  private:
    static const unsigned char KEY_INSERT_MODE;
  private:
    static const unsigned char ESC;
  private:
    static const char TERM_LINE_ERASE_STRING[];
  private:
    static const char TERM_NO_LOCAL_ECHO_STRING[];
  private:
    static const char TERM_BLINKING_CURSOR_STRING[];
  private:
    static const char TERM_NO_BLINKING_CURSOR_STRING[];
  private:
    static const char TERM_CURSOR_LEFT_STRING[];
  private:
    static const char TERM_CURSOR_RIGHT_STRING[];
};
