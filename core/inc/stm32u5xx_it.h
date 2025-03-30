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
#ifndef STM32U5xx_IT_H
#define STM32U5xx_IT_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void BusFault_Handler(void);
void DebugMon_Handler(void);
void EXTI14_IRQHandler(void);
void EXTI15_IRQHandler(void);
void GPDMA1_Channel4_IRQHandler(void);
void GPDMA1_Channel5_IRQHandler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void NMI_Handler(void);
void SPI2_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM6_IRQHandler(void);
void USART1_IRQHandler(void);
void UsageFault_Handler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STM32U5xx_IT_H */
