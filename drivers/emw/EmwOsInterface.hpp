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

#include <cstdbool>
#include <cstddef>
#include <cstdint>

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


class EmwOsInterface final {
  private:
    EmwOsInterface(void) {};
  public:
    enum Status {
      eOK = 0x00U,
      eERROR = 0x01U
    };

#if defined(COMPILATION_WITH_FREERTOS)
    typedef UBaseType_t ThreadPriority_t;
    typedef QueueHandle_t Semaphore_t;
    typedef QueueHandle_t Mutex_t;
    typedef TaskHandle_t Thread_t;
    typedef const void *ThreadFunctionArgument_t;
    typedef QueueHandle_t Queue_t;

#elif defined(COMPILATION_WITH_NO_OS)
    typedef void (*RunnerHook_t)(void *THIS, const class EmwApiCore *corePtr, uint32_t timeoutInMs);

    typedef uint32_t ThreadPriority_t;
    typedef struct {
      volatile /*_Atomic*/ uint32_t count;
      EmwOsInterface::RunnerHook_t waiterRunner;
      void *waiterRunnerThis;
      const class EmwApiCore *waiterRunnerArgumentPtr;
      const char *namePtr;
    } Semaphore_t;

    typedef struct {
      volatile /*_Atomic*/ uint32_t count;
      EmwOsInterface::RunnerHook_t reserved;
      void *reservedThis;
      void *reservedArgumentPtr;
      const char *namePtr;
    } Mutex_t;

    typedef void *Thread_t;
    typedef void *ThreadFunctionArgument_t;
    typedef struct {
      uint32_t elementCountMax;
      uint32_t elementCountIn;
      void const * *fifoPtr;
      uint32_t readIndex;
      uint32_t writeIndex;
      const char *namePtr;
      EmwOsInterface::RunnerHook_t waiterRunner;
      void *waiterRunnerThis;
      const class EmwApiCore *waiterRunnerArgumentPtr;
    } Queue_t;
#endif /* COMPILATION_WITH_FREERTOS */

    typedef void (*ThreadFunction_t)(EmwOsInterface::ThreadFunctionArgument_t argument);

  public:
    static void assertAlways(bool condition);
  public:
    static Status createSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr, const char *semaphoreNamePtr,
                                  uint32_t max_count, uint32_t initial_count);
  public:
    static Status takeSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr, uint32_t timeoutInMs);
  public:
    static Status releaseSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr);
  public:
    static Status deleteSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr);

#if defined(EMW_WITH_NO_OS)
  public:
    static Status addSemaphoreHook(EmwOsInterface::Semaphore_t *semaphorePtr, EmwOsInterface::RunnerHook_t waiter,
                                   void *THIS,
                                   const class EmwApiCore *corePtr);
#endif /* EMW_WITH_NO_OS */

  public:
    static Status createMutex(EmwOsInterface::Mutex_t *mutexPtr, const char *mutexNamePtr);
  public:
    static Status takeMutex(EmwOsInterface::Mutex_t *mutexPtr, uint32_t timeoutInMs);
  public:
    static Status releaseMutex(EmwOsInterface::Mutex_t *mutexPtr);
  public:
    static Status deleteMutex(EmwOsInterface::Mutex_t *mutexPtr);

  public:
    static Status createMessageQueue(EmwOsInterface::Queue_t *queuePtr, const char *queueNamePtr, uint32_t messageCount);
  public:
    static Status putMessageQueue(EmwOsInterface::Queue_t *queuePtr, const void *messagePtr, uint32_t timeoutInMs);
  public:
    static Status getMessageQueue(EmwOsInterface::Queue_t *queuePtr, uint32_t timeoutInMs, const void * *messagePtrPtr);
  public:
    static Status deleteMessageQueue(EmwOsInterface::Queue_t *queuePtr);
#if defined(EMW_WITH_NO_OS)
  public:
    static Status addMessageQueueHook(EmwOsInterface::Queue_t *queuePtr, EmwOsInterface::RunnerHook_t waiter, void *THIS,
                                      const class EmwApiCore *corePtr);
#endif /* EMW_WITH_NO_OS */
  public:
    static Status createThread(EmwOsInterface::Thread_t *threadPtr, const char *threadNamePtr,
                               EmwOsInterface::ThreadFunction_t function, EmwOsInterface::ThreadFunctionArgument_t argument,
                               uint32_t stackSize, EmwOsInterface::ThreadPriority_t priority);
  public:
    static /*__NO_RETURN*/ /*status_t*/ void exitThread(void);
  public:
    static Status terminateThread(EmwOsInterface::Thread_t *threadPtr);

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
    inline explicit EmwScopedLock(EmwOsInterface::Mutex_t *mutexPtr)
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
    EmwOsInterface::Mutex_t *scopedMutexPtr;
};
