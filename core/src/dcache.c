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
#include "dcache.h"
#include "stm32u5xx_hal.h"
//#include "stm32u5xx_hal_dcache.h"
#include "main.hpp"

static DCACHE_HandleTypeDef hDataCache1;

void InitializeDCACHE1(void)
{
  hDataCache1.Instance = DCACHE1;
  hDataCache1.Init.ReadBurstType = DCACHE_READ_BURST_WRAP;
  if (HAL_DCACHE_Init(&hDataCache1) != HAL_OK) {
    ErrorHandler();
  }
}

void HAL_DCACHE_MspInit(DCACHE_HandleTypeDef *handlePtr)
{
  if (handlePtr->Instance == DCACHE1) {
    __HAL_RCC_DCACHE1_CLK_ENABLE();
  }
}

void HAL_DCACHE_MspDeInit(DCACHE_HandleTypeDef *handlePtr)
{
  if (handlePtr->Instance == DCACHE1) {
    __HAL_RCC_DCACHE1_CLK_DISABLE();
  }
}
