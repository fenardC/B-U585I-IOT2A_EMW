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
#include <stdint.h>
#include <stdbool.h>

class Cmd {
  public:
    Cmd(void) : contextPtr(nullptr) {};
  public:
    virtual ~Cmd(void) {};

  public:
    virtual int32_t execute(int32_t argc, char *argvPtrs[])
    {
      static_cast<void>(argc);
      static_cast<void>(argvPtrs);
      return 0;
    }
  public:
    virtual const char *getComment(void)
    {
      return "";
    }
  public:
    virtual const char *getName(void)
    {
      return "";
    }

  private:
    void *contextPtr;
};

class Console final {
  public:
    Console(const char *promptStringPtr, class Cmd *cmdPtrs[]);
  public:
    ~Console(void);
  private:
    void addInHistory(const char *cmdPtr);
  private:
    char *duplicateString(const char *stringPtr) const;
  private:
    void freeHistory(void);
  private:
    void getCommand(char s_cmd[]);
  private:
    unsigned char getUserInputChar(void);
  private:
    unsigned char getUserInputCharWithFiltering(void);
  private:
    void getUserInputString(char s_cmd[]);
  private:
    void help(void);
  private:
    class Cmd **listMatchingCommands(const char *nameStringPtr, int32_t &matchCountRef);
  private:
    class Cmd *matchCommand(const char *cmdNameStringPtr);
  private:
    void printHistory(void) const;
  private:
    void resetInternals(void);
  public:
    void run(void);
  private:
    void quit(void);
  private:
    class Cmd *cmdsMatch[10U];
  private:
    class Cmd **cmdsPtr;
  private:
    const char *history[5U];
  private:
    uint32_t historyCount = 0;
  private:
    int32_t insertMode;
  private:
    const char *promptPtr;
  private:
    volatile bool quitFlag;
  private:
    static const size_t MAX_ARGS = 10U;
  private:
    static const size_t MAX_INPUT_LINE = 80U;
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
