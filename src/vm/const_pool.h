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
#ifndef __CONST_POOL_H__
#define __CONST_POOL_H__

#include "dataArgs.h"
#include "PikaVM.h"

static inline void *const_pool_get_start(ConstPool *self)
{
    return self->content_start;
}

static inline int const_pool_get_last_offset(ConstPool *self)
{
    return self->size;
}

static inline char *const_pool_get_by_offset(ConstPool *self, int offset)
{
    return (char*)((uintptr_t)const_pool_get_start(self) + (uintptr_t)offset);
}

void const_pool_print(ConstPool *self);
uint16_t const_pool_get_offset_by_data(ConstPool *self, char *data);
void const_pool_get_print_as_array(ConstPool *self);
void const_pool_init(ConstPool *self);
void const_pool_deinit(ConstPool *self);
void const_pool_append(ConstPool *self, char *content);

#endif