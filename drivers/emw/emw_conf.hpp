#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include "stm32u5xx_hal.h"
#ifdef __cplusplus
#include <cstdio>
#endif /* __cplusplus */

#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

#define EMW_FLOW_Pin GPIO_PIN_15
#define EMW_FLOW_GPIO_Port GPIOG
#define EMW_FLOW_EXTI_IRQn EXTI15_IRQn
#define EMW_NOTIFY_Pin GPIO_PIN_14
#define EMW_NOTIFY_GPIO_Port GPIOD
#define EMW_NOTIFY_EXTI_IRQn EXTI14_IRQn
#define EMW_NSS_Pin GPIO_PIN_12
#define EMW_NSS_GPIO_Port GPIOB
#define EMW_RESET_Pin GPIO_PIN_15
#define EMW_RESET_GPIO_Port GPIOF

/* #define EMW_API_DEBUG */
/* #define EMW_IPC_DEBUG */
/* #define EMW_HCI_DEBUG */
/* #define EMW_IO_DEBUG  */

#define DRIVER_ERROR_VERBOSE(...) (void)std::printf(__VA_ARGS__);

#if defined(EMW_API_DEBUG) || defined(EMW_IPC_DEBUG) \
  || defined(EMW_HCI_DEBUG) || defined(EMW_IO_DEBUG)
#include <stdio.h>
#define SETBUF(STREAM, BUFFER) setbuf((STREAM), (BUFFER));
#endif /* EMW3080B_XXXX_DEBUG */

#if defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)      (void)std::printf(__VA_ARGS__);
#endif /* EMW_API_DEBUG */

#if defined(EMW_IPC_DEBUG)
#define DEBUG_IPC_LOG(...)      (void)std::printf(__VA_ARGS__);
#endif /* EMW_IPC_DEBUG */

#if defined(EMW_HCI_DEBUG)
#define DEBUG_HCI_LOG(...)      (void)std::printf(__VA_ARGS__);
#define DEBUG_HCI_WARNING(...)  (void)std::printf(__VA_ARGS__);
#endif /* EMW_HCI_DEBUG */

#if defined(EMW_IO_DEBUG)
#define DEBUG_IO_LOG(...)       (void)std::printf(__VA_ARGS__);
#define DEBUG_IO_WARNING(...)   (void)std::printf(__VA_ARGS__);
#endif /* EMW_IO_DEBUG */

#if 0
#define EMW_OS_DEBUG_LOG(...)        \
  do                                 \
  {                                  \
    (void) std::printf(__VA_ARGS__); \
  } while(false);
#endif

#ifndef EMW_CMD_TIMEOUT
#define EMW_CMD_TIMEOUT                         (10000U)
#endif /* EMW3080B_CMD_TIMEOUT */

#ifndef EMW_IO_SPI_THREAD_PRIORITY
#define EMW_IO_SPI_THREAD_PRIORITY              (31)
#endif /* EMW_IO_SPI_THREAD_PRIORITY */

#ifndef EMW_IO_SPI_THREAD_STACK_SIZE
#define EMW_IO_SPI_THREAD_STACK_SIZE            (196U)
#endif /* EMW_IO_SPI_THREAD_STACK_SIZE */

#ifndef EMW_RECEIVED_THREAD_PRIORITY
#define EMW_RECEIVED_THREAD_PRIORITY            (18)
#endif /* EMW_RECEIVED_THREAD_PRIORITY */

#ifndef EMW_RECEIVED_THREAD_STACK_SIZE
#define EMW_RECEIVED_THREAD_STACK_SIZE          (196U)
#endif /* EMW_RECEIVED_THREAD_STACK_SIZE*/

#ifndef EMW_TRANSMIT_THREAD_PRIORITY
#define EMW_TRANSMIT_THREAD_PRIORITY            (17)
#endif /* EMW_TRANSMIT_THREAD_PRIORITY */

#ifndef EMW_TRANSMIT_THREAD_STACK_SIZE
#define EMW_TRANSMIT_THREAD_STACK_SIZE          (256U)
#endif /* EMW_TRANSMIT_THREAD_STACK_SIZE */

#ifndef EMW_HCI_MAX_RX_BUFFER_COUNT
#define EMW_HCI_MAX_RX_BUFFER_COUNT             (4U)
#endif /* EMW_HCI_MAX_RX_BUFFER_COUNT */

#ifndef EMW_MAX_TX_BUFFER_COUNT
#define EMW_MAX_TX_BUFFER_COUNT                 (4U)
#endif /* EMW_MAX_TX_BUFFER_COUNT */

#define EMW_STATS_ON                            (1)

#ifdef __cplusplus
#if (defined(EMW_STATS_ON) && (EMW_STATS_ON == 1))
typedef struct EmwStatistics_s {
  EmwStatistics_s(void) : alloc(0U), free(0U), cmdGetAnswer(0U), callback(0U), fifoIn(0U), fifoOut(0U) {}
  uint32_t alloc;
  uint32_t free;
  uint32_t cmdGetAnswer;
  uint32_t callback;
  uint32_t fifoIn;
  uint32_t fifoOut;
} EmwStatistics_t;

extern EmwStatistics_t EmwStats;

#define EMW_STATS_DECLARE()    EmwStatistics_t EmwStats;
#define EMW_STATS_INCREMENT(A) EmwStats.A++;

#define EMW_STATS_LOG()                                                                                              \
  (void)std::printf("\n Number of allocated buffer for Rx and command answer %" PRIu32 "\n",                         \
                    EmwStats.alloc);                                                                                 \
  (void)std::printf(" Number of freed buffer %" PRIu32 "\n", EmwStats.free);                                         \
  (void)std::printf(" Number of command answer %" PRIu32 ", callback %" PRIu32 ","                                   \
                    " sum of both %" PRIu32 " (must match alloc && free)\n",                                         \
                    EmwStats.cmdGetAnswer, EmwStats.callback,                                                        \
                    EmwStats.cmdGetAnswer + EmwStats.callback);                                                      \
  (void)std::printf(" Number of posted answer (callback + cmd answer) %" PRIu32 ", processed answer %" PRIu32 "\n\n",\
                    EmwStats.fifoIn, EmwStats.fifoOut);

#else /* EMW_STATS_ON */
#define EMW_STATS_DECLARE()
#define EMW_STATS_INCREMENT(A)
#define EMW_STATS_LOG()
#endif /* EMW_STATS_ON */
#endif /* __cplusplus */
