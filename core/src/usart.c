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
//#include "stm32u5xx_hal_cortex.h"
//#include "stm32u5xx_hal_gpio.h"
//#include "stm32u5xx_hal_rcc.h"
//#include "stm32u5xx_hal_uart_ex.h"
#include "main.hpp"

UART_HandleTypeDef hUart1;

void InitializeUSART1(void)
{
  hUart1.Instance = USART1;
  hUart1.Init.BaudRate = 115200;
  hUart1.Init.WordLength = UART_WORDLENGTH_8B;
  hUart1.Init.StopBits = UART_STOPBITS_1;
  hUart1.Init.Parity = UART_PARITY_NONE;
  hUart1.Init.Mode = UART_MODE_TX_RX;
  hUart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hUart1.Init.OverSampling = UART_OVERSAMPLING_16;
  hUart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hUart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hUart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&hUart1) != HAL_OK) {
    ErrorHandler();
  }

  if (HAL_UARTEx_SetTxFifoThreshold(&hUart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
    ErrorHandler();
  }

  if (HAL_UARTEx_SetRxFifoThreshold(&hUart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
    ErrorHandler();
  }

  if (HAL_UARTEx_DisableFifoMode(&hUart1) != HAL_OK) {
    ErrorHandler();
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartPtr)
{
  GPIO_InitTypeDef configuration = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (uartPtr->Instance == USART1) {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
      ErrorHandler();
    }

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    configuration.Pin = GPIO_PIN_10 | GPIO_PIN_9;
    configuration.Mode = GPIO_MODE_AF_PP;
    configuration.Pull = GPIO_NOPULL;
    configuration.Speed = GPIO_SPEED_FREQ_LOW;
    configuration.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &configuration);
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartPtr)
{
  if (uartPtr->Instance == USART1) {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10 | GPIO_PIN_9);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  }
}
