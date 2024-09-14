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

Arg *vm_inst_handler_NON(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    return NULL;
}

Arg *vm_inst_handler_BYT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    if (strIsContain(data, '\\')) {
        Args buffs = {0};
        size_t i_out = 0;
        char *transfered_str = strsTransfer(&buffs, data, &i_out);
        Arg *return_arg = New_arg(NULL);
        return_arg = arg_setBytes(return_arg, "", (uint8_t*)transfered_str, i_out);
        strsDeinit(&buffs);
        return return_arg;
    }
    return arg_newBytes((uint8_t*)data, strGetSize(data));
}

Arg *vm_inst_handler_EXP(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    vm->vm_thread->try_error_code = 0;
    return NULL;
}

Arg *vm_inst_handler_GER(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    PIKA_RES err = (PIKA_RES)vm->vm_thread->try_error_code;
    Arg *err_arg = arg_newInt(err);
    return err_arg;
}

Arg *vm_inst_handler_JMP(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    vm->jmp = fast_atoi(data);
    return NULL;
}

Arg *vm_inst_handler_NTR(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    vm->vm_thread->try_state = TRY_STATE_NONE;
    return NULL;
}

Arg *vm_inst_handler_RET(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    /* exit jmp signal */
    vm->jmp = VM_JMP_EXIT;
    Arg *aReturn = stack_popArg_alloc(&(vm->stack));
    /* set gc root to avoid gc */
    arg_setObjFlag(aReturn, OBJ_FLAG_GC_ROOT);
    method_returnArg(vm->locals->list, aReturn);
    return NULL;
}

Arg *vm_inst_handler_STR(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    if (NULL == data) {
        return arg_setStr(arg_ret_reg, "", "");
    }
    if (strIsContain(data, '\\')) {
        Args buffs = {0};
        size_t i_out = 0;
        char *transfered_str = strsTransfer(&buffs, data, &i_out);
        Arg *return_arg = arg_ret_reg;
        return_arg = arg_setStr(return_arg, "", transfered_str);
        strsDeinit(&buffs);
        return return_arg;
    }
    return arg_setStr(arg_ret_reg, "", data);
}

Arg *vm_inst_handler_TRY(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    pika_assert(NULL != vm->vm_thread);
    vm->vm_thread->try_state = TRY_STATE_INNER;
    return NULL;
}

Arg *vm_inst_handler_SER(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    vm->vm_thread->try_error_code = fast_atoi(data);
    return NULL;
}

Arg *vm_inst_handler_NEL(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    int this_blk_depth = vm_frame_get_blk_deepth_now(vm);
    if (0 == vm->ireg[this_blk_depth]) {
        /* set __else flag */
        vm->jmp = fast_atoi(data);
    }
    return NULL;
}

Arg *vm_inst_handler_EST(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    Arg *arg = obj_getArg(vm->locals, data);
    if (arg == NULL) {
        return arg_setInt(arg_ret_reg, "", 0);
    }
    if (ARG_TYPE_NONE == arg_getType(arg)) {
        return arg_setInt(arg_ret_reg, "", 0);
    }
    return arg_setInt(arg_ret_reg, "", 1);
}

Arg *vm_inst_handler_BRK(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    /* break jmp signal */
    vm->jmp = VM_JMP_BREAK;
    return NULL;
}

Arg *vm_inst_handler_CTN(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    /* continue jmp signal */
    vm->jmp = VM_JMP_CONTINUE;
    return NULL;
}