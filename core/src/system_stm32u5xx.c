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
#include "stm32u5xx.h"
#include <math.h>

#if !defined(HSE_VALUE)
#define HSE_VALUE    16000000U
#endif /* HSE_VALUE */

#if !defined(MSI_VALUE)
#define MSI_VALUE    4000000U
#endif /* MSI_VALUE */

#if !defined(HSI_VALUE)
#define HSI_VALUE    16000000U
#endif /* HSI_VALUE */

#define VECT_TAB_OFFSET  0x00000000UL

uint32_t SystemCoreClock = 4000000U;

const uint8_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t APBPrescTable[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
const uint32_t MSIRangeTable[16] = {48000000U, 24000000U, 16000000U, 12000000U, 4000000U, 2000000U, 1330000U, \
                                    1000000U, 3072000U, 1536000U, 1024000U, 768000U, 400000U, 200000U, 133000U, 100000U
                                   };

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 20U) | (3UL << 22U)); /* set CP10 and CP11 Full Access */
#endif

  RCC->CR = RCC_CR_MSISON;
  RCC->CFGR1 = 0U;
  RCC->CFGR2 = 0U;
  RCC->CFGR3 = 0U;
  RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON);
  RCC->PLL1CFGR = 0U;
  RCC->CR &= ~(RCC_CR_HSEBYP);
  RCC->CIER = 0U;
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
}

void SystemCoreClockUpdate(void)
{
  uint32_t pllr, pllsource, pllm, tmp, pllfracen, msirange;
  float_t fracn1, pllvco;

  if (READ_BIT(RCC->ICSCR1, RCC_ICSCR1_MSIRGSEL) == 0U) {
    msirange = (RCC->CSR & RCC_CSR_MSISSRANGE) >> RCC_CSR_MSISSRANGE_Pos;
  } else {
    msirange = (RCC->ICSCR1 & RCC_ICSCR1_MSISRANGE) >> RCC_ICSCR1_MSISRANGE_Pos;
  }

  msirange = MSIRangeTable[msirange];

  switch (RCC->CFGR1 & RCC_CFGR1_SWS) {
    case 0x00:
      SystemCoreClock = msirange;
      break;

    case 0x04:
      SystemCoreClock = HSI_VALUE;
      break;

    case 0x08:
      SystemCoreClock = HSE_VALUE;
      break;

    case 0x0C:
      pllsource = (RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC);
      pllm = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1M) >> RCC_PLL1CFGR_PLL1M_Pos) + 1U;
      pllfracen = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1FRACEN) >> RCC_PLL1CFGR_PLL1FRACEN_Pos);
      fracn1 = (float_t)(uint32_t)(pllfracen * ((RCC->PLL1FRACR & RCC_PLL1FRACR_PLL1FRACN) >>
                                   RCC_PLL1FRACR_PLL1FRACN_Pos));

      switch (pllsource) {
        case 0x00:
          pllvco = (float_t)0U;
          break;

        case 0x02:
          pllvco = ((float_t)HSI_VALUE / (float_t)pllm);
          break;

        case 0x03:
          pllvco = ((float_t)HSE_VALUE / (float_t)pllm);
          break;

        default:
          pllvco = ((float_t)msirange / (float_t)pllm);
          break;
      }

      pllvco = pllvco * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + (fracn1 /
                         (float_t)0x2000) + (float_t)1U);
      pllr = (((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1R) >> RCC_PLL1DIVR_PLL1R_Pos) + 1U);
      SystemCoreClock = (uint32_t)((uint32_t)pllvco / pllr);
      break;

    default:
      SystemCoreClock = msirange;
      break;
  }

  tmp = AHBPrescTable[((RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos)];
  SystemCoreClock >>= tmp;
}
