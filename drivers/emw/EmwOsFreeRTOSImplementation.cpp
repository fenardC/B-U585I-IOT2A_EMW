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
static const std::uint8_t QUEUE_TYPE_BASE = 0U; /* queueQUEUE_TYPE_BASE */
static const std::uint8_t QUEUE_TYPE_RECURSIVE_MUTEX = 4U; /* queueQUEUE_TYPE_RECURSIVE_MUTEX */
static const BaseType_t SEND_TO_BACK = 0; /* queueSEND_TO_BACK */

static inline std::uint32_t MilliSecondsToTicks(std::uint32_t timeoutInMs);

static inline uint32_t MilliSecondsToTicks(std::uint32_t timeoutInMs)
{
  std::uint64_t ticks = 0U;

  if (1000 == configTICK_RATE_HZ) {
    return timeoutInMs;
  }

  if (timeoutInMs == EMW_OS_TIMEOUT_FOREVER) {
    ticks = 0xFFFFFFFFU;
  }
  if (timeoutInMs != 0U) {
    const std::uint64_t tick_freq = configTICK_RATE_HZ;
    ticks = (static_cast<std::uint64_t>(timeoutInMs) * tick_freq) / 1000;
    if (ticks == 0U) {
      ticks = 1U;
    }
    if (ticks >= UINT32_MAX) {
      ticks = UINT32_MAX - 1U;
    }
  }
  return static_cast<uint32_t>(ticks);
}

void EmwOsInterface::AssertAlways(bool condition) noexcept
{
  if (!condition) {
    while (true) {}
  }
}

void EmwOsInterface::Lock(void) noexcept
{
  vTaskSuspendAll();
}

void EmwOsInterface::UnLock(void) noexcept
{
  (void) xTaskResumeAll();
}

EmwOsInterface::Status EmwOsInterface::CreateSemaphore(EmwOsInterface::Semaphore_t &semaphore,
    const char *semaphoreNamePtr, std::uint32_t maxCount, std::uint32_t initialCount) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateSemaphore()> \"%s\"\n",
                   (nullptr != semaphoreNamePtr) ? semaphoreNamePtr : "")

#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
  const QueueHandle_t queue = xQueueCreateCountingSemaphore(maxCount, initialCount);

  if (nullptr != queue) {
    semaphore = queue;
    vQueueAddToRegistry(queue, (nullptr != semaphoreNamePtr) ? semaphoreNamePtr : "");
    status = EmwOsInterface::eOK;
  }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  return status;
}

EmwOsInterface::Status EmwOsInterface::TakeSemaphore(EmwOsInterface::Semaphore_t &semaphore,
    std::uint32_t timeoutInMs) noexcept
{
  EmwOsInterface::Status status;

  if (PASS == xQueueSemaphoreTake(semaphore, MilliSecondsToTicks(timeoutInMs))) {
    status = EmwOsInterface::eOK;
  }
  else {
    status = EmwOsInterface::eERROR;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::ReleaseSemaphore(EmwOsInterface::Semaphore_t &semaphore) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (TRUE == xPortIsInsideInterrupt()) {
    BaseType_t yield = FALSE;

    if (TRUE == xQueueGiveFromISR(semaphore, &yield)) {
      if (TRUE == yield) {
        taskYIELD();
      }
      status = EmwOsInterface::eOK;
    }
  }
  else {
    if (PASS == xQueueGenericSend(semaphore, nullptr, GIVE_BLOCK_TIME, SEND_TO_BACK)) {
      status = EmwOsInterface::eOK;
    }
  }
  return status;
}

void EmwOsInterface::DeleteSemaphore(EmwOsInterface::Semaphore_t &semaphore) noexcept
{
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::DeleteSemaphore()> \"%s\"\n", pcQueueGetName(semaphore))

  vQueueUnregisterQueue(semaphore);
  vQueueDelete(semaphore);
}

EmwOsInterface::Status EmwOsInterface::CreateMutex(EmwOsInterface::Mutex_t &mutex, const char *mutexNamePtr) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateMutex()> \"%s\"\n", (nullptr != mutexNamePtr) ? mutexNamePtr : "")

#if ((configSUPPORT_DYNAMIC_ALLOCATION == 1))
  {
    const QueueHandle_t queue = xQueueCreateMutex(QUEUE_TYPE_RECURSIVE_MUTEX);

    if (nullptr != queue) {
      mutex = queue;
      vQueueAddToRegistry(queue, (nullptr != mutexNamePtr) ? mutexNamePtr : "");
      status = EmwOsInterface::eOK;
    }
  }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  return status;
}

EmwOsInterface::Status EmwOsInterface::TakeMutex(EmwOsInterface::Mutex_t &mutex, std::uint32_t timeoutInMs) noexcept
{
  EmwOsInterface::Status status;

  if (PASS == xQueueTakeMutexRecursive(mutex, MilliSecondsToTicks(timeoutInMs))) {
    status = EmwOsInterface::eOK;
  }
  else {
    status = EmwOsInterface::eERROR;
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::ReleaseMutex(EmwOsInterface::Mutex_t &mutex) noexcept
{
  EmwOsInterface::Status status;

  if (PASS == xQueueGiveMutexRecursive(mutex)) {
    status = EmwOsInterface::eOK;
  }
  else {
    status = EmwOsInterface::eERROR;
  }
  return status;
}

void EmwOsInterface::DeleteMutex(EmwOsInterface::Mutex_t &mutex) noexcept
{
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::DeleteMutex()> \"%s\"\n", pcQueueGetName(mutex))

  vQueueUnregisterQueue(mutex);
  vQueueDelete(mutex);
}

EmwOsInterface::Status EmwOsInterface::CreateMessageQueue(EmwOsInterface::Queue_t &queue, const char *queueNamePtr,
    std::uint32_t messageCount) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateMessageQueue()> \"%s\"\n", (nullptr != queueNamePtr) ? queueNamePtr : "")

