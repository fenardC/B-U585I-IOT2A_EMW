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
#include "stm32u5xx_it.h"
#include "gpdma.h"
#include "gpio.h"
#include "icache.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "emw_conf.hpp"

extern TIM_HandleTypeDef hTim6;

void NMI_Handler(void)
{
  while (1) {
  }
}

void HardFault_Handler(void)
{
  while (1) {
  }
}

void MemManage_Handler(void)
{
  while (1) {
  }
}

void BusFault_Handler(void)
{
  while (1) {
  }
}

void UsageFault_Handler(void)
{
  while (1) {
  }
}

void DebugMon_Handler(void)
{
}

void EXTI14_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(EMW_NOTIFY_Pin);
}

void EXTI15_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(EMW_FLOW_Pin);
}

void GPDMA1_Channel4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hGpdma1Channel4);
}

void GPDMA1_Channel5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hGpdma1Channel5);
}

void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&hTim2);
  ulHighFrequencyTimerTicks++;
}

void TIM6_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&hTim6);
}

void SPI2_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hSpi2);
}

void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&hUart1);
}
