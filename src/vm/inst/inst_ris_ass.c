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

Arg *vm_inst_handler_RIS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
#if PIKA_NANO_ENABLE
    return NULL;
#endif
    Arg *err_arg = stack_popArg_alloc(&(vm->stack));
    if (ARG_TYPE_INT == arg_getType(err_arg)) {
        PIKA_RES err = (PIKA_RES)arg_getInt(err_arg);
        if (PIKA_RES_ERR_RUNTIME_ERROR != err) {
            vm_frame_set_error_code(vm, PIKA_RES_ERR_INVALID_PARAM);
            vm_frame_set_sys_out(
                vm, "TypeError: exceptions must derive from BaseException");
            goto __exit;
        }
        vm_frame_set_error_code(vm, err);
    }
    if (arg_isConstructor(err_arg)) {
        MethodProp* method_prop = methodArg_getProp(err_arg);
        vm_frame_set_error_code(vm, (uintptr_t)method_prop->ptr);
        goto __exit;
    }
__exit:
    arg_deinit(err_arg);
    return NULL;
}

Arg *vm_inst_handler_ASS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
#if PIKA_NANO_ENABLE
    return NULL;
#endif
    arg_newReg(reg1, PIKA_ARG_BUFF_SIZE);
    arg_newReg(reg2, PIKA_ARG_BUFF_SIZE);
    Arg *arg1 = NULL;
    Arg *arg2 = NULL;
    Arg *res = NULL;
    uint32_t n_arg = vm_frame_get_input_arg_num(vm);
    if (n_arg == 1) {
        arg1 = stack_popArg(&vm->stack, &reg1);
    }
    if (n_arg == 2) {
        arg2 = stack_popArg(&vm->stack, &reg2);
        arg1 = stack_popArg(&vm->stack, &reg1);
    }
    /* assert failed */
    if ((arg_getType(arg1) == ARG_TYPE_INT && arg_getInt(arg1) == 0) ||
        (arg_getType(arg1) == ARG_TYPE_BOOL &&
         arg_getBool(arg1) == pika_false)) {
        stack_pushArg(&vm->stack, arg_newInt(PIKA_RES_ERR_ASSERT));
        res = vm_inst_handler_RIS(self, vm, data, arg_ret_reg);
        // if (vm->vm_thread->try_state == TRY_STATE_NONE) {
        if (n_arg == 1) {
            pika_platform_printf("AssertionError\n");
        }
        if (n_arg == 2) {
            pika_platform_printf("AssertionError: %s\n", arg_getStr(arg2));
        }
        // }
        goto __exit;
    }
__exit:
    arg_deinit(arg1);
    if (NULL != arg2) {
        arg_deinit(arg2);
    }
    return res;
}