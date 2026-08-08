/**
  ******************************************************************************
  * Copyright (C) 2025-2026 C.Fenard.
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
#include "main.hpp"
#include "stm32u5xx_hal.h"
#include "dcache.h"
#include "gpdma.h"
#include "gpio.h"
#include "icache.h"
#include "rng.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "stdio_uart.h"

#if defined(COMPILATION_WITH_EMW)
#include "AppWiFiEmw.hpp"
#elif defined(COMPILATION_WITH_LWIP)
#include "AppWiFiLwip.hpp"
#include "WiFiNetworkCore.hpp"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#endif /* COMPILATION_WITH_EMW) */

#include "wifi_emw.hpp"


#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS) */

#include <errno.h>
#include <cinttypes>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#define STD_PRINTF(...) static_cast<void>(std::printf(__VA_ARGS__))

static void InitializeMainTask(void);

#if defined(COMPILATION_WITH_FREERTOS)
static void FreeRtosMainTask(void *argumentPtr);
#endif /* COMPILATION_WITH_FREERTOS) */

static void ConfigureSystemClock(void);
static void ConfigureSystemPower(void);
static void CheckArmCore(void);
static void CheckExceptionHandling(void);
void ExitingFunction(void);

/* http://en.wikipedia.org/wiki/ANSI_escape_code */
/* static const std::string BLUE("\x1b[1;34m"); */
/* static const std::string RED("\x1b[1;31m"); */
static const std::string GREEN("\x1b[1;32m");
/* static const std::string MAGENTA("\x1b[1;35m"); */
/* static const std::string YELLOW("\x1b[1;33m"); */
static const std::string NORMAL("\x1b[0;39m");

int main(void)
{
  register std::uint8_t *stack_ptr __ASM("sp");
  extern std::uint8_t end __ASM("end");

  HAL_Init();
  ConfigureSystemClock();
  ConfigureSystemPower();
  InitializeGPIOs();
  InitializeUSART1();
  InitializeTIM2();
  InitializeGPDMA1();
  InitializeICACHE();
  InitializeDCACHE1();
  InitializeSPI2();
  InitializeRNG();
  {
    const char welcome[] = " ~ Welcome ~\n";
    const std::uint16_t welcome_size = static_cast<std::uint16_t>(std::strlen(welcome));
    static_cast<void>(HAL_UART_Transmit(&hUart1, reinterpret_cast<const std::uint8_t *>(welcome), welcome_size, 0xFFFF));
  }
  static_cast<void>(InitializeStdoutWithUart());
  static_cast<void>(InitializeStdinWithUart());
  std::setbuf(stdout, NULL);
  std::setbuf(stdin, NULL);
  static_cast<void>(std::printf(" BUFSIZ %" PRIu32 "\n", static_cast<std::uint32_t>(BUFSIZ)));
  {
    char the_compiler_string[70] = {""};

    {
#if defined(__GNUC__)
      const char *gnu_compiler_string = " __GNUC__";
#else
      const char *gnu_compiler_string = " UNKNOWN";
#endif /* __GNUC__ */
      const std::size_t the_compiler_string_size = sizeof(the_compiler_string) - 1;

      std::strncat(the_compiler_string, gnu_compiler_string, the_compiler_string_size - std::strlen(the_compiler_string));
    }

    {
      const std::uint32_t start_counting = HAL_GetTick();
      static volatile std::uint32_t count = 0U;
      for (volatile std::uint32_t i = 0; i < 1600000U;) {
        count = count + 1;
        i = i + 1;
      }
      {
        const std::uint32_t end_counting = HAL_GetTick();
        STD_PRINTF("\n[%6" PRIu32 "] main(): %s %s  (%s)   : %" PRIu32 " ms for %" PRIu32 " loops\n\n",
                   end_counting, __DATE__, __TIME__, the_compiler_string, end_counting - start_counting, count);
      }
    }
  }
  {
    int random_number = HardwareRand();
    std::srand(static_cast<unsigned>(random_number));
    STD_PRINTF(" std::rand() seeded by %" PRId32 " returned %" PRId32 "\n\n",
               static_cast<std::int32_t>(random_number), static_cast<std::int32_t>(std::rand()));
  }
  STD_PRINTF("BUFSIZ %" PRIu32 "\n", static_cast<std::uint32_t>(BUFSIZ));

  CheckArmCore();
  CheckExceptionHandling();
  std::printf(" main(): sp : %10p\n", stack_ptr);
  std::printf(" main(): end: %10p\n", &end);
  std::printf(" main(): clk: %10" PRIi32 "\n", SystemCoreClock);
  std::atexit(ExitingFunction);

  InitializeMainTask();

#if defined(COMPILATION_WITH_FREERTOS)
  STD_PRINTF(" main(): StackType_t with: %" PRIu32 "\n", static_cast<std::uint32_t>(sizeof(StackType_t)));
  vTaskStartScheduler();
#endif /* COMPILATION_WITH_FREERTOS) */
  while (1) {
  }
}

