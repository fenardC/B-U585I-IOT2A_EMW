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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "cmsis_compiler.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */
#include "semphr.h"

#if !defined(EMW_OS_DEBUG_LOG)
#define EMW_OS_DEBUG_LOG(...)
#endif /* EMW_OS_DEBUG_LOG */

static const BaseType_t FALSE = 0; /* pdFALSE */
static const BaseType_t PASS = 1; /* pdPASS */
static const BaseType_t TRUE = 1; /* pdTRUE */
static const TickType_t GIVE_BLOCK_TIME = 0U; /* semGIVE_BLOCK_TIME */
static const uint8_t QUEUE_TYPE_BASE = 0U; /* queueQUEUE_TYPE_BASE */
static const uint8_t QUEUE_TYPE_RECURSIVE_MUTEX = 4U; /* queueQUEUE_TYPE_RECURSIVE_MUTEX */
static const BaseType_t SEND_TO_BACK = 0; /* queueSEND_TO_BACK */

static uint32_t MilliSecondsToTicks(uint32_t timeoutInMs);

void EmwOsInterface::assertAlways(bool condition)
{
  if (!condition) {
    while (true) {}
  }
}

void EmwOsInterface::lock(void)
{
  vTaskSuspendAll();
}

void EmwOsInterface::unLock(void)
{
  (void) xTaskResumeAll();
}

