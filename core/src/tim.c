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
#include "tim.h"
#include "main.hpp"

TIM_HandleTypeDef hTim2;
volatile unsigned long ulHighFrequencyTimerTicks = 0U;

void InitializeTIM2(void)
{
  hTim2.Instance = TIM2;
  hTim2.Init.Prescaler = 1599; /* 100Khz */
  hTim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  hTim2.Init.Period = 10; /* 10 Khz */
  hTim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  hTim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&hTim2) != HAL_OK) {
    ErrorHandler();
  }

  {
    TIM_ClockConfigTypeDef configuration = {0};

    configuration.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    if (HAL_TIM_ConfigClockSource(&hTim2, &configuration) != HAL_OK) {
      ErrorHandler();
    }
  }

  {
    TIM_MasterConfigTypeDef configuration = {0};

    configuration.MasterOutputTrigger = TIM_TRGO_RESET;
    configuration.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(&hTim2, &configuration) != HAL_OK) {
      ErrorHandler();
    }
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *timPtr)
{
  if (timPtr->Instance == TIM2) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *timPtr)
{
  if (timPtr->Instance == TIM2) {
    __HAL_RCC_TIM2_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
  }
}
