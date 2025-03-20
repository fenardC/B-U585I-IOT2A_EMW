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
#include "stm32u5xx_hal.h"

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

void EmwOsInterface::assertAlways(bool condition)
{
  if (!condition) {
    while (true) {}
  }
}

void EmwOsInterface::lock(void)
{
}

void EmwOsInterface::unLock(void)
{
}

EmwOsInterface::Status EmwOsInterface::createSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr, const char *semaphoreNamePtr,
    uint32_t maxCount, uint32_t initialCount)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  static_cast<void>(maxCount);

  if (nullptr != semaphorePtr) {
    semaphorePtr->count = initialCount;
    semaphorePtr->waiterRunner = nullptr;
    semaphorePtr->waiterRunnerThis = nullptr;
    semaphorePtr->waiterRunnerArgumentPtr = nullptr;
    semaphorePtr->namePtr = (nullptr != semaphoreNamePtr) ? semaphoreNamePtr : "";
    status = EmwOsInterface::eOK;
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::createSemaphore()< \"%s\"\n", semaphorePtr->namePtr)
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::takeSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr, uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
    const uint32_t start_ms = HAL_GetTick();

    if (!IS_IRQ()) {
      status = EmwOsInterface::eOK;

      EMW_OS_DEBUG_LOG("\n EmwOsInterface::takeSemaphore()> \"%s\"\n", semaphorePtr->namePtr)
      while (semaphorePtr->count < 1U) {
        const uint32_t elapsed_in_ms = HAL_GetTick() - start_ms;
        if (elapsed_in_ms > timeoutInMs) {
          status = EmwOsInterface::eERROR;
          break;
        }
        if (nullptr != semaphorePtr->waiterRunner) {
          EMW_OS_DEBUG_LOG("\n EmwOsInterface::takeSemaphore()(): RUNNING \"%s\"\n", semaphorePtr->namePtr)

          (*semaphorePtr->waiterRunner)(semaphorePtr->waiterRunnerThis, semaphorePtr->waiterRunnerArgumentPtr,
                                        timeoutInMs - elapsed_in_ms);
        }
      }
      if (EmwOsInterface::eOK == status) {
        semaphorePtr->count--;
      }
    }
    __DMB();
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::takeSemaphore()< \"%s\"\n", semaphorePtr->namePtr)
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::releaseSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
#if defined(EMW_OS_DEBUG_LOG)
    if (!IS_IRQ()) {
      EMW_OS_DEBUG_LOG("\n EmwOsInterface::releaseSemaphore()> \"%s\"\n", semaphorePtr->namePtr)
    }
#endif /* EMW_OS_DEBUG_LOG */

    __DMB();
    semaphorePtr->count++;
    status = EmwOsInterface::eOK;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::deleteSemaphore()> \"%s\"\n", semaphorePtr->namePtr)
    __DMB();
    semaphorePtr->count = 0U;
    semaphorePtr->waiterRunner = nullptr;
    semaphorePtr->waiterRunnerThis = nullptr;
    semaphorePtr->waiterRunnerArgumentPtr = nullptr;
    semaphorePtr->namePtr = nullptr;
    status = EmwOsInterface::eOK;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::addSemaphoreHook(EmwOsInterface::Semaphore_t *semaphorePtr,
	    EmwOsInterface::RunnerHook_t waiter, void *THIS, const class EmwApiCore *corePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
    semaphorePtr->waiterRunner = waiter;
    semaphorePtr->waiterRunnerThis = THIS;
    semaphorePtr->waiterRunnerArgumentPtr = corePtr;

    status = EmwOsInterface::eOK;
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::addSemaphoreHook()< \"%s\"\n", semaphorePtr->namePtr)
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::createMutex(EmwOsInterface::Mutex_t *mutexPtr, const char *mutexNamePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
    status = EmwOsInterface::createSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t *>(mutexPtr), mutexNamePtr, 1, 1);
