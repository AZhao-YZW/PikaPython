/*
 * This file is part of the PikaScript project.
 * http://github.com/pikastech/pikascript
 *
 * MIT License
 *
 * Copyright (c) 2021 lyon 李昂 liang6516@outlook.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* micro pika configuration */
#include "./pika_config_valid.h"

#ifndef __PIKA_PALTFORM__H
#define __PIKA_PALTFORM__H
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compiler */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 5000000) /* ARM Compiler */
#define PIKA_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__) /* for IAR Compiler */
#define PIKA_WEAK __weak
#elif defined(__MINGW32__) /* MINGW32 Compiler */
#define PIKA_WEAK 
#elif defined(__GNUC__) /* GNU GCC Compiler */
#define PIKA_WEAK __attribute__((weak))
#endif
/* default PIKA_WEAK */
#ifndef PIKA_WEAK
#define PIKA_WEAK
#endif

/* OS */
#ifdef __RTTHREAD__
#include <rtthread.h>
#define __platform_printf(...) rt_kprintf(__VA_ARGS__)
#endif

typedef enum {
    
    PIKA_ERR_UNKNOWN_INSTRUCTION                    = -11,
    PIKA_ERR_OUT_OF_RANGE                           = -10,
    PIKA_ERR_IO_ERROR                               = -9,
    PIKA_ERR_INSUFFICIENT_RESOURCE                  = -8,
    PIKA_ERR_INVALID_PARAM                          = -7,
    PIKA_ERR_INVALID_PTR                            = -6,
    PIKA_ERR_UNALIGNED_PTR                          = -5,
    PIKA_ERR_INVALID_VERSION_NUMBER                 = -4,
    PIKA_ERR_ILLEGAL_MAGIC_CODE                     = -3,
    PIKA_ERR_OPERATION_FAILED                       = -2,
    PIKA_ERR_UNKNOWN                                = -1,
    PIKA_ERR_NONE                                   = 0,
    PIKA_ERR_OK                                     = 0,
    
} PikaErr;

/*
    [Note]:
    Create a pika_config.c to override the following weak functions to config
   PikaScript. [Example]:
    1.
   https://gitee.com/Lyon1998/pikascript/blob/master/package/STM32G030Booter/pika_config.c
    2.
   https://gitee.com/Lyon1998/pikascript/blob/master/package/pikaRTBooter/pika_config.c
*/

/* interrupt config */
void __platform_enable_irq_handle(void);
void __platform_disable_irq_handle(void);

/* printf family config */
#ifndef __platform_printf
void __platform_printf(char* fmt, ...);
#endif
int __platform_sprintf(char* buff, char* fmt, ...);
int __platform_vsprintf(char* buff, char* fmt, va_list args);
int __platform_vsnprintf(char* buff,
                         size_t size,
                         const char* fmt,
                         va_list args);

/* libc config */
void* __platform_malloc(size_t size);
void __platform_free(void* ptr);
void* __platform_memset(void* mem, int ch, size_t size);
void* __platform_memcpy(void* dir, const void* src, size_t size);

void* __user_malloc(size_t size);
void __user_free(void* ptr, size_t size);

/* pika memory pool config */
void __platform_wait(void);
uint8_t __is_locked_pikaMemory(void);

/* support shell */
char __platform_getchar(void);

/* file API */
FILE* __platform_fopen(const char* filename, const char* modes);
int __platform_fclose(FILE* stream);
size_t __platform_fwrite(const void* ptr, size_t size, size_t n, FILE* stream);
size_t __platform_fread(void* ptr, size_t size, size_t n, FILE* stream);

/* error */
void __platform_error_handle(void);

#endif
