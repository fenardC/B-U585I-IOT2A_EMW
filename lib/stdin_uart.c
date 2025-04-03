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
#include "stdio_uart.h"
#include "usart.h"
//#include "stm32u5xx_hal_uart.h"
#include <stdint.h>
#include <string.h>

static char UartBuffer[0x40];
static volatile uint32_t UartBufferReadIdx = 0;
static volatile uint32_t UartBufferWriteIdx = 0;

static inline char CharFromUartBuffer(void);

int InitializeStdinWithUart(void)
{
  memset(UartBuffer, 0, sizeof(UartBuffer));
  UartBufferWriteIdx = 0;
  UartBufferReadIdx = 0;

  (void) HAL_UART_Receive_IT(&hUart1, (uint8_t *)&UartBuffer[0], 1U);
  return 0;
}

int ReinitializeStdinWithUart(void)
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
    ch = UartBuffer[UartBufferReadIdx];
    UartBufferReadIdx++;
    UartBufferReadIdx &= 0x0000003F;
  }
  return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uartPtr)
{
  if (uartPtr == &hUart1) {
    UartBufferWriteIdx++;
    UartBufferWriteIdx &= 0x0000003F;
    HAL_UART_Receive_IT(uartPtr, (uint8_t *)&UartBuffer[UartBufferWriteIdx], 1);
  }
}