EmwOsInterface::Status EmwOsInterface::createSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr,
    const char *semaphoreNamePtr,
    uint32_t maxCount, uint32_t initialCount)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    const QueueHandle_t queue = xQueueCreateCountingSemaphore(maxCount, initialCount);
    if (nullptr != queue) {
      *semaphorePtr = static_cast<EmwOsInterface::Semaphore_t>(queue);
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueAddToRegistry(queue, (nullptr != semaphoreNamePtr) ? semaphoreNamePtr : "");
#endif /* configQUEUE_REGISTRY_SIZE */
      status = EmwOsInterface::eOK;
    }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::takeSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr, uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*semaphorePtr);

    if (nullptr != queue) {
      if (PASS == xQueueSemaphoreTake(queue, MilliSecondsToTicks(timeoutInMs))) {
        status = EmwOsInterface::eOK;
      }
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::releaseSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != semaphorePtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*semaphorePtr);

    if (nullptr != queue) {
      if (TRUE == xPortIsInsideInterrupt()) {
        BaseType_t yield = FALSE;

        if (TRUE == xQueueGiveFromISR(queue, &yield)) {
          if (TRUE == yield) {
            taskYIELD();
          }
          status = EmwOsInterface::eOK;
        }
      }
      else {
        if (PASS == xQueueGenericSend(queue, nullptr, GIVE_BLOCK_TIME, SEND_TO_BACK)) {
          status = EmwOsInterface::eOK;
        }
      }
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteSemaphore(EmwOsInterface::Semaphore_t *semaphorePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != semaphorePtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*semaphorePtr);

    if (nullptr != queue) {
#if ( configQUEUE_REGISTRY_SIZE > 0 )
      vQueueUnregisterQueue(queue);
#endif /* configQUEUE_REGISTRY_SIZE */
      vQueueDelete(queue);
      status = EmwOsInterface::eOK;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::createMutex(EmwOsInterface::Mutex_t *mutexPtr, const char *mutexNamePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
#if ((configSUPPORT_DYNAMIC_ALLOCATION == 1) && (configUSE_RECURSIVE_MUTEXES == 1))
    const QueueHandle_t queue = xQueueCreateMutex(QUEUE_TYPE_RECURSIVE_MUTEX);

    if (nullptr != queue) {
      *mutexPtr = static_cast<EmwOsInterface::Mutex_t>(queue);
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueAddToRegistry(queue, (nullptr != mutexNamePtr) ? mutexNamePtr : "");
#endif /*configQUEUE_REGISTRY_SIZE*/
      status = EmwOsInterface::eOK;
    }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION && configUSE_RECURSIVE_MUTEXES */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::takeMutex(EmwOsInterface::Mutex_t *mutexPtr, uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*mutexPtr);

    if (nullptr != queue) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
      if (PASS == xQueueTakeMutexRecursive(queue, MilliSecondsToTicks(timeoutInMs))) {
        status = EmwOsInterface::eOK;
      }
#endif /* configUSE_RECURSIVE_MUTEXES */
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::releaseMutex(EmwOsInterface::Mutex_t *mutexPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*mutexPtr);

    if (nullptr != queue) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
      if (PASS == xQueueGiveMutexRecursive(queue)) {
        status = EmwOsInterface::eOK;
      }
#endif /* configUSE_RECURSIVE_MUTEXES */
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteMutex(EmwOsInterface::Mutex_t *mutexPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != mutexPtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*mutexPtr);

    if (nullptr != queue) {
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueUnregisterQueue(queue);
#endif /* configQUEUE_REGISTRY_SIZE */
      vQueueDelete(queue);
      status = EmwOsInterface::eOK;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::createMessageQueue(EmwOsInterface::Queue_t *queuePtr, const char *queueNamePtr,
    uint32_t messageCount)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != queuePtr) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    const QueueHandle_t queue = xQueueGenericCreate(messageCount, sizeof(void *), QUEUE_TYPE_BASE);

    if (nullptr != queue) {
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueAddToRegistry(queue, (nullptr != queueNamePtr) ? queueNamePtr : "");
#endif /* configQUEUE_REGISTRY_SIZE*/
      *queuePtr = static_cast<Queue_t>(queue);
      EMW_OS_DEBUG_LOG("\nEmwOsInterface::createMessageQueue(): queue %p\n", static_cast<void *>(queue))
      status = EmwOsInterface::eOK;
    }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::putMessageQueue(EmwOsInterface::Queue_t *queuePtr, const void *messagePtr,
    uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if ((nullptr != queuePtr) && (nullptr != messagePtr)) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*queuePtr);
    const void *const item = messagePtr;

    EMW_OS_DEBUG_LOG("\n EmwOsInterface::putMessageQueue(): pushing %p\n", messagePtr)

    if (PASS == xQueueGenericSend(queue, &item, MilliSecondsToTicks(timeoutInMs), SEND_TO_BACK)) {
      status = EmwOsInterface::eOK;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::getMessageQueue(EmwOsInterface::Queue_t *queuePtr, uint32_t timeoutInMs,
    const void **messagePtrPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if ((nullptr != queuePtr) && (nullptr != messagePtrPtr)) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*queuePtr);
    void *message_ptr = nullptr;

    *messagePtrPtr = nullptr;
    if (TRUE == xQueueReceive(queue, &message_ptr, MilliSecondsToTicks(timeoutInMs))) {
      *messagePtrPtr = message_ptr;
      status = EmwOsInterface::eOK;
      EMW_OS_DEBUG_LOG("\nEmwOsInterface::getMessageQueue: popped %p\n", (void *)*messagePtrPtr)
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteMessageQueue(EmwOsInterface::Queue_t *queuePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != queuePtr) {
    const QueueHandle_t queue = static_cast<QueueHandle_t>(*queuePtr);

    if (nullptr != queue) {
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueUnregisterQueue(queue);
#endif /* configQUEUE_REGISTRY_SIZE */
      vQueueDelete(queue);
      status = EmwOsInterface::eOK;
    }
    EMW_OS_DEBUG_LOG("\nEmwOsInterface::deleteMessageQueue(): queue %p\n", static_cast<void *>(queue))
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::createThread(EmwOsInterface::Thread_t *threadPtr, const char *threadNamePtr,
    EmwOsInterface::ThreadFunction_t function, EmwOsInterface::ThreadFunctionArgument_t argument,
    uint32_t stackSize, EmwOsInterface::ThreadPriority_t priority)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if ((nullptr != threadPtr) && (EMW_OS_MINIMAL_THREAD_STACK_SIZE <= stackSize)) {
    const configSTACK_DEPTH_TYPE stack = static_cast<uint16_t>(stackSize); /* In number of 32-bits words. */
    TaskHandle_t thread = nullptr;

    if (PASS == xTaskCreate(reinterpret_cast<TaskFunction_t>(function), (nullptr != threadNamePtr) ? threadNamePtr : "",
                            stack, const_cast<void *>(argument), static_cast<UBaseType_t>(priority), &thread)) {
      if (nullptr != thread) {
        *threadPtr = static_cast<EmwOsInterface::Thread_t>(thread);
        status = EmwOsInterface::eOK;
      }
    }
    else {
      EmwOsInterface::assertAlways(false);
    }
  }
  return status;
}

__NO_RETURN void EmwOsInterface::exitThread(void)
{
  vTaskDelete(nullptr);
  for (;;) {;}
}

EmwOsInterface::Status EmwOsInterface::terminateThread(EmwOsInterface::Thread_t *threadPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != threadPtr) {
    /* Don't call vTaskDelete() */
    *threadPtr = nullptr;
    status = EmwOsInterface::eOK;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::delay(uint32_t timeoutInMs)
{
  vTaskDelay(MilliSecondsToTicks(timeoutInMs));
  return EmwOsInterface::eOK;
}

EmwOsInterface::Status EmwOsInterface::delayTicks(uint32_t count)
{
  vTaskDelay(count);
  return EmwOsInterface::eOK;
}

static uint32_t MilliSecondsToTicks(uint32_t timeoutInMs)
{
  uint64_t ticks = 0U;
  if (timeoutInMs == EMW_OS_TIMEOUT_FOREVER) {
    ticks = 0xFFFFFFFFU;
  }
  if (timeoutInMs != 0U) {
    const uint64_t tick_freq = configTICK_RATE_HZ;
    ticks = (static_cast<uint64_t>(timeoutInMs) * tick_freq) / 1000;
    if (ticks == 0U) {
      ticks = 1U;
    }
    if (ticks >= UINT32_MAX) {
      ticks = UINT32_MAX - 1U;
    }
  }
  return static_cast<uint32_t>(ticks);
}

void *EmwOsInterface::malloc(size_t size)
{
  void *const memory_ptr = pvPortMalloc(size);
  EmwOsInterface::assertAlways(nullptr != memory_ptr);
  EMW_OS_DEBUG_LOG("EmwOsInterface::malloc(): %p (%" PRIu32 ")\n", memory_ptr, (uint32_t)size)
  return memory_ptr;
}

void EmwOsInterface::free(void *memoryPtr)
{
  EMW_OS_DEBUG_LOG("EmwOsInterface::free()  : %p\n", memoryPtr)
  vPortFree(memoryPtr);
}