#else
    mutexPtr->namePtr = (nullptr != mutexNamePtr) ? mutexNamePtr : "";
    status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::takeMutex(EmwOsInterface::Mutex_t *mutexPtr, uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
    status = EmwOsInterface::takeSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t *>(mutexPtr), timeoutInMs);
#else
    (void) timeoutInMs;
    status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::releaseMutex(EmwOsInterface::Mutex_t *mutexPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
    status = EmwOsInterface::releaseSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t *>(mutexPtr));
#else
    status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteMutex(EmwOsInterface::Mutex_t *mutexPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
#if defined(IMPLEMENT_MUTEX_AS_SEMAPHORE)
    status = EmwOsInterface::deleteSemaphore(reinterpret_cast<EmwOsInterface::Semaphore_t *>(mutexPtr));
#else
    mutexPtr->namePtr = nullptr;
    status = EmwOsInterface::eOK;
#endif /* IMPLEMENT_MUTEX_AS_SEMAPHORE */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::createMessageQueue(EmwOsInterface::Queue_t *queuePtr, const char *queueNamePtr,
    uint32_t messageCount)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  /* Arbitrary set to 16 in this implementation. */
  if ((nullptr != queuePtr) && (messageCount > 0U) && (messageCount < 16U)) {
    queuePtr->elementCountMax = 0U;
    queuePtr->elementCountIn = 0U;
    queuePtr->fifoPtr = nullptr;
    queuePtr->readIndex = 0U;
    queuePtr->writeIndex = 0U;
    queuePtr->waiterRunner = nullptr;
    queuePtr->waiterRunnerThis = nullptr;
    queuePtr->waiterRunnerArgumentPtr = nullptr;
    queuePtr->namePtr = (nullptr != queueNamePtr) ? queueNamePtr : "";
    {
      void const **const fifo_ptr = new void const*[messageCount];
      if (nullptr != fifo_ptr) {
        queuePtr->elementCountMax = messageCount;
        queuePtr->fifoPtr = fifo_ptr;

        for (uint32_t i = 0; i < messageCount; i++) {
          EMW_OS_DEBUG_LOG("\n EmwOsInterface::createMessageQueue(): FIFO[%" PRIu32 "]: %p -> %p\n",
                           (uint32_t)i, (void *)&queuePtr->fifoPtr[i], (void *)&queuePtr->fifoPtr[i])
        }
        status = EmwOsInterface::eOK;
      }
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::addMessageQueueHook(EmwOsInterface::Queue_t *queuePtr,
    EmwOsInterface::RunnerHook_t waiter, void *THIS, const class EmwApiCore *corePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != queuePtr) {
    queuePtr->waiterRunner = waiter;
    queuePtr->waiterRunnerThis = THIS;
    queuePtr->waiterRunnerArgumentPtr = corePtr;
    status = EmwOsInterface::eOK;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::putMessageQueue(EmwOsInterface::Queue_t *queuePtr, const void *messagePtr,
    uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if ((nullptr != queuePtr) && (nullptr != messagePtr)) {
    const uint32_t start_in_ms = HAL_GetTick();
    status = EmwOsInterface::eOK;
    while (queuePtr->elementCountIn == queuePtr->elementCountMax) {
      const uint32_t elapsed_in_ms = HAL_GetTick() - start_in_ms;
      if (elapsed_in_ms > timeoutInMs) {
        status = EmwOsInterface::eERROR;
        break;
      }
    }
    if (EmwOsInterface::eOK == status) {
      EMW_OS_DEBUG_LOG("\n EmwOsInterface::putMessageQueue(): pushing %p @ %p\n",
                       messagePtr, (void *)&queuePtr->fifoPtr[queuePtr->writeIndex])
      queuePtr->fifoPtr[queuePtr->writeIndex] = messagePtr;
      queuePtr->elementCountIn++;
      queuePtr->writeIndex = (queuePtr->writeIndex + 1U) % queuePtr->elementCountMax;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::getMessageQueue(EmwOsInterface::Queue_t *queuePtr, uint32_t timeoutInMs,
    const void **messagePtrPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if ((nullptr != queuePtr) && (nullptr != messagePtrPtr)) {
    const uint32_t start_ms = HAL_GetTick();

    *messagePtrPtr = nullptr;
    status = EmwOsInterface::eOK;

    while (0U == queuePtr->elementCountIn) {
      if ((HAL_GetTick() - start_ms) > timeoutInMs) {
        status = EmwOsInterface::eERROR;
        break;
      }
      if (nullptr != queuePtr->waiterRunner) {
        EMW_OS_DEBUG_LOG("\n EmwOsInterface::getMessageQueue(): RUNNING \"%s\"\n", queuePtr->namePtr)
        (*queuePtr->waiterRunner)(queuePtr->waiterRunnerThis, queuePtr->waiterRunnerArgumentPtr,
                                  timeoutInMs - (HAL_GetTick() - start_ms));
      }
    }
    if (EmwOsInterface::eOK == status) {
      *messagePtrPtr = queuePtr->fifoPtr[queuePtr->readIndex];
      EMW_OS_DEBUG_LOG("\n EmwOsInterface::getMessageQueue(): popped %p @ %p\n",
                       (void *)*messagePtrPtr, (void *)&queuePtr->fifoPtr[queuePtr->readIndex])
      queuePtr->elementCountIn--;
      queuePtr->readIndex = (queuePtr->readIndex + 1U) % queuePtr->elementCountMax;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteMessageQueue(EmwOsInterface::Queue_t *queuePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != queuePtr) {
    delete[](queuePtr->fifoPtr);
    queuePtr->elementCountMax = 0U;
    queuePtr->elementCountIn = 0U;
    queuePtr->fifoPtr = nullptr;
    queuePtr->readIndex = 0U;
    queuePtr->writeIndex = 0U;
    queuePtr->waiterRunner = nullptr;
    queuePtr->waiterRunnerThis = nullptr;
    queuePtr->waiterRunnerArgumentPtr = nullptr;
    queuePtr->namePtr = nullptr;
    status = EmwOsInterface::eOK;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::createThread(EmwOsInterface::Thread_t *threadPtr, const char *threadNamePtr,
    EmwOsInterface::ThreadFunction_t function, EmwOsInterface::ThreadFunctionArgument_t argument,
    uint32_t stackSize, EmwOsInterface::ThreadPriority_t priority)
{
  static_cast<void>(threadPtr);
  static_cast<void>(threadNamePtr);
  static_cast<void>(function);
  static_cast<void>(argument);
  static_cast<void>(stackSize);
  static_cast<void>(priority);
  return EmwOsInterface::eOK;
}

/*__NO_RETURN*/ void EmwOsInterface::exitThread(void)
{
  return;
}

EmwOsInterface::Status EmwOsInterface::terminateThread(EmwOsInterface::Thread_t *threadPtr)
{
  static_cast<void>(threadPtr);
  return EmwOsInterface::eOK;
}

EmwOsInterface::Status EmwOsInterface::delay(uint32_t timeoutInMs)
{
  HAL_Delay(timeoutInMs);
  return EmwOsInterface::eOK;
}

EmwOsInterface::Status EmwOsInterface::delayTicks(uint32_t count)
{
  HAL_Delay(count);
  return EmwOsInterface::eOK;
}

void *EmwOsInterface::malloc(size_t size)
{
  auto const memory_ptr = new uint8_t[size];
  EmwOsInterface::assertAlways(nullptr != memory_ptr);
  EMW_OS_DEBUG_LOG("EmwOsInterface::malloc(): %p (%" PRIu32 ")\n", memory_ptr, static_cast<uint32_t>(size))
  return static_cast<void *>(memory_ptr);
}

void EmwOsInterface::free(void *memoryPtr)
{
  EMW_OS_DEBUG_LOG("EmwOsInterface::free()  : %p\n", memoryPtr)
  delete[]static_cast<uint8_t *>(memoryPtr);
}
