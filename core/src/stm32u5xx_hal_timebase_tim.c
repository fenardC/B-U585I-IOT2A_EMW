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
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_tim.h"

TIM_HandleTypeDef hTim6;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  /*RCC_ClkInitTypeDef clock_config;*/
  /*uint32_t flash_latency;*/
  uint32_t timer_clock_frequency = 0;
  uint32_t prescaler_value = 0;
  HAL_StatusTypeDef status;

  __HAL_RCC_TIM6_CLK_ENABLE();

  /*HAL_RCC_GetClockConfig(&clock_config, &flash_latency);*/
  timer_clock_frequency = HAL_RCC_GetPCLK1Freq();
  prescaler_value = (uint32_t)((timer_clock_frequency / 1000000U) - 1U);

  hTim6.Instance = TIM6;
  hTim6.Init.Period = (1000000U / 1000U) - 1U;
  hTim6.Init.Prescaler = prescaler_value;
  hTim6.Init.ClockDivision = 0U;
  hTim6.Init.CounterMode = TIM_COUNTERMODE_UP;

  status = HAL_TIM_Base_Init(&hTim6);

  if (status == HAL_OK) {
    status = HAL_TIM_Base_Start_IT(&hTim6);

    if (status == HAL_OK) {
      if (TickPriority < (1UL << __NVIC_PRIO_BITS)) {
        HAL_NVIC_SetPriority(TIM6_IRQn, TickPriority, 0U);
        uwTickPrio = TickPriority;
      } else {
        status = HAL_ERROR;
      }
    }
  }

  HAL_NVIC_EnableIRQ(TIM6_IRQn);
  return status;
}

void HAL_SuspendTick(void)
{
  __HAL_TIM_DISABLE_IT(&hTim6, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
  __HAL_TIM_ENABLE_IT(&hTim6, TIM_IT_UPDATE);
}
