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
#include "EmwOsInterface.hpp"
#include "emw_conf.hpp"

extern "C" {
#include "cmsis_compiler.h"
}
#include <cstring>
#include <cstdlib>

#define IMPLEMENT_MUTEX_AS_SEMAPHORE

#if !defined(EMW_OS_DEBUG_LOG)
#define EMW_OS_DEBUG_LOG(...)
#endif /* EMW_OS_DEBUG_LOG */

#define IS_IRQ_MODE() (__get_IPSR() != 0U)
#define IS_IRQ() IS_IRQ_MODE()

void EmwOsInterface::AssertAlways(bool condition) noexcept
{
  if (!condition) {
    while (true) {}
  }
}

void EmwOsInterface::Lock(void) noexcept
{
}

void EmwOsInterface::UnLock(void) noexcept
{
}

EmwOsInterface::Status EmwOsInterface::CreateSemaphore(EmwOsInterface::Semaphore_t &semaphore,
    const char *semaphoreNamePtr, std::uint32_t maxCount, std::uint32_t initialCount) noexcept
{
  static_cast<void>(maxCount);

  semaphore.count = initialCount;
  semaphore.waiterRunner = nullptr;
  semaphore.waiterRunnerThis = nullptr;
  semaphore.waiterRunnerArgumentPtr = nullptr;
  semaphore.namePtr = (nullptr != semaphoreNamePtr) ? semaphoreNamePtr : "";
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateSemaphore()< \"%s\"\n", semaphore.namePtr)
  return EmwOsInterface::eOK;
}

EmwOsInterface::Status EmwOsInterface::TakeSemaphore(EmwOsInterface::Semaphore_t &semaphore,
    std::uint32_t timeoutInMs) noexcept
{
  EmwOsInterface::Status status;

  if (!IS_IRQ()) {
    const std::uint32_t start_ms = HAL_GetTick();

    status = EmwOsInterface::eOK;
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::TakeSemaphore()> \"%s\"\n", semaphore.namePtr)

    while (semaphore.count < 1U) {
      const std::uint32_t elapsed_in_ms = HAL_GetTick() - start_ms;
      if (elapsed_in_ms > timeoutInMs) {
        status = EmwOsInterface::eERROR;
        break;
      }
      if (nullptr != semaphore.waiterRunner) {
        EMW_OS_DEBUG_LOG("\n EmwOsInterface::TakeSemaphore()(): RUNNING \"%s\"\n", semaphore.namePtr)

        (*semaphore.waiterRunner)(semaphore.waiterRunnerThis, semaphore.waiterRunnerArgumentPtr,
                                  timeoutInMs - elapsed_in_ms);
      }
    }
    if (EmwOsInterface::eOK == status) {
      semaphore.count = semaphore.count - 1;
    }
  }
  else {
    status = EmwOsInterface::eERROR;
  }
  __DMB();
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::TakeSemaphore()< \"%s\"\n", semaphore.namePtr)
  return status;
}

EmwOsInterface::Status EmwOsInterface::ReleaseSemaphore(EmwOsInterface::Semaphore_t &semaphore) noexcept
{
#if defined(EMW_OS_DEBUG_LOG)
  if (!IS_IRQ()) {
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::ReleaseSemaphore()> \"%s\"\n", semaphore.namePtr)
  }
#endif /* EMW_OS_DEBUG_LOG */

  __DMB();
  semaphore.count = semaphore.count + 1;
  return EmwOsInterface::eOK;
}

void EmwOsInterface::DeleteSemaphore(EmwOsInterface::Semaphore_t &semaphore) noexcept
{
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::DeleteSemaphore()> \"%s\"\n", semaphore.namePtr)

  __DMB();
  semaphore.count = 0U;
  semaphore.waiterRunner = nullptr;
  semaphore.waiterRunnerThis = nullptr;
  semaphore.waiterRunnerArgumentPtr = nullptr;
  semaphore.namePtr = nullptr;
}

