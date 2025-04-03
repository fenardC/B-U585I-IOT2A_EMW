/**
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include <errno.h>
#include <stdint.h>

void *_sbrk(ptrdiff_t Increment);

static uint8_t *__sbrk_heap_end = NULL;

/**
 * @brief _sbrk() allocates memory to the newlib heap and is used by malloc
 *        and others from the C library
 *
 * ############################################################################
 * #  .data  #  .bss  #       newlib heap       #          MSP stack          #
 * #         #        #                         # Reserved by _Min_Stack_Size #
 * ############################################################################
 * ^-- RAM start      ^-- _end                             _estack, RAM end --^
 *
 * This implementation starts allocating at the '_end' linker symbol
 * The '_Min_Stack_Size' linker symbol reserves a memory for the MSP stack
 * The implementation considers '_estack' linker symbol to be RAM end
 * NOTE: If the MSP stack, at any point during execution, grows larger than the
 * reserved size, please increase the '_Min_Stack_Size'.
 */
void *_sbrk(ptrdiff_t Increment)
{
  extern uint8_t _end;
  extern uint8_t _estack;
  extern uint32_t _Min_Stack_Size;
  const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
  const uint8_t *max_heap = (uint8_t *)stack_limit;
  uint8_t *prev_heap_end;

  if (NULL == __sbrk_heap_end) {
    __sbrk_heap_end = &_end;
  }
  if (__sbrk_heap_end + Increment > max_heap) {
    errno = ENOMEM;
    return (void *) -1;
  }
  prev_heap_end = __sbrk_heap_end;
  __sbrk_heap_end += Increment;
  return (void *)prev_heap_end;
}
