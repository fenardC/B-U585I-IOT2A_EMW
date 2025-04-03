/**
 ******************************************************************************
 * @file      syscalls.c
 * @brief     STM32CubeIDE Minimal System calls file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2020-2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

extern int io_putchar(char ch) __attribute__((weak));
extern char io_getchar(void) __attribute__((weak));

char *__env[1] = { 0 };
char **environ = __env;

/* Functions */
void initialise_monitor_handles(void)
{
}

int _getpid(void)
{
  return 1;
}

int _kill(int Pid, int Sig)
{
  (void)Pid;
  (void)Sig;
  errno = EINVAL;
  return -1;
}

void _exit(int Status)
{
  _kill(Status, -1);

  while (1) {}
}

__attribute__((weak)) int _read(int File, char *Ptr, int Length)
{
  (void)File;

  for (int data_idx = 0; data_idx < Length; data_idx++) {
    *Ptr++ = io_getchar();
  }

  return Length;
}

__attribute__((weak)) int _write(int File, char *Ptr, int Length)
{
  (void)File;

  for (int data_idx = 0; data_idx < Length; data_idx++) {
    io_putchar(*Ptr++);
  }

  return Length;
}

int _close(int file)
{
  (void)file;
  return -1;
}

int _fstat(int File, struct stat *St)
{
  (void)File;
  St->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int File)
{
  (void)File;
  return 1;
}

int _lseek(int File, int Ptr, int Dir)
{
  (void)File;
  (void)Ptr;
  (void)Dir;
  return 0;
}

int _open(char *Path, int Flags, ...)
{
  (void)Path;
  (void)Flags;
  return -1;
}

int _wait(int *Status)
{
  (void)Status;
  errno = ECHILD;
  return -1;
}

int _unlink(char *Name)
{
  (void)Name;
  errno = ENOENT;
  return -1;
}

int _times(struct tms *Buf)
{
  (void)Buf;
  return -1;
}

int _stat(char *File, struct stat *St)
{
  (void)File;
  St->st_mode = S_IFCHR;
  return 0;
}

int _link(char *Old, char *New)
{
  (void)Old;
  (void)New;
  errno = EMLINK;
  return -1;
}

int _fork(void)
{
  errno = EAGAIN;
  return -1;
}

int _execve(char *Name, char **Argv, char **Env)
{
  (void)Name;
  (void)Argv;
  (void)Env;
  errno = ENOMEM;
  return -1;
}

int _getentropy(void *buf, size_t buflen)
{
  (void)buf;
  (void)buflen;
  errno = ENOSYS;
  return -1;
}
