/**
  ******************************************************************************
  * @file      newlib_lock_glue.c
  * @author    STMicroelectronics
  * @brief     Implementation of newlib lock interface
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
#if !defined (__GNUC__)
#error "newlib_lock_glue.c" should be used with GNU Compilers only
#endif /* !defined (__GNUC__) */

#include <cmsis_compiler.h>
#include <stdatomic.h>

#include "stm32_lock.h"

typedef struct {
  atomic_uchar initialized; /**< Indicate if object is initialized */
  uint8_t acquired; /**< Ensure non-recursive lock */
  uint16_t unused; /**< Padding */
} __attribute__((packed)) CxaGuardObject_t;

int __cxa_guard_acquire(CxaGuardObject_t *guard_object);
void __cxa_guard_abort(CxaGuardObject_t *guard_object);
void __cxa_guard_release(CxaGuardObject_t *guard_object);


__WEAK void ErrorHandler(void)
{
  while (1);
}

#ifdef __SINGLE_THREAD__
#warning C library is in single-threaded mode. Please take care when using C library functions in threaded contexts
#else
#include <newlib.h>

#if (__NEWLIB__ >= 3) && defined (_RETARGETABLE_LOCKING)
#include <errno.h>
#include <stdlib.h>
#include <sys/lock.h>

#define STM32_LOCK_PARAMETER(lock) (&(lock)->lock_data)
struct __lock {
  LockingData_t lock_data;
};

struct __lock __lock___sinit_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___sfp_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___atexit_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___at_quick_exit_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___malloc_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___env_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___tz_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___dd_hash_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___arc4random_mutex = { LOCKING_DATA_INIT };

void __retarget_lock_init(_LOCK_T *lock)
{
  __retarget_lock_init_recursive(lock);
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
  if (lock == NULL) {
    errno = EINVAL;
    return;
  }
  *lock = (_LOCK_T)malloc(sizeof(struct __lock));
  if (*lock != NULL) {
    stm32_lock_init(STM32_LOCK_PARAMETER(*lock));
    return;
  }
  STM32_LOCK_BLOCK();
}

void __retarget_lock_close(_LOCK_T lock)
{
  __retarget_lock_close_recursive(lock);
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
  free(lock);
}

void __retarget_lock_acquire(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_acquire(STM32_LOCK_PARAMETER(lock));
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_acquire(STM32_LOCK_PARAMETER(lock));
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
  __retarget_lock_acquire(lock);
  return 0;
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
  __retarget_lock_acquire_recursive(lock);
  return 0;
}

void __retarget_lock_release(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_release(STM32_LOCK_PARAMETER(lock));
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_release(STM32_LOCK_PARAMETER(lock));
}
#endif /* (__NEWLIB__ >= 3) && defined(_RETARGETABLE_LOCKING) */

static LockingData_t __cxa_guard_mutex = LOCKING_DATA_INIT;

int __cxa_guard_acquire(CxaGuardObject_t *guard_object)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(guard_object);

  if (atomic_load(&guard_object->initialized) == 0) {
    stm32_lock_acquire(&__cxa_guard_mutex);
    if (atomic_load(&guard_object->initialized) == 0) {
      if (guard_object->acquired) {
        STM32_LOCK_BLOCK();
      }
      guard_object->acquired = 1;
      return 1;
    } else {
      stm32_lock_release(&__cxa_guard_mutex);
    }
  }
  return 0;
}

void __cxa_guard_abort(CxaGuardObject_t *guard_object)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(guard_object);

  if (guard_object->acquired) {
    guard_object->acquired = 0;
    stm32_lock_release(&__cxa_guard_mutex);
  } else {
    STM32_LOCK_BLOCK();
  }
}

void __cxa_guard_release(CxaGuardObject_t *guard_object)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(guard_object);

  atomic_store(&guard_object->initialized, 1);
  __cxa_guard_abort(guard_object);
}

#endif /* __SINGLE_THREAD__ */
