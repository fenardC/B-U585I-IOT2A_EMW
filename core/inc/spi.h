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
#ifndef SPI_H
#define SPI_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "stm32u5xx_hal.h"

//#include "stm32u5xx_hal_dma.h"
//#include "stm32u5xx_hal_spi.h"

extern DMA_HandleTypeDef hGpdma1Channel5;
extern DMA_HandleTypeDef hGpdma1Channel4;
extern SPI_HandleTypeDef hSpi2;

void InitializeSPI2(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SPI_H */
