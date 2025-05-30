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
#ifndef STDIO_UART_H
#define STDIO_UART_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int InitializeStdoutWithUart(void);
int io_putchar(char ch);

int InitializeStdinWithUart(void);
int ReinitializeStdinWithUart(void);
char io_getchar(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STDIO_UART_H */
