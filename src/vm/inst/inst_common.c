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

int arg_getLen(Arg* self)
{
    if (arg_isObject(self)) {
        return obj_getSize(arg_getPtr(self));
    }
    if (arg_getType(self) == ARG_TYPE_STRING) {
        int strGetSizeUtf8(char* str);
        return strGetSizeUtf8(arg_getStr(self));
    }
    if (arg_getType(self) == ARG_TYPE_BYTES) {
        return arg_getBytesSize(self);
    }
    return -1;
}

Arg *_vm_get(PikaVMFrame* vm, PikaObj* self, Arg *aKey, Arg *aObj)
{
    ArgType eType = arg_getType(aObj);
    Arg *aObjNew = NULL;
    int iIndex = 0;
    int iLen = arg_getLen(aObj);
    if (ARG_TYPE_INT == arg_getType(aKey)) {
        iIndex = arg_getInt(aKey);
    }

    if (iLen != -1) {
        if (iIndex < 0) {
            iIndex += iLen;
            arg_setInt(aKey, "", iIndex);
        }

        if (iIndex >= iLen) {
            vm_frame_set_error_code(vm, PIKA_RES_ERR_OUT_OF_RANGE);
            vm_frame_set_sys_out(vm, "IndexError: index out of range");
            return arg_newNone();
        }
    }

    if (ARG_TYPE_STRING == eType) {
#if PIKA_STRING_UTF8_ENABLE
        pika_bool bIsTemp = pika_false;
        aObjNew = arg_newObj(_arg_to_obj(aObj, &bIsTemp));
        eType = arg_getType(aObjNew);
#else
        char* sPyload = arg_getStr(aObj);
        char sCharBuff[] = " ";
        if (iIndex < 0) {
            iIndex = strGetSize(sPyload) + iIndex;
        }
        sCharBuff[0] = sPyload[iIndex];
        return arg_newStr(sCharBuff);
#endif
    }
    if (ARG_TYPE_BYTES == eType) {
        uint8_t* sBytesPyload = arg_getBytes(aObj);
        uint8_t sByteBuff[] = " ";
        sByteBuff[0] = sBytesPyload[iIndex];
        return arg_newInt(sByteBuff[0]);
    }
    if (argType_isObject(eType)) {
        PikaObj* oArg = NULL;
        Arg *aRes = NULL;
        if (aObjNew != NULL) {
            oArg = arg_getPtr(aObjNew);
        } else {
            oArg = arg_getPtr(aObj);
        }
        obj_setArg(oArg, "__key", aKey);
        /* clang-format off */
        PIKA_PYTHON(
        @res_item = __getitem__(__key)
        )
        /* clang-format on */
        const uint8_t bytes[] = {
            0x0c, 0x00, 0x00, 0x00, /* instruct array size */
            0x10, 0x81, 0x01, 0x00, 0x00, 0x02, 0x07, 0x00, 0x00, 0x04, 0x13,
            0x00,
            /* instruct array */
            0x1d, 0x00, 0x00, 0x00, /* const pool size */
            0x00, 0x5f, 0x5f, 0x6b, 0x65, 0x79, 0x00, 0x5f, 0x5f, 0x67, 0x65,
            0x74, 0x69, 0x74, 0x65, 0x6d, 0x5f, 0x5f, 0x00, 0x40, 0x72, 0x65,
            0x73, 0x5f, 0x69, 0x74, 0x65, 0x6d, 0x00, /* const pool */
        };
        if (NULL != vm) {
            aRes = pikaVM_runByteCode_exReturn(oArg, oArg, oArg,
                                               (uint8_t*)bytes, vm->vm_thread,
                                               pika_true, "@res_item");
        } else {
            aRes = pikaVM_runByteCodeReturn(oArg, (uint8_t*)bytes, "@res_item");
        }
        if (NULL != aObjNew) {
            arg_deinit(aObjNew);
        }
        if (NULL == aRes) {
            if (NULL != vm) {
                vm_frame_set_error_code(vm, PIKA_RES_ERR_ARG_NO_FOUND);
            }
            return arg_newNone();
        }
        return aRes;
    }
    return arg_newNone();
}

