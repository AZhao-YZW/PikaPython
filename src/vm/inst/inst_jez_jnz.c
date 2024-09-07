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

static Arg *_VM_JEZ(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg, int bAssert)
{
    int this_blk_depth = vm_frame_get_blk_deepth_now(vm);
    int jmp_expect = fast_atoi(data);
    vm->ireg[this_blk_depth] = (pika_bool)!bAssert;

    if (0 == bAssert) {
        /* jump */
        vm->jmp = jmp_expect;
    }

    /* restore loop depth */
    if (2 == jmp_expect && 0 == bAssert) {
        int block_deepth_now = vm_frame_get_blk_deepth_now(vm);
        vm->loop_deepth = block_deepth_now;
    }

    return NULL;
}

Arg *vm_inst_handler_JEZ(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    Arg *aAssert = stack_popArg(&(vm->stack), arg_ret_reg);
    pika_bool bAssert = 0;
    if (NULL != aAssert) {
        PIKA_RES res = _transeBool(aAssert, &bAssert);
        if (PIKA_RES_OK != res) {
            bAssert = 0;
        }
        arg_deinit(aAssert);
    }
    return _VM_JEZ(self, vm, data, arg_ret_reg, bAssert);
}

Arg *vm_inst_handler_JNZ(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    Arg *aAssert = stack_popArg(&(vm->stack), arg_ret_reg);
    pika_bool bAssert = 0;
    if (NULL != aAssert) {
        PIKA_RES res = _transeBool(aAssert, &bAssert);
        if (PIKA_RES_OK != res) {
            bAssert = 0;
        }
        arg_deinit(aAssert);
    }
    return _VM_JEZ(self, vm, data, arg_ret_reg, !bAssert);
}