static void InitializeMainTask(void)
{
#if defined(COMPILATION_WITH_NO_OS)
#if defined(COMPILATION_WITH_EMW)
  InitializeEmw(Emw);
  AppTaskFunction(&Emw);
#elif defined(COMPILATION_WITH_LWIP)
#error "NOT YET SUPPORTED"
#endif /* COMPILATION_WITH_EMW */
#endif /* COMPILATION_WITH_NO_OS */

#if defined(COMPILATION_WITH_FREERTOS)
  {
    const char main_task_name[] = {"MainTask"};
    void *arg_ptr = NULL;
#if defined(COMPILATION_WITH_EMW)
    arg_ptr = &Emw;
#elif defined(COMPILATION_WITH_LWIP)
    arg_ptr = &EmwBypass;
#endif /* COMPILATION_WITH_EMW */
    static_cast<void>(xTaskCreate(FreeRtosMainTask, main_task_name, 1024U, arg_ptr, 16U, NULL));
  }
#endif /* COMPILATION_WITH_FREERTOS) */
}

#if defined(COMPILATION_WITH_FREERTOS)
static void FreeRtosMainTask(void *argumentPtr)
{
  const char app_task_function_name[] = {"AppWiFi"};
  auto &emw = *(static_cast<class EmwApiCore*>(argumentPtr));

#if defined(STM32_THREAD_SAFE_STRATEGY)
  {
    char the_thread_safe_strategy[40] = {""};

    static_cast<void>(std::strcat(the_thread_safe_strategy, "STM32_THREAD_SAFE_STRATEGY "));
    static_cast<void>(std::sprintf(&the_thread_safe_strategy[std::strlen(the_thread_safe_strategy)],
                                   "(%" PRIu32 ") (%" PRIu32 " - %" PRIu32 ")",
                                   static_cast<uint32_t>(STM32_THREAD_SAFE_STRATEGY),
                                   static_cast<uint32_t>(__NEWLIB__), static_cast<uint32_t>(_RETARGETABLE_LOCKING)));
    static_cast<void>(std::printf("\n\n [%" PRIu32 "] FreeRtosMainTask(): %s\n", HAL_GetTick(), the_thread_safe_strategy));
  }
#endif /* STM32_THREAD_SAFE_STRATEGY */

#if defined(COMPILATION_WITH_LWIP)
  STD_PRINTF("MEM_SIZE           : %6" PRIu32 "\n", static_cast<std::uint32_t>(MEM_SIZE));
  STD_PRINTF("PBUF_POOL_BUFSIZE  : %6" PRIu32 "\n\n", static_cast<std::uint32_t>(PBUF_POOL_BUFSIZE));
#if LWIP_IPV4 && LWIP_IPV6
  STD_PRINTF("LWIP_IPV4, LWIP_IPV6\n");
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#endif /* COMPILATION_WITH_LWIP) */

#if defined(COMPILATION_WITH_LWIP)
  tcpip_init(NULL, NULL);
#endif /* COMPILATION_WITH_LWIP) */

  InitializeEmw(emw);

#if defined(COMPILATION_WITH_LWIP)
  WiFiNetworkCore::InitializeSystem(&emw);
#endif /* COMPILATION_WITH_LWIP) */

  static_cast<void>(xTaskCreate(AppTaskFunction, app_task_function_name, 2224U, argumentPtr, 16U, NULL));

  for (;;) {
    vTaskDelay(1000U);
  }
}
#endif /* COMPILATION_WITH_FREERTOS) */