#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
  const QueueHandle_t the_queue = xQueueGenericCreate(messageCount, sizeof(void *), QUEUE_TYPE_BASE);

  if (nullptr != the_queue) {
    vQueueAddToRegistry(the_queue, (nullptr != queueNamePtr) ? queueNamePtr : "");
    queue = the_queue;
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateMessageQueue(): \"%s\" queue %p\n",
                     pcQueueGetName(the_queue), static_cast<void *>(the_queue))
    status = EmwOsInterface::eOK;
  }
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
  return status;
}

EmwOsInterface::Status EmwOsInterface::PutMessageQueue(EmwOsInterface::Queue_t &queue, const void *messagePtr,
    std::uint32_t timeoutInMs) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  if (nullptr != messagePtr) {
    const void *const item = messagePtr;

    EMW_OS_DEBUG_LOG("\n EmwOsInterface::PutMessageQueue(): pushing %p\n", messagePtr)

    if (PASS == xQueueGenericSend(queue, &item, MilliSecondsToTicks(timeoutInMs), SEND_TO_BACK)) {
      status = EmwOsInterface::eOK;
    }
  }
  return status;
}

EmwOsInterface::Status EmwOsInterface::GetMessageQueue(EmwOsInterface::Queue_t &queue, std::uint32_t timeoutInMs,
    const void *&messagePtr) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;
  void *message_ptr = nullptr;

  messagePtr = nullptr;
  if (TRUE == xQueueReceive(queue, &message_ptr, MilliSecondsToTicks(timeoutInMs))) {
    messagePtr = message_ptr;
    status = EmwOsInterface::eOK;
    EMW_OS_DEBUG_LOG("\n EmwOsInterface::GetMessageQueue(): popped  %p\n", messagePtr)
  }
  return status;
}

void EmwOsInterface::DeleteMessageQueue(EmwOsInterface::Queue_t &queue) noexcept
{
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::DeleteMessageQueue()> \"%s\"\n", pcQueueGetName(queue))

  vQueueUnregisterQueue(queue);
  vQueueDelete(queue);
  EMW_OS_DEBUG_LOG("\n EmwOsInterface::DeleteMessageQueue(): queue %p\n", static_cast<const void *>(queue))
}

EmwOsInterface::Status EmwOsInterface::CreateThread(EmwOsInterface::Thread_t &thread, const char *threadNamePtr,
    EmwOsInterface::ThreadFunction_t function, EmwOsInterface::ThreadFunctionArgument_t argument,
    std::uint32_t stackSize, EmwOsInterface::ThreadPriority_t priority) noexcept
{
  EmwOsInterface::Status status = EmwOsInterface::eERROR;

  EMW_OS_DEBUG_LOG("\n EmwOsInterface::CreateThread()> \"%s\"\n", (nullptr != threadNamePtr) ? threadNamePtr : "")

  if ((EMW_OS_MINIMAL_THREAD_STACK_SIZE <= stackSize)) {
    const configSTACK_DEPTH_TYPE stack = static_cast<std::uint16_t>(stackSize); /* In number of 32-bits words. */
    TaskHandle_t the_thread = nullptr;

    if (PASS == xTaskCreate(reinterpret_cast<TaskFunction_t>(function), (nullptr != threadNamePtr) ? threadNamePtr : "",
                            stack, const_cast<void *>(argument), static_cast<UBaseType_t>(priority), &the_thread)) {
      if (nullptr != the_thread) {
        thread = the_thread;
        status = EmwOsInterface::eOK;
      }
    }
    else {
      EmwOsInterface::AssertAlways(false);
    }
  }
  return status;
}

__NO_RETURN void EmwOsInterface::ExitThread(void) noexcept
{
  vTaskDelete(nullptr);
  for (;;) {;}
}

void EmwOsInterface::TerminateThread(EmwOsInterface::Thread_t &thread) noexcept
{
  /* Don't call vTaskDelete() */
  thread = nullptr;
}

void EmwOsInterface::Delay(std::uint32_t timeoutInMs) noexcept
{
  vTaskDelay(MilliSecondsToTicks(timeoutInMs));
}

void EmwOsInterface::DelayTicks(std::uint32_t count) noexcept
{
  vTaskDelay(count);
}

void *EmwOsInterface::Malloc(std::size_t size) noexcept
{
  void *const memory_ptr = pvPortMalloc(size);
  EmwOsInterface::AssertAlways(nullptr != memory_ptr);
  EMW_OS_DEBUG_LOG(" EmwOsInterface::Malloc(): %p (%" PRIu32 ")\n", memory_ptr, static_cast<std::uint32_t>(size))
  return memory_ptr;
}

void EmwOsInterface::Free(void *memoryPtr) noexcept
{
  EMW_OS_DEBUG_LOG(" EmwOsInterface::Free()  : %p\n", memoryPtr)

  vPortFree(memoryPtr);
}
