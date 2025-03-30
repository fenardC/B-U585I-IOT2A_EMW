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
#ifndef STM32U5xx_HAL_CONF_H
#define STM32U5xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HAL_MODULE_ENABLED
#define HAL_DCACHE_MODULE_ENABLED
#define HAL_ICACHE_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE              16000000UL /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    100UL   /*!< Time out for HSE start up, in ms */
#endif /* HSE_STARTUP_TIMEOUT */

#if !defined(MSI_VALUE)
#define MSI_VALUE              4000000UL /*!< Value of the Internal oscillator in Hz*/
#endif /* MSI_VALUE */

#if !defined(HSI_VALUE)
#define HSI_VALUE              16000000UL /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

#if !defined(HSI48_VALUE)
#define HSI48_VALUE             48000000UL /*!< Value of the Internal High Speed oscillator for USB FS/SDMMC/RNG in Hz.
                                                The real value my vary depending on manufacturing process variations.*/
#endif /* HSI48_VALUE */

#if !defined(LSI_VALUE)
#define LSI_VALUE               32000UL    /*!< LSI Typical Value in Hz*/
#endif /* LSI_VALUE */

#if !defined(LSE_VALUE)
#define LSE_VALUE              32768UL   /*!< Value of the External oscillator in Hz*/
#endif /* LSE_VALUE */

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT    5000UL     /*!< Time out for LSE start up, in ms */
#endif /* LSE_STARTUP_TIMEOUT */

#if !defined(EXTERNAL_SAI1_CLOCK_VALUE)
#define EXTERNAL_SAI1_CLOCK_VALUE  48000UL /*!< Value of the SAI1 External clock source in Hz*/
#endif /* EXTERNAL_SAI1_CLOCK_VALUE */

#define  VDD_VALUE              3300UL /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY      (0UL)  /*!< tick interrupt priority (lowest by default) */
#define  USE_RTOS                0U
#define  PREFETCH_ENABLE         1U    /*!< Enable prefetch */

#define USE_FULL_ASSERT    1U

