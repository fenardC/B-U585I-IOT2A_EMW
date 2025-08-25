/*
 * FreeRTOS Kernel V11.2.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stm32u585xx.h"
#include <stdint.h>

/******************************************************************************/
/* Hardware description related definitions. **********************************/
/******************************************************************************/
#define configCPU_CLOCK_HZ                         SystemCoreClock

#define configENABLE_TRUSTZONE                     0
#define configRUN_FREERTOS_SECURE_ONLY             0
#define configENABLE_FPU                           0
#define configENABLE_MPU                           0

/******************************************************************************/
/* Scheduling behaviour related definitions. **********************************/
/******************************************************************************/
#define configTICK_RATE_HZ                         1000
#define configUSE_PREEMPTION                       1
#define configUSE_TIME_SLICING                     0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
#define configMAX_PRIORITIES                       32
#define configMINIMAL_STACK_SIZE                   128
#define configMAX_TASK_NAME_LEN                    20
#define configQUEUE_REGISTRY_SIZE                  24
#define configUSE_MINI_LIST_ITEM                   1
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t
#define configUSE_NEWLIB_REENTRANT                 1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    1


/******************************************************************************/
/* Software timer related definitions. ****************************************/
/******************************************************************************/
#define configUSE_TIMERS                           1
#define configTIMER_TASK_PRIORITY                  2
#define configTIMER_QUEUE_LENGTH                   10
#define configTIMER_TASK_STACK_DEPTH               configMINIMAL_STACK_SIZE

/******************************************************************************/
/* Memory allocation related definitions. *************************************/
/******************************************************************************/
#define configSUPPORT_STATIC_ALLOCATION            0
#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configTOTAL_HEAP_SIZE                      (130*1024)
#define configENABLE_HEAP_PROTECTOR                1

/******************************************************************************/
/* Interrupt nesting behaviour configuration. *********************************/
/******************************************************************************/
#define configPRIO_BITS                            __NVIC_PRIO_BITS

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY    15
#define configKERNEL_INTERRUPT_PRIORITY \
  (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/******************************************************************************/
/* Hook and callback function related definitions. ****************************/
/******************************************************************************/
#define configUSE_IDLE_HOOK                        0
#define configUSE_TICK_HOOK                        0
#define configUSE_MALLOC_FAILED_HOOK               1
#define configCHECK_FOR_STACK_OVERFLOW             2

/******************************************************************************/
/* Run time and task stats gathering related definitions. *********************/
/******************************************************************************/
#define configGENERATE_RUN_TIME_STATS              1
#define configUSE_TRACE_FACILITY                   1
#define configUSE_STATS_FORMATTING_FUNCTIONS       1

/******************************************************************************/
/* Co-routine related definitions. ********************************************/
/******************************************************************************/
#define configUSE_CO_ROUTINES                      0
#define configMAX_CO_ROUTINE_PRIORITIES            1

/******************************************************************************/
/* Debugging assistance. ******************************************************/
/******************************************************************************/
#define configASSERT( x )     \
  if( ( x ) == 0 )            \
  {                           \
    taskDISABLE_INTERRUPTS(); \
    for( ; ; )                \
    ;                         \
  }
/******************************************************************************/
/* Definitions that include or exclude functionality. *************************/
/******************************************************************************/
#define configUSE_TASK_NOTIFICATIONS               1
#define configUSE_MUTEXES                          1
#define configUSE_RECURSIVE_MUTEXES                1
#define configUSE_COUNTING_SEMAPHORES              1
#define configUSE_16_BIT_TICKS                     0
#define configUSE_SB_COMPLETED_CALLBACK            0
#define configRECORD_STACK_HIGH_ADDRESS            1
#define configRUN_TIME_COUNTER_TYPE                size_t

#define INCLUDE_vTaskPrioritySet                   1
#define INCLUDE_uxTaskPriorityGet                  1
#define INCLUDE_vTaskDelete                        1
#define INCLUDE_vTaskCleanUpResources              1
#define INCLUDE_vTaskSuspend                       1
#define INCLUDE_vTaskDelayUntil                    1
#define INCLUDE_vTaskDelay                         1
#define INCLUDE_xTaskGetSchedulerState             1
#define INCLUDE_xTaskGetCurrentTaskHandle          1
#define INCLUDE_uxTaskGetStackHighWaterMark        1
#define INCLUDE_eTaskGetState                      1
#define INCLUDE_xTimerPendFunctionCall             1
#define INCLUDE_xTaskGetHandle                     1
#define INCLUDE_xEventGroupSetBitFromISR           1
#define INCLUDE_xQueueGetMutexHolder               1
#define INCLUDE_xSemaphoreGetMutexHolder           1
#define INCLUDE_pcTaskGetTaskName                  1

void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS configureTimerForRunTimeStats
#define portGET_RUN_TIME_COUNTER_VALUE getRunTimeCounterValue

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FREERTOS_CONFIG_H */
