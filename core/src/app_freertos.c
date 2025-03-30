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
#include "tim.h"
#include "app_freertos.h"
#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern void AppTaskFunction(void *argumentPtr);

static void StartRootTask(void *argumentPtr);

void InitializeFreeRtosRootTask(void)
{
  const char start_root_task_name[] = {"RootTask"};

  (void) xTaskCreate(StartRootTask, start_root_task_name, 1024U, NULL, 16U, NULL);
}

static void StartRootTask(void *argumentPtr)
{
  (void) argumentPtr;
  const char app_task_function_name[] = {"AppWiFi"};

#if defined(STM32_THREAD_SAFE_STRATEGY)
  {
    char the_thread_safe_strategy[40] = {""};

    strcat(the_thread_safe_strategy, "STM32_THREAD_SAFE_STRATEGY ");
    sprintf(&the_thread_safe_strategy[strlen(the_thread_safe_strategy)], "(%" PRIu32 ") (%" PRIu32 " - %" PRIu32 ")",
            (uint32_t)STM32_THREAD_SAFE_STRATEGY, (uint32_t)__NEWLIB__, (uint32_t)_RETARGETABLE_LOCKING);
    printf("\n\n[%" PRIu32 "] StartRootTask(): %s\n", HAL_GetTick(), the_thread_safe_strategy);
  }
#endif /* STM32_THREAD_SAFE_STRATEGY */
  (void) xTaskCreate(AppTaskFunction, app_task_function_name, 2224U, (void *) NULL, 16U, NULL);
  for (;;) {
    vTaskDelay(1000U);
  }
}

void vApplicationMallocFailedHook(void)
{
  printf("Run out of memory\n");
  while (true) {}
}

void vApplicationStackOverflowHook(xTaskHandle task, char *taskNamePtr)
{
  (void) task;
  printf("Stack overflow: %s", taskNamePtr);
  while (true) {}
}

#if configENABLE_HEAP_PROTECTOR
void vApplicationGetRandomHeapCanary(portPOINTER_SIZE_TYPE *pxHeapCanary);
void vApplicationGetRandomHeapCanary(portPOINTER_SIZE_TYPE *pxHeapCanary)
{
  *pxHeapCanary = 0xdead3472;
}
#endif /* configENABLE_HEAP_PROTECTOR */

void configureTimerForRunTimeStats(void)
{
  HAL_TIM_Base_Start_IT(&hTim2);
}

unsigned long getRunTimeCounterValue(void)
{
  return ulHighFrequencyTimerTicks;
}
