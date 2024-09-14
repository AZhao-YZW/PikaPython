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
#include "const_pool.h"
#include "PikaVM.h"

void constPool_update(ConstPool *self) {
    self->content_start = (void*)arg_getContent(self->arg_buff);
}

void const_pool_init(ConstPool *self) {
    self->arg_buff = NULL;
    self->content_start = NULL;
    self->content_offset_now = 0;
    self->size = 1;
    self->output_redirect_fun = NULL;
    self->output_f = NULL;
}

void const_pool_deinit(ConstPool *self) {
    if (NULL != self->arg_buff) {
        arg_deinit(self->arg_buff);
    }
}

void const_pool_append(ConstPool *self, char *content) {
    if (NULL == self->arg_buff) {
        self->arg_buff = arg_newStr("");
    }
    uint16_t size = strGetSize(content) + 1;
    if (NULL == self->output_redirect_fun) {
        self->arg_buff = arg_append(self->arg_buff, content, size);
    } else {
        self->output_redirect_fun(self, content);
    };
    constPool_update(self);
    self->size += size;
}

char *constPool_getNow(ConstPool *self) {
    if (self->content_offset_now >= self->size) {
        /* is the end */
        return NULL;
    }
    return (char*)((uintptr_t)const_pool_get_start(self) +
                   (uintptr_t)(self->content_offset_now));
}

char *constPool_getNext(ConstPool *self) {
    self->content_offset_now += strGetSize(constPool_getNow(self)) + 1;
    return constPool_getNow(self);
}

uint16_t const_pool_get_offset_by_data(ConstPool *self, char *data) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    uint16_t offset_out = 65535;
    if (self->arg_buff == NULL) {
        goto __exit;
    }
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto __exit;
        }
        if (strEqu(data, constPool_getNow(self))) {
            offset_out = self->content_offset_now;
            goto __exit;
        }
    }
__exit:
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return offset_out;
}

char *constPool_getByIndex(ConstPool *self, uint16_t index) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    for (uint16_t i = 0; i < index; i++) {
        constPool_getNext(self);
    }
    char *res = constPool_getNow(self);
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return res;
}

void const_pool_print(ConstPool *self) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto __exit;
        }
        uint16_t offset = self->content_offset_now;
        pika_platform_printf("%d: %s\r\n", offset, constPool_getNow(self));
    }
__exit:
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return;
}

void const_pool_get_print_as_array(ConstPool *self)
{
    uint8_t* const_size_str = (uint8_t*)&(self->size);
    pika_platform_printf("0x%02x, ", *(const_size_str));
    pika_platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)1));
    pika_platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)2));
    pika_platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)3));
    pika_platform_printf("/* const pool size */\n");
    uint16_t ptr_befor = self->content_offset_now;
    uint8_t line_num = 12;
    uint16_t g_i = 0;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    pika_platform_printf("0x00, ");
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto __exit;
        }
        char *data_each = constPool_getNow(self);
        /* todo start */
        size_t len = strlen(data_each);
        for (uint32_t i = 0; i < len + 1; i++) {
            pika_platform_printf("0x%02x, ", *(data_each + (uintptr_t)i));
            g_i++;
            if (g_i % line_num == 0) {
                pika_platform_printf("\n");
            }
        }
        /* todo end */
    }
__exit:
    /* retore ptr_now */
    pika_platform_printf("/* const pool */\n");
    self->content_offset_now = ptr_befor;
    return;
}