#define  USE_HAL_ADC_REGISTER_CALLBACKS        0U /* ADC register callback disabled       */
#define  USE_HAL_COMP_REGISTER_CALLBACKS       0U /* COMP register callback disabled      */
#define  USE_HAL_CORDIC_REGISTER_CALLBACKS     0U /* CORDIC register callback disabled    */
#define  USE_HAL_CRYP_REGISTER_CALLBACKS       0U /* CRYP register callback disabled      */
#define  USE_HAL_DAC_REGISTER_CALLBACKS        0U /* DAC register callback disabled       */
#define  USE_HAL_DCMI_REGISTER_CALLBACKS       0U /* DCMI register callback disabled      */
#define  USE_HAL_DMA2D_REGISTER_CALLBACKS      0U /* DMA2D register callback disabled     */
#define  USE_HAL_DSI_REGISTER_CALLBACKS        0U /* DSI register callback disabled       */
#define  USE_HAL_FDCAN_REGISTER_CALLBACKS      0U /* FDCAN register callback disabled     */
#define  USE_HAL_FMAC_REGISTER_CALLBACKS       0U /* FMAC register callback disabled      */
#define  USE_HAL_HASH_REGISTER_CALLBACKS       0U /* HASH register callback disabled      */
#define  USE_HAL_HCD_REGISTER_CALLBACKS        0U /* HCD register callback disabled       */
#define  USE_HAL_GFXMMU_REGISTER_CALLBACKS     0U /* GFXMMU register callback disabled    */
#define  USE_HAL_GFXTIM_REGISTER_CALLBACKS     0U /* GFXTIM register callback disabled    */
#define  USE_HAL_GPU2D_REGISTER_CALLBACKS      0U /* GPU2D register callback disabled     */
#define  USE_HAL_I2C_REGISTER_CALLBACKS        0U /* I2C register callback disabled       */
#define  USE_HAL_IWDG_REGISTER_CALLBACKS       0U /* IWDG register callback disabled      */
#define  USE_HAL_IRDA_REGISTER_CALLBACKS       0U /* IRDA register callback disabled      */
#define  USE_HAL_JPEG_REGISTER_CALLBACKS       0U /* JPEG register callback disabled      */
#define  USE_HAL_LPTIM_REGISTER_CALLBACKS      0U /* LPTIM register callback disabled     */
#define  USE_HAL_LTDC_REGISTER_CALLBACKS       0U /* LTDC register callback disabled      */
#define  USE_HAL_MDF_REGISTER_CALLBACKS        0U /* MDF register callback disabled       */
#define  USE_HAL_MMC_REGISTER_CALLBACKS        0U /* MMC register callback disabled       */
#define  USE_HAL_NAND_REGISTER_CALLBACKS       0U /* NAND register callback disabled      */
#define  USE_HAL_NOR_REGISTER_CALLBACKS        0U /* NOR register callback disabled       */
#define  USE_HAL_OPAMP_REGISTER_CALLBACKS      0U /* MDIO register callback disabled      */
#define  USE_HAL_OTFDEC_REGISTER_CALLBACKS     0U /* OTFDEC register callback disabled    */
#define  USE_HAL_PCD_REGISTER_CALLBACKS        0U /* PCD register callback disabled       */
#define  USE_HAL_PKA_REGISTER_CALLBACKS        0U /* PKA register callback disabled       */
#define  USE_HAL_RAMCFG_REGISTER_CALLBACKS     0U /* RAMCFG register callback disabled    */
#define  USE_HAL_RNG_REGISTER_CALLBACKS        0U /* RNG register callback disabled       */
#define  USE_HAL_RTC_REGISTER_CALLBACKS        0U /* RTC register callback disabled       */
#define  USE_HAL_SAI_REGISTER_CALLBACKS        0U /* SAI register callback disabled       */
#define  USE_HAL_SD_REGISTER_CALLBACKS         0U /* SD register callback disabled        */
#define  USE_HAL_SDRAM_REGISTER_CALLBACKS      0U /* SDRAM register callback disabled     */
#define  USE_HAL_SMARTCARD_REGISTER_CALLBACKS  0U /* SMARTCARD register callback disabled */
#define  USE_HAL_SMBUS_REGISTER_CALLBACKS      0U /* SMBUS register callback disabled     */
#define  USE_HAL_SPI_REGISTER_CALLBACKS        1U /* SPI register callback enabled       */
#define  USE_HAL_SRAM_REGISTER_CALLBACKS       0U /* SRAM register callback disabled      */
#define  USE_HAL_TIM_REGISTER_CALLBACKS        0U /* TIM register callback disabled       */
#define  USE_HAL_TSC_REGISTER_CALLBACKS        0U /* TSC register callback disabled       */
#define  USE_HAL_UART_REGISTER_CALLBACKS       0U /* UART register callback disabled      */
#define  USE_HAL_USART_REGISTER_CALLBACKS      0U /* USART register callback disabled     */
#define  USE_HAL_WWDG_REGISTER_CALLBACKS       0U /* WWDG register callback disabled      */
#define  USE_HAL_OSPI_REGISTER_CALLBACKS       0U /* OSPI register callback disabled      */

#define USE_SPI_CRC                   0U
#define USE_SD_TRANSCEIVER            0U

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32u5xx_hal_rcc.h"
#endif /* HAL_RCC_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32u5xx_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_ICACHE_MODULE_ENABLED
#include "stm32u5xx_hal_icache.h"
#endif /* HAL_ICACHE_MODULE_ENABLED */

#ifdef HAL_DCACHE_MODULE_ENABLED
#include "stm32u5xx_hal_dcache.h"
#endif /* HAL_DCACHE_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32u5xx_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32u5xx_hal_cortex.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32u5xx_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32u5xx_hal_pwr.h"
#endif /* HAL_PWR_MODULE_ENABLED */

#ifdef HAL_RNG_MODULE_ENABLED
#include "stm32u5xx_hal_rng.h"
#endif /* HAL_RNG_MODULE_ENABLED */

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32u5xx_hal_spi.h"
#endif /* HAL_SPI_MODULE_ENABLED */

#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32u5xx_hal_tim.h"
#endif /* HAL_TIM_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32u5xx_hal_uart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_USART_MODULE_ENABLED
#include "stm32u5xx_hal_usart.h"
#endif /* HAL_USART_MODULE_ENABLED */

#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32u5xx_hal_exti.h"
#endif /* HAL_EXTI_MODULE_ENABLED */

#if defined(USE_FULL_ASSERT) && (USE_FULL_ASSERT == 1)
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((const char *)__FILE__, __LINE__))
void assert_failed(const char *fileName, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STM32U5xx_HAL_CONF_H */
