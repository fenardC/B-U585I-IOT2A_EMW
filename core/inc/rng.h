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
#ifndef RNG_H
#define RNG_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "stm32u5xx_hal.h"
//#include "stm32u5xx_hal_rng.h"

extern RNG_HandleTypeDef hRng;

void InitializeRNG(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* RNG_H */
