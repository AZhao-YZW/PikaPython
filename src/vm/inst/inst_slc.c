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

#if PIKA_BUILTIN_STRUCT_ENABLE
extern PikaObj *New_PikaStdData_List(Args* args);
extern PikaObj *New_PikaStdData_Tuple(Args* args);
#endif
extern char *string_slice(Args* outBuffs, char *str, int start, int end);

Arg *_vm_slice(PikaVMFrame *vm, PikaObj *self, Arg *aEnd, Arg *aObj, Arg *aStart, int step)
{
#if PIKA_SYNTAX_SLICE_ENABLE
    /* No interger index only support __getitem__ */
    if (!(arg_getType(aStart) == ARG_TYPE_INT &&
          arg_getType(aEnd) == ARG_TYPE_INT)) {
        return _vm_get(vm, self, aStart, aObj);
    }
    int iLen = arg_getLen(aObj);
    int iStart = arg_getInt(aStart);
    int iEnd = arg_getInt(aEnd);

    if (iStart < 0) {
        iStart += iLen;
    }
    /* magic code, to the end */
    if (iEnd == VM_PC_EXIT) {
        iEnd = iLen;
    }
    if (iEnd < 0) {
        iEnd += iLen;
    }

    if (iStart > iLen) {
        iStart = iLen;
    }

    if (iEnd > iLen) {
        iEnd = iLen;
    }

    if (iStart > iEnd) {
        return arg_newStr("");
    }

    /* __slice__ is equal to __getitem__ */
    if (iEnd - iStart == 1 && arg_getType(aObj) != ARG_TYPE_BYTES) {
        return _vm_get(vm, self, aStart, aObj);
    }

    if (ARG_TYPE_STRING == arg_getType(aObj)) {
        Args buffs = {0};
        Arg *aSliced = NULL;
        char *sSliced = string_slice(&buffs, arg_getStr(aObj), iStart, iEnd);
        if (NULL != sSliced) {
            aSliced = arg_newStr(sSliced);
        } else {
            aSliced = arg_newNone();
        }
        strsDeinit(&buffs);
        return aSliced;
    }

    if (ARG_TYPE_BYTES == arg_getType(aObj)) {
        uint8_t* sBytesOrigin = arg_getBytes(aObj);
        Arg *aSliced = arg_newBytes(sBytesOrigin + iStart, iEnd - iStart);
        return aSliced;
    }

    if (arg_isObject(aObj)) {
        PikaObj *oArg = arg_getPtr(aObj);
        PikaObj *New_PikaStdData_List(Args * args);
        PikaObj *New_PikaStdData_Tuple(Args * args);
        if (oArg->constructor == New_PikaStdData_List ||
            oArg->constructor == New_PikaStdData_Tuple) {
            PikaObj *oSliced = newNormalObj((NewFun)oArg->constructor);
            pikaList_init(oSliced);
            for (int i = iStart; i < iEnd; i++) {
                Arg *aIndex = arg_newInt(i);
                Arg *aItem = _vm_get(vm, self, aIndex, aObj);
                pikaList_append(oSliced, aItem);
                arg_deinit(aIndex);
            }
            return arg_newObj(oSliced);
        }
    }
    return arg_newNone();
#else
    return _vm_get(vm, self, aStart, aObj);
#endif
}

Arg *vm_inst_handler_SLC(PikaObj *self, PikaVMFrame *vm, char *data, Arg *arg_ret_reg)
{
#if PIKA_SYNTAX_SLICE_ENABLE
    uint32_t n_input = vm_frame_get_input_arg_num(vm);
    if (n_input < 2) {
        return arg_newNone();
    }
    if (n_input == 2) {
        Arg *key = stack_popArg_alloc(&vm->stack);
        Arg *obj = stack_popArg_alloc(&vm->stack);
        Arg *res = _vm_get(vm, self, key, obj);
        arg_deinit(key);
        arg_deinit(obj);
        return res;
    }
    if (n_input == 3) {
        Arg *end = stack_popArg_alloc(&vm->stack);
        Arg *start = stack_popArg_alloc(&vm->stack);
        if (arg_getType(start) != ARG_TYPE_INT ||
            arg_getType(end) != ARG_TYPE_INT) {
            vm_frame_set_error_code(vm, PIKA_RES_ERR_INVALID_PARAM);
            vm_frame_set_sys_out(vm,
                                  "TypeError: slice indices must be integers");
            arg_deinit(end);
            arg_deinit(start);
            return arg_newNone();
        }
        Arg *obj = stack_popArg_alloc(&vm->stack);
        Arg *res = _vm_slice(vm, self, end, obj, start, 1);
        arg_deinit(end);
        arg_deinit(obj);
        arg_deinit(start);
        return res;
    }
    return arg_newNone();
#else
    Arg *key = stack_popArg_alloc(&vm->stack);
    Arg *obj = stack_popArg_alloc(&vm->stack);
    Arg *res = _vm_get(vm, self, key, obj);
    arg_deinit(key);
    arg_deinit(obj);
    return res;
#endif
}
