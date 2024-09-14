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

static Arg *__VM_instruction_handler_DEF(PikaObj *self, PikaVMFrame *vm, char *data, uint8_t is_class)
{
    int this_blk_depth = vm_frame_get_blk_deepth_now(vm);

    PikaObj *hostObj = vm->locals;
    uint8_t is_in_class = 0;
    /* use RunAs object */
    if (obj_getFlag(vm->locals, OBJ_FLAG_RUN_AS)) {
        hostObj = args_getPtr(vm->locals->list, "@r");
        is_in_class = 1;
    }
    int offset = 0;
    /* byteCode */
    while (1) {
        InstructUnit *ins_unit_now =
            vm_frame_get_inst_unit_with_offset(vm, offset);
        if (!inst_unit_get_is_new_line(ins_unit_now)) {
            offset += inst_unit_get_size();
            continue;
        }
        if (inst_unit_get_blk_deepth(ins_unit_now) == this_blk_depth + 1) {
            if (is_in_class) {
                class_defineObjectMethod(hostObj, data, (Method)ins_unit_now,
                                         self, vm->bytecode_frame);
            } else {
                if (is_class) {
                    class_defineRunTimeConstructor(hostObj, data,
                                                   (Method)ins_unit_now, self,
                                                   vm->bytecode_frame);
                } else {
                    class_defineStaticMethod(hostObj, data,
                                             (Method)ins_unit_now, self,
                                             vm->bytecode_frame);
                }
            }
            break;
        }
        offset += inst_unit_get_size();
    }

    return NULL;
}

Arg *vm_inst_handler_DEF(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    return __VM_instruction_handler_DEF(self, vm, data, 0);
}

Arg *vm_inst_handler_CLS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    return __VM_instruction_handler_DEF(self, vm, data, 1);
}