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

Arg *vm_inst_handler_DCT(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
#if PIKA_BUILTIN_STRUCT_ENABLE
    uint32_t n_arg = vm_frame_get_input_arg_num(vm);
    PikaObj *dict = New_PikaDict();
    Stack stack = {0};
    stack_init(&stack);
    /* load to local stack to change sort */
    for (int i = 0; i < n_arg; i++) {
        Arg *arg = stack_popArg_alloc(&(vm->stack));
        stack_pushArg(&stack, arg);
    }
    for (int i = 0; i < n_arg / 2; i++) {
        Arg *key_arg = stack_popArg_alloc(&stack);
        Arg *val_arg = stack_popArg_alloc(&stack);
        pikaDict_set(dict, arg_getStr(key_arg), val_arg);
        arg_deinit(key_arg);
    }
    stack_deinit(&stack);
    return arg_newObj(dict);
#else
    return vm_inst_handler_NON(self, vm, data, arg_ret_reg);
#endif
}