#if PIKA_BUILTIN_STRUCT_ENABLE
PikaObj* New_PikaStdData_List(Args* args);
PikaObj* New_PikaStdData_Tuple(Args* args);
#endif

Arg* _vm_create_list_or_tuple(PikaObj* self, PikaVMFrame* vm, pika_bool is_list)
{
#if PIKA_BUILTIN_STRUCT_ENABLE
    NewFun constructor = is_list ? New_PikaStdData_List : New_PikaStdData_Tuple;
    uint32_t n_arg = vm_frame_get_input_arg_num(vm);
    PikaObj* list = newNormalObj(constructor);
    pikaList_init(list);
    Stack stack = {0};
    stack_init(&stack);
    /* load to local stack to change sort */
    for (int i = 0; i < n_arg; i++) {
        Arg* arg = stack_popArg_alloc(&(vm->stack));
        pika_assert(arg != NULL);
        stack_pushArg(&stack, arg);
    }
    for (int i = 0; i < n_arg; i++) {
        Arg* arg = stack_popArg_alloc(&stack);
        pika_assert(arg != NULL);
        pikaList_append(list, arg);
    }
    stack_deinit(&stack);
    return arg_newObj(list);
#else
    return vm_inst_handler_NON(self, vm, "", NULL);
#endif
}

static uint8_t _getLRegIndex(char* data)
{
    return data[2] - '0';
}

static void VMLocals_delLReg(VMLocals* self, uint8_t index)
{
    PikaObj* obj = self->lreg[index];
    if (NULL != obj) {
        obj_enableGC(obj);
        self->lreg[index] = NULL;
        obj_GC(obj);
    }
}

void locals_del_lreg(PikaObj* self, char* name)
{
    if (!locals_check_lreg(name)) {
        return;
    }
    uint8_t reg_index = _getLRegIndex(name);
    VMLocals* locals = obj_getStruct(self, "@l");
    VMLocals_delLReg(locals, reg_index);
}

static void VMLocals_clearReg(VMLocals* self) {
    for (int i = 0; i < PIKA_REGIST_SIZE; i++) {
        VMLocals_delLReg(self, i);
    }
}

PikaObj* locals_get_lreg(PikaObj* self, char* name)
{
    /* get method host obj from reg */
    if (!locals_check_lreg(name)) {
        return NULL;
    }
    uint8_t reg_index = _getLRegIndex(name);
    VMLocals* locals = obj_getStruct(self, "@l");
    return locals->lreg[reg_index];
}

PikaObj* locals_new(Args* args)
{
    PikaObj* self = New_PikaObj();
    self->constructor = locals_new;
#if PIKA_KERNAL_DEBUG_ENABLE
    self->name = "Locals";
#endif
    return self;
}

void locals_deinit(PikaObj* self)
{
    VMLocals* tLocals = obj_getStruct(self, "@l");
    if (NULL == tLocals) {
        return;
    }
    VMLocals_clearReg(tLocals);
}

pika_bool locals_check_lreg(char* data)
{
    if ((data[0] == '$') && (data[1] == 'l') && (data[2] >= '0') &&
        (data[2] <= '9')) {
        return pika_true;
    }
    return pika_false;
}

static void VMLocals_setLReg(VMLocals* self, uint8_t index, PikaObj* obj) {
    obj_refcntInc(obj);
    self->lreg[index] = obj;
}

void locals_set_lreg(PikaObj* self, char* name, PikaObj* obj)
{
    if (!locals_check_lreg(name)) {
        return;
    }
    uint8_t reg_index = _getLRegIndex(name);
    VMLocals* tlocals = obj_getStruct(self, "@l");
    if (NULL == tlocals) {
        /* init locals */
        VMLocals locals = {0};
        obj_setStruct(self, "@l", locals);
        tlocals = obj_getStruct(self, "@l");
    }
    pika_assert(tlocals != NULL);
    obj_setName(obj, name);
    VMLocals_setLReg(tlocals, reg_index, obj);
}