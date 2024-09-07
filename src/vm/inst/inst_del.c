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

Arg *vm_inst_handler_DEL(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    if (locals_check_lreg(data)) {
        locals_del_lreg(vm->locals, data);
        goto __exit;
    }
    if (obj_isArgExist(vm->locals, data)) {
        obj_removeArg(vm->locals, data);
        goto __exit;
    }
    if (obj_isArgExist(vm->globals, data)) {
        obj_removeArg(vm->globals, data);
        goto __exit;
    }
    vm_frame_set_error_code(vm, PIKA_RES_ERR_OPERATION_FAILED);
    vm_frame_set_sys_out(vm, "NameError: name '%s' is not defined", data);
__exit:
    return NULL;
}