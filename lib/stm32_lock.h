/**
  ******************************************************************************
  * @file      stm32_lock.h
  * @author    STMicroelectronics
  * @brief     STMicroelectronics lock mechanisms
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#ifndef __STM32_LOCK_H__
#define __STM32_LOCK_H__

#include <stdint.h>
#include <stddef.h>
#include <cmsis_compiler.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ErrorHandler(void);

#define STM32_LOCK_BLOCK()                      \
  do                                            \
  {                                             \
    __disable_irq();                            \
    ErrorHandler();                             \
  } while (0)

#define STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(x)    \
  do                                            \
  {                                             \
    if ((x) == NULL)                            \
    {                                           \
      STM32_LOCK_BLOCK();                       \
    }                                           \
  } while (0)

#define STM32_LOCK_BLOCK_IF_INTERRUPT_CONTEXT() \
  do                                            \
  {                                             \
    if (__get_IPSR())                           \
    {                                           \
      STM32_LOCK_BLOCK();                       \
    }                                           \
  } while (0)

#define STM32_LOCK_UNUSED(var) (void)var
#define STM32_LOCK_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#if defined(STM32_THREAD_SAFE_STRATEGY) && (STM32_THREAD_SAFE_STRATEGY == 4)
#include <FreeRTOS.h>
#include <task.h>

#if defined (__GNUC__) && configUSE_NEWLIB_REENTRANT == 0
#warning Please set configUSE_NEWLIB_REENTRANT to 1 in FreeRTOSConfig.h, otherwise newlib will not be thread-safe
#endif /* defined (__GNUC__) configUSE_NEWLIB_REENTRANT == 0 */

#define LOCKING_DATA_INIT { {0, 0}, 0 }
#define STM32_LOCK_MAX_NESTED_LEVELS 2
typedef struct {
  uint32_t basepri[STM32_LOCK_MAX_NESTED_LEVELS];
  uint8_t nesting_level;
} LockingData_t;

#define STM32_LOCK_ASSERT_VALID_NESTING_LEVEL(lock)                   \
  do                                                                  \
  {                                                                   \
    if (lock->nesting_level >= STM32_LOCK_ARRAY_SIZE(lock->basepri))  \
    {                                                                 \
      STM32_LOCK_BLOCK();                                             \
    }                                                                 \
  } while (0)

static inline void stm32_lock_init(LockingData_t *lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);

  for (size_t i = 0; i < STM32_LOCK_ARRAY_SIZE(lock->basepri); i++) {
    lock->basepri[i] = 0U;
  }
  lock->nesting_level = 0U;
}

static inline void stm32_lock_acquire(LockingData_t *lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  STM32_LOCK_ASSERT_VALID_NESTING_LEVEL(lock);
  const uint32_t mask = taskENTER_CRITICAL_FROM_ISR();
  lock->basepri[lock->nesting_level] = mask;
  lock->nesting_level++;
}

static inline void stm32_lock_release(LockingData_t *lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  lock->nesting_level--;
  STM32_LOCK_ASSERT_VALID_NESTING_LEVEL(lock);
  taskEXIT_CRITICAL_FROM_ISR(lock->basepri[lock->nesting_level]);
}

#undef STM32_LOCK_ASSERT_VALID_NESTING_LEVEL
#undef STM32_LOCK_MAX_NESTED_LEVELS
#else

#error Invalid STM32_THREAD_SAFE_STRATEGY specified
#endif /* STM32_THREAD_SAFE_STRATEGY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STM32_LOCK_H__ */
