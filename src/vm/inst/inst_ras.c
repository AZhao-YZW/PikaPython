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

/* run as */
Arg *vm_inst_handler_RAS(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
    if (strEqu(data, "$origin")) {
        /* use origin object to run */
        obj_removeArg(vm->locals, "@r");
        obj_clearFlag(vm->locals, OBJ_FLAG_RUN_AS);
        return NULL;
    }
    /* use "data" object to run */
    PikaObj* runAs = obj_getObj(vm->locals, data);
    args_setRef(vm->locals->list, "@r", runAs);
    obj_setFlag(vm->locals, OBJ_FLAG_RUN_AS);
    return NULL;
}