EmwOsInterface::Status EmwOsInterface::AddSemaphoreHook(EmwOsInterface::Semaphore_t &semaphore,
    EmwOsInterface::RunnerHook_t waiter, void *THIS, const class EmwApiCore *corePtr) noexcept
{
  semaphore.waiterRunner = waiter;
  semaphore.waiterRunnerThis = THIS;
  semaphore.waiterRunnerArgumentPtr = corePtr;

  EMW_OS_DEBUG_LOG("\n EmwOsInterface::AddSemaphoreHook()< \"%s\"\n", semaphore.namePtr)
  return EmwOsInterface::eOK;
}

EmwOsInterface::Status EmwOsInterface::CreateMutex(EmwOsInterface::Mutex_t &mutex, const char *mutexNamePtr) noexcept
{
  EmwOsInterface::Status status;

#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
  status = EmwOsInterface::CreateSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t &>(mutex), mutexNamePtr, 1, 1);
#else
  mutex.namePtr = (nullptr != mutexNamePtr) ? mutexNamePtr : "";
  status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  return status;
}

EmwOsInterface::Status EmwOsInterface::TakeMutex(EmwOsInterface::Mutex_t &mutex, std::uint32_t timeoutInMs) noexcept
{
  EmwOsInterface::Status status;

#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
  status = EmwOsInterface::TakeSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t &>(mutex), timeoutInMs);
#else
  static_cast<void>(timeoutInMs);
  status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  return status;
}

EmwOsInterface::Status EmwOsInterface::ReleaseMutex(EmwOsInterface::Mutex_t &mutex) noexcept
{
  EmwOsInterface::Status status;

#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
  status = EmwOsInterface::ReleaseSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t &>(mutex));
#else
  status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  return status;
}

void EmwOsInterface::DeleteMutex(EmwOsInterface::Mutex_t &mutex) noexcept
{
#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
  EmwOsInterface::DeleteSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t &>(mutex));
#else
  mutex->namePtr = nullptr;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
}

EmwOsInterface::Status EmwOsInterface::CreateMessageQueue(EmwOsInterface::Queue_t &queue, const char *queueNamePtr,
    std::uint32_t messageCount) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  /* Arbitrary set to 16 in this implementation. */
  if ((messageCount > 0U) && (messageCount < 16U)) {
    queue.elementCountMax = 0U;
    queue.elementCountIn = 0U;
    queue.fifoPtr = nullptr;
    queue.readIndex = 0U;
    queue.writeIndex = 0U;
    queue.waiterRunner = nullptr;
    queue.waiterRunnerThis = nullptr;
    queue.waiterRunnerArgumentPtr = nullptr;
    queue.namePtr = (nullptr != queueNamePtr) ? queueNamePtr : "";
    {
      void const **const fifo_ptr = new void const*[messageCount];

      if (nullptr != fifo_ptr) {
        queue.elementCountMax = messageCount;
        queue.fifoPtr = fifo_ptr;

        for (std::uint32_t i = 0U; i < messageCount; i++) {
          EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateMessageQueue(): FIFO[%" PRIu32 "]: %p -> %p\n",
                           static_cast<std::uint32_t>(i),
                           static_cast<const void *>(&queue.fifoPtr[i]), static_cast<const void *>(&queue.fifoPtr[i]))
        }
        status = EmwOsInterface::eOK;
      }
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::AddMessageQueueHook(EmwOsInterface::Queue_t &queue,
    EmwOsInterface::RunnerHook_t waiter, void *THIS, const void *argumentPtr) noexcept
{
  queue.waiterRunner = waiter;
  queue.waiterRunnerThis = THIS;
  queue.waiterRunnerArgumentPtr = argumentPtr;
  return EmwOsInterface::eOK;
}

EmwOsInterface::Status EmwOsInterface::PutMessageQueue(EmwOsInterface::Queue_t &queue, const void *messagePtr,
    std::uint32_t timeoutInMs) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != messagePtr) {
    const std::uint32_t start_in_ms = HAL_GetTick();

    status = EmwOsInterface::eOK;
    while (queue.elementCountIn == queue.elementCountMax) {
      const std::uint32_t elapsed_in_ms = HAL_GetTick() - start_in_ms;
      if (elapsed_in_ms > timeoutInMs) {
        status = EmwOsInterface::eERROR;
        break;
      }
    }
    if (EmwOsInterface::eOK == status) {
      EMW_OS_DEBUG_LOG("\n EmwOsInterface::PutMessageQueue(): pushing %p @ %p\n",
                       messagePtr, static_cast<const void *>(&queue.fifoPtr[queue.writeIndex]))
      queue.fifoPtr[queue.writeIndex] = messagePtr;
      queue.elementCountIn++;
      queue.writeIndex = (queue.writeIndex + 1U) % queue.elementCountMax;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::GetMessageQueue(EmwOsInterface::Queue_t &queue, std::uint32_t timeoutInMs,
    const void *&messagePtr) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  const std::uint32_t start_ms = HAL_GetTick();

  messagePtr = nullptr;
  status = EmwOsInterface::eOK;

  while (0U == queue.elementCountIn) {
    if ((HAL_GetTick() - start_ms) > timeoutInMs) {
      status = EmwOsInterface::eERROR;
      break;
    }
    if (nullptr != queue.waiterRunner) {
      EMW_OS_DEBUG_LOG("\n EmwOsInterface::GetMessageQueue(): RUNNING \"%s\"\n", queue.namePtr)
      (*queue.waiterRunner)(queue.waiterRunnerThis, queue.waiterRunnerArgumentPtr,
                            timeoutInMs - (HAL_GetTick() - start_ms));
    }
  }
  if (EmwOsInterface::eOK == status) {
    messagePtr = queue.fifoPtr[queue.readIndex];
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::GetMessageQueue(): popped  %p @ %p\n",
                     static_cast<const void *>(messagePtr), static_cast<void *>(&queue.fifoPtr[queue.readIndex]))
    queue.elementCountIn--;
    queue.readIndex = (queue.readIndex + 1U) % queue.elementCountMax;
  }
  return status;
}

