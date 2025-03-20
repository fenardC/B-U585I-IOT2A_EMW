#pragma once

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#else
#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#endif /* __cplusplus */

#include "stm32u5xx_hal.h"

/* #define EMW_API_DEBUG */
/* #define EMW_IPC_DEBUG */
/* #define EMW_HCI_DEBUG */
/* #define EMW_IO_DEBUG  */

#define DRIVER_ERROR_VERBOSE(...) (void)std::printf(__VA_ARGS__);

#if defined(EMW_API_DEBUG)
#define DEBUG_API_LOG(...)        (void)std::printf(__VA_ARGS__);
#endif /* EMW_API_DEBUG */

#if defined(EMW_IPC_DEBUG)
#define DEBUG_IPC_LOG(...)        (void)std::printf(__VA_ARGS__);
#endif /* EMW_IPC_DEBUG */

#if defined(EMW_HCI_DEBUG)
#define DEBUG_HCI_LOG(...)        (void)std::printf(__VA_ARGS__);
#define DEBUG_HCI_WARNING(...)    (void)std::printf(__VA_ARGS__);
#endif /* EMW_HCI_DEBUG */

#if defined(EMW_IO_DEBUG)
#define DEBUG_IO_LOG(...)         (void)std::printf(__VA_ARGS__);
#define DEBUG_IO_WARNING(...)     (void)std::printf(__VA_ARGS__);
#endif /* EMW_IO_DEBUG */

#if 0
#define EMW_OS_DEBUG_LOG(...)        \
  do                                 \
  {                                  \
    (void) std::printf(__VA_ARGS__); \
  } while(false);
#endif

#define EMW_CMD_TIMEOUT                         (10000U)

#define EMW_IO_SPI_THREAD_PRIORITY              (31)
#define EMW_IO_SPI_THREAD_STACK_SIZE            (360U + 240U)

#define EMW_RECEIVED_THREAD_PRIORITY            (18)
#define EMW_RECEIVED_THREAD_STACK_SIZE          (360U + 384U)

#define EMW_HCI_MAX_RX_BUFFER_COUNT             (4U)

#define EMW_STATS_ON                            (1)

#if (defined(EMW_STATS_ON) && (EMW_STATS_ON == 1))
#ifdef __cplusplus
typedef struct EmwStatistics_s {
  constexpr EmwStatistics_s(void)
    : alloc(0U), free(0U), cmdGetAnswer(0U), callback(0U), fifoIn(0U), fifoOut(0U) {}
  std::uint32_t alloc;
  std::uint32_t free;
  std::uint32_t cmdGetAnswer;
  std::uint32_t callback;
  std::uint32_t fifoIn;
  std::uint32_t fifoOut;
} EmwStatistics_t;

extern EmwStatistics_t EmwStats;

#define EMW_STATS_DECLARE()    EmwStatistics_t EmwStats;
#define EMW_STATS_INCREMENT(A) EmwStats.A++;

#define EMW_STATS_LOG()                                                                      \
  (void)std::printf("\n Number of allocated buffer for Rx and command answer %" PRIu32 "\n", \
                    EmwStats.alloc);                                                         \
  (void)std::printf(" Number of freed buffer %" PRIu32 "\n", EmwStats.free);                 \
  (void)std::printf(" Number of command answer %" PRIu32 ", callback %" PRIu32 ","           \
                    " sum of both %" PRIu32 " (must match alloc && free)\n",                 \
                    EmwStats.cmdGetAnswer, EmwStats.callback,                                \
                    EmwStats.cmdGetAnswer + EmwStats.callback);                              \
  (void)std::printf(" Number of posted answer (callback + cmd answer) %" PRIu32 ","          \
                    " processed answer %" PRIu32 "\n\n",\
                    EmwStats.fifoIn, EmwStats.fifoOut);
#endif /* __cplusplus */
#else /* EMW_STATS_ON */
#define EMW_STATS_DECLARE()
#define EMW_STATS_INCREMENT(A)
#define EMW_STATS_LOG()
#endif /* EMW_STATS_ON */

