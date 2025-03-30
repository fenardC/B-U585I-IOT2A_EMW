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
#include "rng.h"
#include "stm32u5xx_hal_rcc.h"
#include "main.hpp"

RNG_HandleTypeDef hRng;

void InitializeRNG(void)
{
  hRng.Instance = RNG;
  hRng.Init.ClockErrorDetection = RNG_CED_ENABLE;

  if (HAL_RNG_Init(&hRng) != HAL_OK) {
    ErrorHandler();
  }
}

void HAL_RNG_MspInit(RNG_HandleTypeDef *rngPtr)
{
  if (rngPtr->Instance == RNG) {
    RCC_PeriphCLKInitTypeDef configuration = {0};

    configuration.PeriphClockSelection = RCC_PERIPHCLK_RNG;
    configuration.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;

    if (HAL_RCCEx_PeriphCLKConfig(&configuration) != HAL_OK) {
      ErrorHandler();
    }

    __HAL_RCC_RNG_CLK_ENABLE();
  }
}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef *rngPtr)
{
  if (rngPtr->Instance == RNG) {
    __HAL_RCC_RNG_CLK_DISABLE();
  }
}
