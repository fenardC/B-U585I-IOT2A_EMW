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

int InitializeStdoutWithUart(void)
{
  return 0;
}

int io_putchar(char ch)
{
  HAL_UART_Transmit(&hUart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
