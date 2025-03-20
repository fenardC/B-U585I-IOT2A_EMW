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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "cmsis_compiler.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(COMPILATION_WITH_FREERTOS)
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#endif /* COMPILATION_WITH_FREERTOS */

#if defined(COMPILATION_WITH_FREERTOS)
#define EMW_WITH_RTOS
#define RTOS_NAME_STRING "FreeRTOS" "(" tskKERNEL_VERSION_NUMBER  ")"
#define EMW_OS_TIMEOUT_FOREVER UINT32_MAX
#define EMW_OS_MINIMAL_THREAD_STACK_SIZE (128U) /**< 128 x 32-bit words at least.*/

#elif defined(COMPILATION_WITH_NO_OS)
#define EMW_WITH_NO_OS
#define RTOS_NAME_STRING "NoOS"
#define EMW_OS_TIMEOUT_FOREVER UINT32_MAX
#endif /* COMPILATION_WITH_FREERTOS */

typedef void (*runner_hook_t)(void *THIS, const class EmwApiCore *corePtr, uint32_t timeoutInMs);

class EmwOsInterface final {
  private:
    EmwOsInterface(void) {};
  public:
    enum Status {
      eOK = 0x00U,
      eERROR = 0x01U
    };

#if defined(COMPILATION_WITH_FREERTOS)
    typedef UBaseType_t thread_priority_t;
    typedef QueueHandle_t semaphore_t;
    typedef QueueHandle_t mutex_t;
    typedef TaskHandle_t thread_t;
    typedef const void *thread_function_argument;
    typedef QueueHandle_t queue_t;

#elif defined(COMPILATION_WITH_NO_OS)
    typedef uint32_t thread_priority_t;
    typedef struct {
      volatile /*_Atomic*/ uint32_t count;
      runner_hook_t waiterRunner;
      void *waiterRunnerThis;
      const class EmwApiCore *waiterRunnerArgumentPtr;
      const char *namePtr;
    } semaphore_t;

    typedef struct {
      volatile /*_Atomic*/ uint32_t count;
      runner_hook_t reserved;
      void *reservedThis;
      void *reservedArgumentPtr;
      const char *namePtr;
    } mutex_t;

    typedef void *thread_t;
    typedef void *thread_function_argument;
    typedef struct {
      uint32_t elementCountMax;
      uint32_t elementCountIn;
      void const * *fifoPtr;
      uint32_t readIndex;
      uint32_t writeIndex;
      const char *namePtr;
      runner_hook_t waiterRunner;
      void *waiterRunnerThis;
      const class EmwApiCore *waiterRunnerArgumentPtr;
    } queue_t;
#endif /* COMPILATION_WITH_FREERTOS */

    typedef void (*thread_function_t)(EmwOsInterface::thread_function_argument argument);

  public:
    static void assertAlways(bool condition);
  public:
    static Status createSemaphore(semaphore_t *semaphorePtr, const char *semaphoreNamePtr,
                                  uint32_t max_count, uint32_t initial_count);
  public:
    static Status takeSemaphore(semaphore_t *semaphorePtr, uint32_t timeoutInMs);
  public:
    static Status releaseSemaphore(semaphore_t *semaphorePtr);
  public:
    static Status deleteSemaphore(semaphore_t *semaphorePtr);

#if defined(EMW_WITH_NO_OS)
  public:
    static Status addSemaphoreHook(semaphore_t *semaphorePtr, runner_hook_t waiter, void *THIS,
                                   const class EmwApiCore *corePtr);
#endif /* EMW_WITH_NO_OS */

  public:
    static Status createMutex(mutex_t *mutexPtr, const char *mutexNamePtr);
  public:
    static Status takeMutex(mutex_t *mutexPtr, uint32_t timeoutInMs);
  public:
    static Status releaseMutex(mutex_t *mutexPtr);
  public:
    static Status deleteMutex(mutex_t *mutexPtr);

  public:
    static Status createMessageQueue(queue_t *queuePtr, const char *p_queue_name, uint32_t message_count);
  public:
    static Status putMessageQueue(queue_t *queuePtr, const void *messagePtr, uint32_t timeoutInMs);
  public:
    static Status getMessageQueue(queue_t *queuePtr, uint32_t timeoutInMs, const void * *messagePtrPtr);
  public:
    static Status deleteMessageQueue(queue_t *queuePtr);
#if defined(EMW_WITH_NO_OS)
  public:
    static Status addMessageQueueHook(queue_t *queuePtr, runner_hook_t waiter, void *THIS,
                                      const class EmwApiCore *corePtr);
#endif /* EMW_WITH_NO_OS */
  public:
    static Status createThread(thread_t *threadPtr, const char *threadNamePtr,
                               thread_function_t function, thread_function_argument argument,
                               uint32_t stackSize, thread_priority_t priority);
  public:
    static /*__NO_RETURN*/ /*status_t*/ void exitThread(void);
  public:
    static Status terminateThread(thread_t *threadPtr);

  public:
    static Status delay(uint32_t timeoutInMs);
  public:
    static Status delayTicks(uint32_t count);
  public:
    static void *malloc(size_t size);
  public:
    static void free(void *memoryPtr);
  public:
    static void lock(void);
  public:
    static void unLock(void);
};

class EmwScopedLock final {
  public:
    inline explicit EmwScopedLock(EmwOsInterface::mutex_t *mutexPtr)
      : scopedMutexPtr(mutexPtr)
    {
      EmwOsInterface::takeMutex(this->scopedMutexPtr, EMW_OS_TIMEOUT_FOREVER);
    }

  public:
    inline ~EmwScopedLock(void)
    {
      EmwOsInterface::releaseMutex(this->scopedMutexPtr);
    }

  private:
    EmwOsInterface::mutex_t *scopedMutexPtr;
};
