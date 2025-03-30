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
#include "gpio.h"
#include "stm32u5xx_hal.h"
//#include "stm32u5xx_hal_gpio.h"
//#include "stm32u5xx_hal_rcc.h"
//#include "stm32u5xx_hal_cortex.h"
#include "emw_conf.hpp"

void InitializeGPIOs(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  HAL_GPIO_WritePin(EMW_NSS_GPIO_Port, EMW_NSS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(EMW_RESET_GPIO_Port, EMW_RESET_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = EMW_FLOW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EMW_FLOW_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = EMW_NOTIFY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EMW_NOTIFY_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = EMW_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(EMW_NSS_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = EMW_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EMW_RESET_GPIO_Port, &GPIO_InitStruct);
  HAL_NVIC_SetPriority(EXTI14_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI14_IRQn);
  HAL_NVIC_SetPriority(EXTI15_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI15_IRQn);
}
