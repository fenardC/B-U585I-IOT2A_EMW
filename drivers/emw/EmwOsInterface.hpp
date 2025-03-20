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
#define EMW_OS_MINIMAL_THREAD_STACK_SIZE (360U + 128U) /**< BUFSIZE + 128 x 32-bit words at least. */

#elif defined(COMPILATION_WITH_NO_OS)
#define EMW_WITH_NO_OS
#define RTOS_NAME_STRING "NoOS"
#define EMW_OS_TIMEOUT_FOREVER UINT32_MAX
#endif /* COMPILATION_WITH_FREERTOS */


class EmwOsInterface final {
  private:
    EmwOsInterface(void) {};
  public:
    enum /*class*/ Status {
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
    typedef void (*RunnerHook_t)(void *THIS, const void *argumentPtr, std::uint32_t timeoutInMs);

    typedef std::uint32_t ThreadPriority_t;
    typedef struct {
      volatile /*_Atomic*/ std::uint32_t count;
      EmwOsInterface::RunnerHook_t waiterRunner;
      void *waiterRunnerThis;
      const void *waiterRunnerArgumentPtr;
      const char *namePtr;
    } Semaphore_t;

    typedef struct {
      volatile /*_Atomic*/ std::uint32_t count;
      EmwOsInterface::RunnerHook_t reserved;
      void *reservedThis;
      const void *reservedArgumentPtr;
      const char *namePtr;
    } Mutex_t;

    typedef void *Thread_t;
    typedef const void *ThreadFunctionArgument_t;
    typedef struct {
      std::uint32_t elementCountMax;
      std::uint32_t elementCountIn;
      void const * *fifoPtr;
      std::uint32_t readIndex;
      std::uint32_t writeIndex;
      const char *namePtr;
      EmwOsInterface::RunnerHook_t waiterRunner;
      void *waiterRunnerThis;
      const void *waiterRunnerArgumentPtr;
    } Queue_t;
#endif /* COMPILATION_WITH_FREERTOS */

    typedef void (*ThreadFunction_t)(EmwOsInterface::ThreadFunctionArgument_t argument);

  public:
    static void AssertAlways(bool condition) noexcept;
  public:
    static Status CreateSemaphore(EmwOsInterface::Semaphore_t &semaphore, const char *semaphoreNamePtr,
                                  std::uint32_t max_count, std::uint32_t initial_count) noexcept;
  public:
    static Status TakeSemaphore(EmwOsInterface::Semaphore_t &semaphore, std::uint32_t timeoutInMs) noexcept;
  public:
    static Status ReleaseSemaphore(EmwOsInterface::Semaphore_t &semaphore) noexcept;
  public:
    static void DeleteSemaphore(EmwOsInterface::Semaphore_t &semaphore) noexcept;

#if defined(EMW_WITH_NO_OS)
  public:
    static Status AddSemaphoreHook(EmwOsInterface::Semaphore_t &semaphore, EmwOsInterface::RunnerHook_t waiter,
                                   void *THIS,
                                   const class EmwApiCore *corePtr) noexcept;
#endif /* EMW_WITH_NO_OS */

  public:
    static Status CreateMutex(EmwOsInterface::Mutex_t &mutex, const char *mutexNamePtr) noexcept;
  public:
    static Status TakeMutex(EmwOsInterface::Mutex_t &mutex, std::uint32_t timeoutInMs) noexcept;
  public:
    static Status ReleaseMutex(EmwOsInterface::Mutex_t &mutex) noexcept;
  public:
    static void DeleteMutex(EmwOsInterface::Mutex_t &mutex) noexcept;

  public:
    static Status CreateMessageQueue(EmwOsInterface::Queue_t &queue, const char *queueNamePtr,
                                     std::uint32_t messageCount) noexcept;
  public:
    static Status PutMessageQueue(EmwOsInterface::Queue_t &queue, const void *messagePtr,
                                  std::uint32_t timeoutInMs) noexcept;
  public:
    static Status GetMessageQueue(EmwOsInterface::Queue_t &queue, std::uint32_t timeoutInMs,
                                  const void * &messagePtr) noexcept;
  public:
    static void DeleteMessageQueue(EmwOsInterface::Queue_t &queue) noexcept;
#if defined(EMW_WITH_NO_OS)
  public:
    static Status AddMessageQueueHook(EmwOsInterface::Queue_t &queue,
                                      EmwOsInterface::RunnerHook_t waiter, void *THIS, const void *argumentPtr) noexcept;
#endif /* EMW_WITH_NO_OS */
  public:
    static Status CreateThread(EmwOsInterface::Thread_t &thread, const char *threadNamePtr,
                               EmwOsInterface::ThreadFunction_t function, EmwOsInterface::ThreadFunctionArgument_t argument,
                               std::uint32_t stackSize, EmwOsInterface::ThreadPriority_t priority) noexcept;
  public:
    static /*__NO_RETURN*/ /*status_t*/ void ExitThread(void) noexcept;
  public:
    static void TerminateThread(EmwOsInterface::Thread_t &thread) noexcept;

  public:
    static void Delay(std::uint32_t timeoutInMs) noexcept;
  public:
    static void DelayTicks(std::uint32_t count) noexcept;
  public:
    static void *Malloc(std::size_t size) noexcept;
  public:
    static void Free(void *memoryPtr) noexcept;
  public:
    static void Lock(void) noexcept;
  public:
    static void UnLock(void) noexcept;
};

class EmwScopedLock final {
  public:
    inline explicit EmwScopedLock(EmwOsInterface::Mutex_t &mutex)
      : scopedMutexPtr(&mutex)
    {
      EmwOsInterface::TakeMutex(*this->scopedMutexPtr, EMW_OS_TIMEOUT_FOREVER);
    }

  public:
    inline ~EmwScopedLock(void)
    {
      EmwOsInterface::ReleaseMutex(*this->scopedMutexPtr);
    }

  private:
    EmwOsInterface::Mutex_t *scopedMutexPtr;
};