void EmwOsInterface::DeleteMessageQueue(EmwOsInterface::Queue_t &queue) noexcept
{
  delete[](queue.fifoPtr);
  queue.elementCountMax = 0U;
  queue.elementCountIn = 0U;
  queue.fifoPtr = nullptr;
  queue.readIndex = 0U;
  queue.writeIndex = 0U;
  queue.waiterRunner = nullptr;
  queue.waiterRunnerThis = nullptr;
  queue.waiterRunnerArgumentPtr = nullptr;
  queue.namePtr = nullptr;
}

EmwOsInterface::Status EmwOsInterface::CreateThread(EmwOsInterface::Thread_t &thread, const char *threadNamePtr,
    EmwOsInterface::ThreadFunction_t function, EmwOsInterface::ThreadFunctionArgument_t argument,
    std::uint32_t stackSize, EmwOsInterface::ThreadPriority_t priority) noexcept
{
  static_cast<void>(thread);
  static_cast<void>(threadNamePtr);
  static_cast<void>(function);
  static_cast<void>(argument);
  static_cast<void>(stackSize);
  static_cast<void>(priority);

  return EmwOsInterface::eOK;
}

/*__NO_RETURN*/ void EmwOsInterface::ExitThread(void) noexcept
{
  return;
}

void EmwOsInterface::TerminateThread(EmwOsInterface::Thread_t &thread) noexcept
{
  static_cast<void>(thread);
}

void EmwOsInterface::Delay(std::uint32_t timeoutInMs) noexcept
{
  HAL_Delay(timeoutInMs);
}

void EmwOsInterface::DelayTicks(std::uint32_t count) noexcept
{
  HAL_Delay(count);
}

void *EmwOsInterface::Malloc(std::size_t size) noexcept
{
  auto const memory_ptr = new std::uint8_t[size];
  EmwOsInterface::AssertAlways(nullptr != memory_ptr);

  EMW_OS_DEBUG_LOG("EmwOsInterface::Malloc(): %p (%" PRIu32 ")\n", memory_ptr, static_cast<std::uint32_t>(size))
  return static_cast<void *>(memory_ptr);
}

void EmwOsInterface::Free(void *memoryPtr) noexcept
{
  EMW_OS_DEBUG_LOG("EmwOsInterface::Free()  : %p\n", memoryPtr)

  delete[] static_cast<std::uint8_t *>(memoryPtr);
}
