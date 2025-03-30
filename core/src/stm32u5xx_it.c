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
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "cmsis_compiler.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */
#include "stm32u5xx_it.h"
#include "gpdma.h"
#include "gpio.h"
#include "icache.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

#include <stdio.h>
#include <inttypes.h>

#define EMW_FLOW_Pin GPIO_PIN_15
#define EMW_NOTIFY_Pin GPIO_PIN_14

extern TIM_HandleTypeDef hTim6;

void NMI_Handler(void)
{
  printf("NMI_Handler()>\n");
  while (1) {
  }
}

typedef __PACKED_STRUCT {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t return_address;
  uint32_t xpsr;
} ContextStateFrame_t;

void my_fault_handler_c(ContextStateFrame_t *framePtr);

/**
  * Disable optimizations for this function so "frame" argument
  * does not get optimized away
  */
__attribute__((optimize("O0")))
void my_fault_handler_c(ContextStateFrame_t *framePtr)
{
  printf("R0 : 0x%08" PRIx32 "\n", framePtr->r0);
  printf("R1 : 0x%08" PRIx32 "\n", framePtr->r1);
  printf("R2 : 0x%08" PRIx32 "\n", framePtr->r2);
  printf("R3 : 0x%08" PRIx32 "\n", framePtr->r3);
  printf("R12: 0x%08" PRIx32 "\n", framePtr->r12);
  printf("LR : 0x%08" PRIx32 "\n", framePtr->lr);
  printf("   : 0x%08" PRIx32 "\n", framePtr->return_address);
  printf("   : 0x%08" PRIx32 "\n", framePtr->xpsr);
  while (1) {
  }
}

#define HARDFAULT_HANDLING_ASM(_x)   \
  __asm volatile(                    \
      "tst lr, #4 \n"                \
      "ite eq \n"                    \
      "mrseq r0, msp \n"             \
      "mrsne r0, psp \n"             \
      "b my_fault_handler_c \n"      \
                                     )

void HardFault_Handler(void)
{
  HARDFAULT_HANDLING_ASM();
  while (1) {
  }
}

void MemManage_Handler(void)
{

  printf("MemManage_Handler()>\n");
  while (1) {
  }
}

void BusFault_Handler(void)
{
  printf("BusFault_Handler()>\n");
  while (1) {
  }
}

void UsageFault_Handler(void)
{
  printf("UsageFault_Handler()>\n");
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
