/*
 * MIT License
 *
 * Copyright (c) 2024 azhao 余钊炜 yuzhaowei2002@outlook.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "leaf_lib.h"
#include "PikaPlatform.h"
#include "dataString.h"
#include "dataMemory.h"

static const uint64_t __talbe_fast_atoi[][10] = {
    {0, 1e0, 2e0, 3e0, 4e0, 5e0, 6e0, 7e0, 8e0, 9e0},
    {0, 1e1, 2e1, 3e1, 4e1, 5e1, 6e1, 7e1, 8e1, 9e1},
    {0, 1e2, 2e2, 3e2, 4e2, 5e2, 6e2, 7e2, 8e2, 9e2},
    {0, 1e3, 2e3, 3e3, 4e3, 5e3, 6e3, 7e3, 8e3, 9e3},
    {0, 1e4, 2e4, 3e4, 4e4, 5e4, 6e4, 7e4, 8e4, 9e4},
    {0, 1e5, 2e5, 3e5, 4e5, 5e5, 6e5, 7e5, 8e5, 9e5},
    {0, 1e6, 2e6, 3e6, 4e6, 5e6, 6e6, 7e6, 8e6, 9e6},
    {0, 1e7, 2e7, 3e7, 4e7, 5e7, 6e7, 7e7, 8e7, 9e7},
    {0, 1e8, 2e8, 3e8, 4e8, 5e8, 6e8, 7e8, 8e8, 9e8},
    {0, 1e9, 2e9, 3e9, 4e9, 5e9, 6e9, 7e9, 8e9, 9e9},
};

int64_t fast_atoi(char* src)
{
    const char* p = src;
    uint16_t size = strGetSize(src);
    p = p + size - 1;
    if (*p) {
        int64_t s = 0;
        const uint64_t* n = __talbe_fast_atoi[0];
        while (p != src) {
            s += n[(*p - '0')];
            n += 10;
            p--;
        }
        if (*p == '-') {
            return -s;
        }
        return s + n[(*p - '0')];
    }
    return 0;
}

PIKA_RES fast_atoi_safe(char* src, int64_t* out)
{
    // Check is digit
    char* p = src;
    while (*p) {
        if (*p < '0' || *p > '9') {
            return PIKA_RES_ERR_INVALID_PARAM;
        }
        p++;
    }
    *out = fast_atoi(src);
    return PIKA_RES_OK;
}

char* fast_itoa(char* buf, uint32_t val)
{
    char* p = &buf[10];
    *p = '\0';
    while (val >= 10) {
        p--;
        *p = '0' + (val % 10);
        val /= 10;
    }
    p--;
    *p = '0' + val;
    return p;
}

static PIKA_BOOL _check_no_buff_format(char* format) {
    while (*format) {
        if (*format == '%') {
            ++format;
            if (*format != 's' && *format != '%') {
                return PIKA_FALSE;
            }
        }
        ++format;
    }
    return PIKA_TRUE;
}

int pika_pvsprintf(char** buff, const char* fmt, va_list args) {
    int required_size;
    int current_size = PIKA_SPRINTF_BUFF_SIZE;
    *buff = (char*)pika_platform_malloc(current_size * sizeof(char));

    if (*buff == NULL) {
        return -1;  // Memory allocation failed
    }

    va_list args_copy;
    va_copy(args_copy, args);

    required_size =
        pika_platform_vsnprintf(*buff, current_size, fmt, args_copy);
    va_end(args_copy);

    while (required_size >= current_size) {
        current_size *= 2;
        char* new_buff = (char*)pika_reallocn(*buff, current_size / 2,
                                              current_size * sizeof(char));

        if (new_buff == NULL) {
            pika_platform_free(*buff);
            *buff = NULL;
            return -1;  // Memory allocation failed
        } else {
            *buff = new_buff;
        }

        va_copy(args_copy, args);
        required_size =
            pika_platform_vsnprintf(*buff, current_size, fmt, args_copy);
        va_end(args_copy);
    }

    return required_size;
}

static int _no_buff_vprintf(char* fmt, va_list args) {
    int written = 0;
    while (*fmt) {
        if (*fmt == '%') {
            ++fmt;
            if (*fmt == 's') {
                char* str = va_arg(args, char*);
                if (str == NULL) {
                    str = "(null)";
                }
                int len = strlen(str);
                written += len;
                for (int i = 0; i < len; i++) {
                    pika_putchar(str[i]);
                }
            } else if (*fmt == '%') {
                pika_putchar('%');
                ++written;
            }
        } else {
            pika_putchar(*fmt);
            ++written;
        }
        ++fmt;
    }
    return written;
}

int pika_vprintf(char* fmt, va_list args) {
    int ret = 0;
    if (_check_no_buff_format(fmt)) {
        _no_buff_vprintf(fmt, args);
        return 0;
    }

    char* buff = NULL;
    int required_size = pika_pvsprintf(&buff, fmt, args);

    if (required_size < 0) {
        ret = -1;  // Memory allocation or other error occurred
        goto __exit;
    }

    // putchar
    for (int i = 0; i < strlen(buff); i++) {
        pika_putchar(buff[i]);
    }

__exit:
    if (NULL != buff) {
        pika_platform_free(buff);
    }
    return ret;
}

int pika_sprintf(char* buff, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = pika_platform_vsnprintf(buff, PIKA_SPRINTF_BUFF_SIZE, fmt, args);
    va_end(args);
    if (res >= PIKA_SPRINTF_BUFF_SIZE) {
        pika_platform_printf(
            "OverflowError: sprintf buff size overflow, please use bigger "
            "PIKA_SPRINTF_BUFF_SIZE\r\n");
        pika_platform_printf("Info: buff size request: %d\r\n", res);
        pika_platform_printf("Info: buff size now: %d\r\n",
                             PIKA_SPRINTF_BUFF_SIZE);
        while (1)
            ;
    }
    return res;
}

int pika_vsprintf(char* buff, char* fmt, va_list args) {
    /* vsnprintf */
    return pika_platform_vsnprintf(buff, PIKA_SPRINTF_BUFF_SIZE, fmt, args);
}

int pika_snprintf(char* buff, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = pika_platform_vsnprintf(buff, size, fmt, args);
    va_end(args);
    return ret;
}

void _do_vsysOut(char* fmt, va_list args) {
    char* fmt_buff = pikaMalloc(strGetSize(fmt) + 2);
    pika_platform_memcpy(fmt_buff, fmt, strGetSize(fmt));
    fmt_buff[strGetSize(fmt)] = '\n';
    fmt_buff[strGetSize(fmt) + 1] = '\0';
    char* out_buff = pikaMalloc(PIKA_SPRINTF_BUFF_SIZE);
    pika_platform_vsnprintf(out_buff, PIKA_SPRINTF_BUFF_SIZE, fmt_buff, args);
    pika_platform_printf("%s", out_buff);
    pikaFree(out_buff, PIKA_SPRINTF_BUFF_SIZE);
    pikaFree(fmt_buff, strGetSize(fmt) + 2);
}