static void CheckArmCore(void)
{
  const std::uint32_t cpuid = (reinterpret_cast<SCB_Type *>(SCB_BASE))->CPUID;
  const std::uint32_t idcode = (reinterpret_cast<DBGMCU_TypeDef *>(DBGMCU_BASE))->IDCODE;
  std::uint32_t var;
  std::uint32_t pat;

  STD_PRINTF(" ---------------------------------------------------------\n");
  STD_PRINTF(" CPUID: %08" PRIX32 ", DEVID: %03" PRIX32 ", REVID: %04" PRIX32 "\n",
             cpuid, idcode & 0xFFF, (idcode >> 16) & 0xFFFF);
  pat = (cpuid & 0x0000000F);
  var = (cpuid & 0x00F00000) >> 20;
  if ((cpuid & 0xFF000000) == 0x41000000) { /* ARM */
    switch ((cpuid & 0x0000FFF0) >> 4) {
      case 0xC20 : {
          STD_PRINTF(" Cortex M0 r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      case 0xC60 : {
          STD_PRINTF(" Cortex M0+ r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      case 0xC21 : {
          STD_PRINTF(" Cortex M1 r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      case 0xC23 : {
          STD_PRINTF(" Cortex M3 r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      case 0xC24 : {
          STD_PRINTF(" Cortex M4 r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      case 0xC27 : {
          STD_PRINTF(" Cortex M7 r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      case 0xD21 : {
          STD_PRINTF(" Cortex M33 r%" PRId32 "p%" PRId32 "\n", var, pat);
          break;
        }
      default :
        STD_PRINTF(" Unknown CORE\n");
    }
  }
  else {
    STD_PRINTF(" Unknown CORE IMPLEMENTER\n");
  }
  STD_PRINTF(" ---------------------------------------------------------\n");
}

void CheckExceptionHandling(void)
{
  std::cout << " ---------------------------------------------------------\n" << std::endl;
  try {
    throw std::runtime_error("Checking C++ exception");
  }
  catch (const std::runtime_error &error) {
    std::cout << GREEN << " CheckExceptionHandling(): " << "**" << error.what() << "**" << NORMAL << std::endl;
  }
  std::cout << " ---------------------------------------------------------\n" << std::endl;
}

void ExitingFunction(void)
{
  const std::int32_t errno_value = errno;
  STD_PRINTF("\n ExitingFunction()> errno: (%" PRId32 ") \"%s\"\n", errno_value, strerror(errno_value));
}

static void ConfigureSystemClock(void)
{
  {
    RCC_OscInitTypeDef configuration = {
      0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}
    };

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
      ErrorHandler();
    }

    configuration.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_MSI;
    configuration.HSI48State = RCC_HSI48_ON;
    configuration.MSIState = RCC_MSI_ON;
    configuration.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    configuration.MSIClockRange = RCC_MSIRANGE_0;
    configuration.PLL.PLLState = RCC_PLL_ON;
    configuration.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    configuration.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
    configuration.PLL.PLLM = 3;
    configuration.PLL.PLLN = 10;
    configuration.PLL.PLLP = 2;
    configuration.PLL.PLLQ = 2;
    configuration.PLL.PLLR = 1;
    configuration.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    configuration.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&configuration) != HAL_OK) {
      ErrorHandler();
    }
  }
  {
    RCC_ClkInitTypeDef configuration = {0U, 0U, 0U, 0U, 0U, 0U};
    configuration.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                              | RCC_CLOCKTYPE_PCLK3;
    configuration.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    configuration.AHBCLKDivider = RCC_SYSCLK_DIV1;
    configuration.APB1CLKDivider = RCC_HCLK_DIV1;
    configuration.APB2CLKDivider = RCC_HCLK_DIV1;
    configuration.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&configuration, FLASH_LATENCY_4) != HAL_OK) {
      ErrorHandler();
    }
  }
}

static void ConfigureSystemPower(void)
{
  HAL_PWREx_EnableVddIO2();
  HAL_PWREx_DisableUCPDDeadBattery();
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK) {
    ErrorHandler();
  }
}

extern "C" int HardwareRand(void)
{
  while ((hRng.Instance->SR & RNG_SR_DRDY) == 0);
  return (static_cast<int>(hRng.Instance->DR));
}

extern "C" void HAL_TIM_PeriodElapsedCallback(const TIM_HandleTypeDef *handlePtr)
{
  if (handlePtr->Instance == reinterpret_cast<TIM_TypeDef *>(TIM6_BASE_NS)) {
    HAL_IncTick();
  }
}

extern "C" void ErrorHandler(void)
{
  while (1) {
#if defined(COMPILATION_WITH_FREERTOS)
    /* Be cooperative. */
    vTaskDelay(1U);
#endif /* COMPILATION_WITH_FREERTOS) */
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(const char *fileName, uint32_t line)
{
  STD_PRINTF(" Wrong parameters value: file %s on line %" PRIu32 "\n", fileName, line);
}
#endif /* USE_FULL_ASSERT */
