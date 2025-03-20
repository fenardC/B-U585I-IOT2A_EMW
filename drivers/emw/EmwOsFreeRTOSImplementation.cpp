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

EmwOsInterface::Status EmwOsInterface::createSemaphore(semaphore_t *semaphorePtr, const char *semaphoreNamePtr,
    uint32_t maxCount, uint32_t initialCount)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != semaphorePtr) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    const QueueHandle_t queue = xQueueCreateCountingSemaphore(maxCount, initialCount);
    if (nullptr != queue) {
      *semaphorePtr = (semaphore_t)queue;
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueAddToRegistry(queue, (nullptr != semaphoreNamePtr) ? semaphoreNamePtr : "");
#endif /* configQUEUE_REGISTRY_SIZE */
      status = EmwOsInterface::eOK;
    }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::takeSemaphore(semaphore_t *semaphorePtr, uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != semaphorePtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*semaphorePtr);
    if (nullptr != queue) {
      if (pdPASS == xQueueSemaphoreTake(queue, MilliSecondsToTicks(timeoutInMs))) {
        status = EmwOsInterface::eOK;
      }
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::releaseSemaphore(semaphore_t *semaphorePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != semaphorePtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*semaphorePtr);
    if (nullptr != queue) {
      if (pdTRUE == xPortIsInsideInterrupt()) {
        BaseType_t yield = pdFALSE;
        if (pdTRUE == xQueueGiveFromISR(queue, &yield)) {
          if (pdTRUE == yield) {
            taskYIELD();
          }
          status = EmwOsInterface::eOK;
        }
      }
      else {
        if (pdPASS == xQueueGenericSend(queue, nullptr, semGIVE_BLOCK_TIME, queueSEND_TO_BACK)) {
          status = EmwOsInterface::eOK;
        }
      }
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteSemaphore(semaphore_t *semaphorePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != semaphorePtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*semaphorePtr);
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

EmwOsInterface::Status EmwOsInterface::createMutex(mutex_t *mutexPtr, const char *mutexNamePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != mutexPtr) {
#if ((configSUPPORT_DYNAMIC_ALLOCATION == 1) && (configUSE_RECURSIVE_MUTEXES == 1))
    const QueueHandle_t queue = xQueueCreateMutex(queueQUEUE_TYPE_RECURSIVE_MUTEX);
    if (nullptr != queue) {
      *mutexPtr = (mutex_t)queue;
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueAddToRegistry(queue, (nullptr != mutexNamePtr) ? mutexNamePtr : "");
#endif /*configQUEUE_REGISTRY_SIZE*/
      status = EmwOsInterface::eOK;
    }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION && configUSE_RECURSIVE_MUTEXES */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::takeMutex(mutex_t *mutexPtr, uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != mutexPtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*mutexPtr);
    if (nullptr != queue) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
      if (pdPASS == xQueueTakeMutexRecursive(queue, MilliSecondsToTicks(timeoutInMs))) {
        status = EmwOsInterface::eOK;
      }
#endif /* configUSE_RECURSIVE_MUTEXES */
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::releaseMutex(mutex_t *mutexPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != mutexPtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*mutexPtr);
    if (nullptr != queue) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
      if (pdPASS == xQueueGiveMutexRecursive(queue)) {
        status = EmwOsInterface::eOK;
      }
#endif /* configUSE_RECURSIVE_MUTEXES */
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteMutex(mutex_t *mutexPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != mutexPtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*mutexPtr);
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

EmwOsInterface::Status EmwOsInterface::createMessageQueue(queue_t *queuePtr, const char *queueNamePtr,
    uint32_t messageCount)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != queuePtr) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    const QueueHandle_t queue = xQueueCreate(messageCount, sizeof(void *));
    if (nullptr != queue) {
#if (configQUEUE_REGISTRY_SIZE > 0)
      vQueueAddToRegistry(queue, (nullptr != queueNamePtr) ? queueNamePtr : "");
#endif /* configQUEUE_REGISTRY_SIZE*/
      *queuePtr = (queue_t)queue;
      EMW_OS_DEBUG_LOG("\nEmwOsInterface::createMessageQueue(): queue %p\n", static_cast<void *>(queue))
      status = EmwOsInterface::eOK;
    }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::putMessageQueue(queue_t *queuePtr, const void *messagePtr,
    uint32_t timeoutInMs)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if ((nullptr != queuePtr) && (nullptr != messagePtr)) {
    const QueueHandle_t queue = (QueueHandle_t)(*queuePtr);
    const void *const item = messagePtr;
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::putMessageQueue(): pushing %p\n", messagePtr)
    if (pdPASS == xQueueSendToBack(queue, &item, (TickType_t)MilliSecondsToTicks(timeoutInMs))) {
      status = EmwOsInterface::eOK;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::getMessageQueue(queue_t *queuePtr, uint32_t timeoutInMs,
    const void **messagePtrPtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if ((nullptr != queuePtr) && (nullptr != messagePtrPtr)) {
    const QueueHandle_t queue = (QueueHandle_t)(*queuePtr);
    void *message_ptr = nullptr;
    *messagePtrPtr = nullptr;
    if (pdTRUE == xQueueReceive(queue, &message_ptr, MilliSecondsToTicks(timeoutInMs))) {
      *messagePtrPtr = message_ptr;
      status = EmwOsInterface::eOK;
      EMW_OS_DEBUG_LOG("\nEmwOsInterface::getMessageQueue: popped %p\n", (void *)*messagePtrPtr)
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::deleteMessageQueue(queue_t *queuePtr)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if (nullptr != queuePtr) {
    const QueueHandle_t queue = (QueueHandle_t)(*queuePtr);
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

EmwOsInterface::Status EmwOsInterface::createThread(thread_t *threadPtr, const char *threadNamePtr,
    thread_function_t function, thread_function_argument argument,
    uint32_t stackSize, thread_priority_t priority)
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  if ((nullptr != threadPtr) && (EMW_OS_MINIMAL_THREAD_STACK_SIZE <= stackSize)) {
    const configSTACK_DEPTH_TYPE stack = (uint16_t)(stackSize); /* In number of 32-bits words. */
    TaskHandle_t thread = nullptr;
    if (pdPASS == xTaskCreate((TaskFunction_t)function, (nullptr != threadNamePtr) ? threadNamePtr : "",
                              stack, const_cast<void *>(argument), static_cast<UBaseType_t>(priority), &thread)) {
      if (nullptr != thread) {
        *threadPtr = static_cast<thread_t>(thread);
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

EmwOsInterface::Status EmwOsInterface::terminateThread(thread_t *threadPtr)
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
    const uint32_t tick_freq = configTICK_RATE_HZ;
    ticks = ((uint64_t)timeoutInMs * (uint64_t)tick_freq) / (uint64_t)1000;
    if (ticks == 0U) {
      ticks = 1U;
    }
    if (ticks >= UINT32_MAX) {
      ticks = UINT32_MAX - 1U;
    }
  }
  return (uint32_t)ticks;
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
