/*
 * This file is part of the PikaPython project.
 * http://github.com/pikastech/pikapython
 *
 * MIT License
 *
 * Copyright (c) 2021 lyon liang6516@outlook.com
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

#include "PikaVM.h"
#include "BaseObj.h"
#include "PikaCompiler.h"
#include "PikaObj.h"
#include "PikaParser.h"
#include "PikaPlatform.h"
#include "dataArg.h"
#include "dataStrs.h"
#include "vm.h"
#include "inst.h"
#include "vm_frame.h"
#if PIKA_MATH_ENABLE
#include <math.h>
#endif

extern VMParameters* _pikaVM_runByteCodeFrameWithState(PikaObj* self, VMParameters* locals,
    VMParameters* globals, ByteCodeFrame *bytecode_frame, uint16_t pc, PikaVMThread* vm_thread);

#if PIKA_SETJMP_ENABLE

typedef struct JmpBufCQ {
    jmp_buf* buf[PIKA_JMP_BUF_LIST_SIZE];
    int head;
    int tail;
} JmpBufCQ;

static pika_bool _jcq_isEmpty(volatile JmpBufCQ* cq) {
    return (pika_bool)(cq->head == cq->tail);
}

static pika_bool _jcq_isFull(volatile JmpBufCQ* cq) {
    return (pika_bool)((cq->tail + 1) % PIKA_JMP_BUF_LIST_SIZE == cq->head);
}

static jmp_buf* _jcq_pop(volatile JmpBufCQ* cq) {
    if (_jcq_isEmpty(cq)) {
        return NULL;
    }
    jmp_buf* buf = cq->buf[cq->head];
    cq->head = (cq->head + 1) % PIKA_JMP_BUF_LIST_SIZE;
    return buf;
}

static jmp_buf* _jcq_check_pop(volatile JmpBufCQ* cq) {
    if (_jcq_isEmpty(cq)) {
        return NULL;
    }
    return cq->buf[cq->head];
}

static PIKA_RES _jcq_push(volatile JmpBufCQ* cq, jmp_buf* pos) {
    if (_jcq_isFull(cq)) {
        return -1;
    }
    cq->buf[cq->tail] = pos;
    cq->tail = (cq->tail + 1) % PIKA_JMP_BUF_LIST_SIZE;
    return PIKA_RES_OK;
}

static PIKA_RES _jcq_remove(volatile JmpBufCQ* cq, jmp_buf* pos) {
    if (_jcq_isEmpty(cq)) {
        return -1;
    }
    for (int i = cq->head; i != cq->tail;
         i = (i + 1) % PIKA_JMP_BUF_LIST_SIZE) {
        if (cq->buf[i] == pos) {
            /* move */
            for (int j = i; j != cq->tail;
                 j = (j + 1) % PIKA_JMP_BUF_LIST_SIZE) {
                cq->buf[j] = cq->buf[(j + 1) % PIKA_JMP_BUF_LIST_SIZE];
            }
            cq->tail = (cq->tail - 1) % PIKA_JMP_BUF_LIST_SIZE;
            return PIKA_RES_OK;
        }
    }
    return -1;
}
#endif

Arg* _get_return_arg(PikaObj* locals) {
    Arg* res = args_getArg(locals->list, (char*)"@rt");
    args_removeArg_notDeinitArg(locals->list, res);
    return res;
}

Arg* _obj_runMethodArgWithState(PikaObj* self,
                                PikaObj* locals,
                                Arg* aMethod,
                                PikaVMThread* vm_thread,
                                Arg* aReturnCache) {
    pika_assert(NULL != vm_thread);
    Arg* aReturn = NULL;
    /* get method Ptr */
    Method fMethod = methodArg_getPtr(aMethod);
    /* get method type list */
    ArgType methodType = arg_getType(aMethod);
    /* error */
    if (ARG_TYPE_NONE == methodType) {
        return NULL;
    }

    /* redirect to def context */
    if (!argType_isNative(methodType)) {
        self = methodArg_getDefContext(aMethod);
    }

    /* run method */
    if (argType_isNative(methodType)) {
        /* native method */
        PikaObj* oHost = self;
        if (methodType == ARG_TYPE_METHOD_NATIVE_ACTIVE) {
            oHost = methodArg_getHostObj(aMethod);
        }
        fMethod(oHost, locals->list);
        /* get method return */
        aReturn = _get_return_arg(locals);
    } else {
        /* static method and object method */
        /* byteCode */
        ByteCodeFrame *method_bytecodeFrame =
            methodArg_getBytecodeFrame(aMethod);
        uintptr_t insturctArray_start = (uintptr_t)inst_array_get_by_offset(
            &(method_bytecodeFrame->instruct_array), 0);
        uint16_t pc = (uintptr_t)fMethod - insturctArray_start;
        locals = _pikaVM_runByteCodeFrameWithState(
            self, locals, self, method_bytecodeFrame, pc, vm_thread);

        /* get method return */
        aReturn = _get_return_arg(locals);
    }
#if PIKA_TYPE_FULL_FEATURE_ENABLE
    PikaObj* oReturn = NULL;
    if (arg_isConstructor(aMethod)) {
        if (arg_isObject(aReturn)) {
            oReturn = arg_getPtr(aReturn);
            obj_setArg(oReturn, "__class__", aMethod);
        }
    }
#endif
    pika_assert_arg_alive(aReturn);
    return aReturn;
}

Arg* obj_runMethodArgWithState(PikaObj* self,
                               PikaObj* method_args_obj,
                               Arg* method_arg,
                               PikaVMThread* vm_thread) {
    return _obj_runMethodArgWithState(self, method_args_obj, method_arg,
                                      vm_thread, NULL);
}

Arg* obj_runMethodArgWithState_noalloc(PikaObj* self,
                                       PikaObj* locals,
                                       Arg* method_arg,
                                       PikaVMThread* vm_thread,
                                       Arg* ret_arg_reg) {
    return _obj_runMethodArgWithState(self, locals, method_arg, vm_thread,
                                      ret_arg_reg);
}

Arg* obj_runMethodArg(PikaObj* self,
                      PikaObj* method_args_obj,
                      Arg* method_arg) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    return obj_runMethodArgWithState(self, method_args_obj, method_arg,
                                     &vm_thread);
}

#if PIKA_BUILTIN_STRUCT_ENABLE
PikaObj* New_PikaStdData_Dict(Args* args);
#endif