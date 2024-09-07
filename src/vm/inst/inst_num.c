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
#include "inst_common.h"

Arg *vm_inst_handler_NUM(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    /* fast return */
    if (data[1] == '\0') {
        return arg_setInt(arg_ret_reg, "", data[0] - '0');
    }
    /* hex */
    if (data[1] == 'x' || data[1] == 'X') {
        return arg_setInt(arg_ret_reg, "", strtoll(data, NULL, 0));
    }
    if (data[1] == 'o' || data[1] == 'O') {
        char strtoll_buff[10] = {0};
        strtoll_buff[0] = '0';
        pika_platform_memcpy(strtoll_buff + 1, data + 2, strGetSize(data) - 2);
        return arg_setInt(arg_ret_reg, "", strtoll(strtoll_buff, NULL, 0));
    }
    if (data[1] == 'b' || data[1] == 'B') {
        char strtoll_buff[10] = {0};
        pika_platform_memcpy(strtoll_buff, data + 2, strGetSize(data) - 2);
        return arg_setInt(arg_ret_reg, "", strtoll(strtoll_buff, NULL, 2));
    }
    /* float */
    if (strIsContain(data, '.') ||
        (strIsContain(data, 'e') || strIsContain(data, 'E'))) {
        return arg_setFloat(arg_ret_reg, "", strtod(data, NULL));
    }
    /* int */
    int64_t i64 = 0;
    if (PIKA_RES_OK != fast_atoi_safe(data, &i64)) {
        vm_frame_set_sys_out(vm, "ValueError: invalid literal for int(): '%s'",
                              data);
        vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
        return NULL;
    }
    return arg_setInt(arg_ret_reg, "", i64);
}
