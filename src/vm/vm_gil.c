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
#include "vm_gil.h"
#include "PikaVM.h"

static pika_thread_recursive_mutex_t g_pikaGIL = {0};

pika_bool vm_gil_is_init(void)
{
    return g_pikaGIL.mutex.is_init;
}

int vm_gil_get_bare_lock(void)
{
    return g_pikaGIL.mutex.bare_lock;
}

int vm_gil_enter(void)
{
    if (!g_pikaGIL.mutex.is_init) {
        if (g_pikaGIL.mutex.bare_lock > 0) {
            return -1;
        }
        g_pikaGIL.mutex.bare_lock = 1;
        return 0;
    }
    int ret = pika_thread_recursive_mutex_lock(&g_pikaGIL);
    // pika_debug("vm_gil_enter");
    if (!g_pikaGIL.mutex.is_first_lock) {
        g_pikaGIL.mutex.is_first_lock = 1;
    }
    return ret;
}

int vm_gil_exit(void)
{
    if (!g_pikaGIL.mutex.is_first_lock || !g_pikaGIL.mutex.is_init) {
        g_pikaGIL.mutex.bare_lock = 0;
        return 0;
    }
    return pika_thread_recursive_mutex_unlock(&g_pikaGIL);
}

int vm_gil_deinit(void)
{
    pika_platform_memset(&g_pikaGIL, 0, sizeof(g_pikaGIL));
    return 0;
}

int vm_gil_init(void)
{
    if (g_pikaGIL.mutex.is_init) {
        return 0;
    }
    int ret = pika_thread_recursive_mutex_init(&g_pikaGIL);
    if (0 == ret) {
        g_pikaGIL.mutex.is_init = 1;
    }
    return ret;
}

int vm_gil_is_first_lock(void)
{
    return g_pikaGIL.mutex.is_first_lock;
}