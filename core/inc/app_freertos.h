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
#ifndef APP_FREERTOS_H
#define APP_FREERTOS_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//#include "FreeRTOS.h"
//#include "task.h"

void InitializeFreeRtosRootTask(void);

void vApplicationMallocFailedHook(void);
//void vApplicationStackOverflowHook(xTaskHandle task, char *taskNamePtr)
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* APP_FREERTOS_H */
