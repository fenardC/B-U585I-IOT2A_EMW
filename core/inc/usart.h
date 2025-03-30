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
#ifndef USART_H
#define USART_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "stm32u5xx_hal.h"
//#include "stm32u5xx_hal_uart.h"

extern UART_HandleTypeDef hUart1;

void InitializeUSART1(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* USART_H */
