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
#include "usart.h"
//#include "stm32u5xx_hal_uart.h"

#include <stdint.h>
#include <string.h>

int initializeStdinWithUart(void);
int stdin_reinitialize(void);

static char UartBuffer[60U];
static const uint32_t UartBufferWriteIdxMax = sizeof(UartBuffer) / sizeof(UartBuffer[0]);
static volatile uint32_t UartBufferWriteIdx = 0;
static uint32_t UartBufferReadIdx = 0;

static inline char CharFromUartBuffer(void);

int initializeStdinWithUart(void)
{
  memset(UartBuffer, 0, sizeof(UartBuffer));
  UartBufferWriteIdx = 0;
  UartBufferReadIdx = 0;

  (void) HAL_UART_Receive_IT(&hUart1, (uint8_t *)&UartBuffer[0], 1U);
  return 0;
}

int stdin_reinitialize(void)
{
  memset(UartBuffer, 0, sizeof(UartBuffer));
  UartBufferWriteIdx = 0;
  UartBufferReadIdx = 0;

  (void) HAL_UART_Receive_IT(&hUart1, (uint8_t *)&UartBuffer[0], 1U);
  return 0;
}

char io_getchar(void)
{
  const char ch = CharFromUartBuffer();
  return ch;
}

static inline char CharFromUartBuffer(void)
{
  char ch = 0;

  if (UartBufferWriteIdx != UartBufferReadIdx) {
    ch = UartBuffer[UartBufferReadIdx++];

    if (UartBufferReadIdx == UartBufferWriteIdxMax) {
      UartBufferReadIdx = 0;
    }
  }
  return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &hUart1) {
    UartBufferWriteIdx++;

    if (UartBufferWriteIdx == UartBufferWriteIdxMax) {
      UartBufferWriteIdx = 0;
    }
    HAL_UART_Receive_IT(huart, (uint8_t *)&UartBuffer[UartBufferWriteIdx], 1);
  }
}
