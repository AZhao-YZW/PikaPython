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
#include "frame.h"
#if PIKA_MATH_ENABLE
#include <math.h>
#endif

static pika_thread_recursive_mutex_t g_pikaGIL = {0};
volatile VMState g_PikaVMState = {
    .signal_ctrl = VM_SIGNAL_CTRL_NONE,
    .vm_cnt = 0,
#if PIKA_EVENT_ENABLE
    .cq =
        {
            .head = 0,
            .tail = 0,
            .res = {0},
        },
    .sq =
        {
            .head = 0,
            .tail = 0,
            .res = {0},
        },
    .event_pickup_cnt = 0,
    .event_thread = NULL,
    .event_thread_exit = pika_false,
#endif
#if PIKA_DEBUG_BREAK_POINT_MAX > 0
    .break_module_hash = {0},
    .break_point_pc = {0},
    .break_point_cnt = 0,
#endif
};
extern volatile PikaObjState g_PikaObjState;

pika_bool pika_GIL_isInit(void) {
    return g_pikaGIL.mutex.is_init;
}

int pika_GIL_ENTER(void) {
    if (!g_pikaGIL.mutex.is_init) {
        if (g_pikaGIL.mutex.bare_lock > 0) {
            return -1;
        }
        g_pikaGIL.mutex.bare_lock = 1;
        return 0;
    }
    int ret = pika_thread_recursive_mutex_lock(&g_pikaGIL);
    // pika_debug("pika_GIL_ENTER");
    if (!g_pikaGIL.mutex.is_first_lock) {
        g_pikaGIL.mutex.is_first_lock = 1;
    }
    return ret;
}

int pika_GIL_getBareLock(void) {
    return g_pikaGIL.mutex.bare_lock;
}

int pika_GIL_EXIT(void) {
    if (!g_pikaGIL.mutex.is_first_lock || !g_pikaGIL.mutex.is_init) {
        g_pikaGIL.mutex.bare_lock = 0;
        return 0;
    }
    return pika_thread_recursive_mutex_unlock(&g_pikaGIL);
}

int pika_GIL_deinit(void) {
    pika_platform_memset(&g_pikaGIL, 0, sizeof(g_pikaGIL));
    return 0;
}

int _VM_lock_init(void) {
    if (g_pikaGIL.mutex.is_init) {
        return 0;
    }
    int ret = pika_thread_recursive_mutex_init(&g_pikaGIL);
    if (0 == ret) {
        g_pikaGIL.mutex.is_init = 1;
    }
    return ret;
}

int _VM_is_first_lock(void) {
    return g_pikaGIL.mutex.is_first_lock;
}

int _VMEvent_getVMCnt(void) {
    return g_PikaVMState.vm_cnt;
}

int _VMEvent_getEventPickupCnt(void) {
#if !PIKA_EVENT_ENABLE
    return -1;
#else
    return g_PikaVMState.event_pickup_cnt;
#endif
}

#if PIKA_EVENT_ENABLE
static pika_bool _ecq_isEmpty(volatile PikaEventQueue* cq) {
    return (pika_bool)(cq->head == cq->tail);
}

static pika_bool _ecq_isFull(volatile PikaEventQueue* cq) {
    return (pika_bool)((cq->tail + 1) % PIKA_EVENT_LIST_SIZE == cq->head);
}
#endif

#if PIKA_SETJMP_ENABLE

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

void _VMEvent_deinit(void) {
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
#else
    for (int i = 0; i < PIKA_EVENT_LIST_SIZE; i++) {
        poiner_set_NULL((void**)&g_PikaVMState.cq.res[i]);
        poiner_set_NULL((void**)&g_PikaVMState.cq.data[i].arg);
        poiner_set_NULL((void**)&g_PikaVMState.sq.res[i]);
        g_PikaVMState.cq.id[i] = 0;
        g_PikaVMState.cq.listener[i] = NULL;
        g_PikaVMState.sq.id[i] = 0;
        g_PikaVMState.sq.data[i].signal = 0;
        g_PikaVMState.sq.listener[i] = NULL;
    }
    g_PikaVMState.cq.head = 0;
    g_PikaVMState.cq.tail = 0;
    g_PikaVMState.sq.head = 0;
    g_PikaVMState.sq.tail = 0;
    if (NULL != g_PikaVMState.event_thread) {
        g_PikaVMState.event_thread_exit = 1;
        pika_platform_thread_destroy(g_PikaVMState.event_thread);
        g_PikaVMState.event_thread = NULL;
        pika_GIL_EXIT();
        while (!g_PikaVMState.event_thread_exit_done) {
            pika_platform_thread_yield();
        }
        g_PikaVMState.event_thread_exit = 0;
        g_PikaVMState.event_thread_exit_done = 0;
        pika_GIL_ENTER();
    }
#endif
}

PIKA_RES __eventListener_pushEvent(PikaEventListener* lisener,
                                   uintptr_t eventId,
                                   Arg* eventData) {
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    if (arg_getType(eventData) == ARG_TYPE_OBJECT_NEW) {
        arg_setType(eventData, ARG_TYPE_OBJECT);
    }
    /* push to event_cq_buff */
    if (_ecq_isFull(&g_PikaVMState.cq)) {
        // pika_debug("event_cq_buff is full");
        arg_deinit(eventData);
        return PIKA_RES_ERR_SIGNAL_EVENT_FULL;
    }
    poiner_set_NULL((void**)&g_PikaVMState.cq.res[g_PikaVMState.cq.tail]);
    poiner_set_NULL((void**)&g_PikaVMState.cq.data[g_PikaVMState.cq.tail].arg);
    g_PikaVMState.cq.id[g_PikaVMState.cq.tail] = eventId;
    g_PikaVMState.cq.data[g_PikaVMState.cq.tail].arg = eventData;
    g_PikaVMState.cq.listener[g_PikaVMState.cq.tail] = lisener;
    g_PikaVMState.cq.tail = (g_PikaVMState.cq.tail + 1) % PIKA_EVENT_LIST_SIZE;
    return PIKA_RES_OK;
#endif
}

PIKA_RES __eventListener_pushSignal(PikaEventListener* lisener,
                                    uintptr_t eventId,
                                    int signal) {
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    /* push to event_cq_buff */
    if (_ecq_isFull(&g_PikaVMState.sq)) {
        // pika_debug("event_cq_buff is full");
        return PIKA_RES_ERR_SIGNAL_EVENT_FULL;
    }
    g_PikaVMState.sq.id[g_PikaVMState.sq.tail] = eventId;
    g_PikaVMState.sq.data[g_PikaVMState.sq.tail].signal = signal;
    g_PikaVMState.sq.listener[g_PikaVMState.sq.tail] = lisener;
    g_PikaVMState.sq.tail = (g_PikaVMState.sq.tail + 1) % PIKA_EVENT_LIST_SIZE;
    if (_VM_is_first_lock()) {
        pika_GIL_ENTER();
        poiner_set_NULL((void**)&g_PikaVMState.sq.res[g_PikaVMState.sq.tail]);
        pika_GIL_EXIT();
    }
    return PIKA_RES_OK;
#endif
}

PIKA_RES __eventListener_popEvent(PikaEventListener** lisener_p,
                                  uintptr_t* id,
                                  Arg** data,
                                  int* signal,
                                  int* head) {
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    PikaEventQueue* cq = NULL;
    /* pop from event_cq_buff */
    if (!_ecq_isEmpty(&g_PikaVMState.cq)) {
        cq = (PikaEventQueue*)&g_PikaVMState.cq;
    } else if (!_ecq_isEmpty(&g_PikaVMState.sq)) {
        cq = (PikaEventQueue*)&g_PikaVMState.sq;
    }
    if (NULL == cq) {
        return PIKA_RES_ERR_SIGNAL_EVENT_EMPTY;
    }
    *id = cq->id[g_PikaVMState.cq.head];
    if (cq == &g_PikaVMState.cq) {
        *data = cq->data[g_PikaVMState.cq.head].arg;
    } else {
        *signal = cq->data[g_PikaVMState.cq.head].signal;
        *data = NULL;
    }
    *lisener_p = cq->listener[g_PikaVMState.cq.head];
    *head = cq->head;
    cq->head = (cq->head + 1) % PIKA_EVENT_LIST_SIZE;
    return PIKA_RES_OK;
#endif
}

PIKA_RES __eventListener_popSignalEvent(PikaEventListener** lisener_p,
                                        uintptr_t* id,
                                        int* signal,
                                        int* head) {
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
    return PIKA_RES_ERR_OPERATION_FAILED;
#else
    /* pop from event_cq_buff */
    if (_ecq_isEmpty(&g_PikaVMState.sq)) {
        return PIKA_RES_ERR_SIGNAL_EVENT_EMPTY;
    }
    *id = g_PikaVMState.sq.id[g_PikaVMState.sq.head];
    *signal = g_PikaVMState.sq.data[g_PikaVMState.sq.head].signal;
    *lisener_p = g_PikaVMState.sq.listener[g_PikaVMState.sq.head];
    *head = g_PikaVMState.sq.head;
    g_PikaVMState.sq.head = (g_PikaVMState.sq.head + 1) % PIKA_EVENT_LIST_SIZE;
    return PIKA_RES_OK;
#endif
}

void __VMEvent_pickupEvent(char* info) {
#if !PIKA_EVENT_ENABLE
    pika_event_not_enable();
#else
    int evt_pickup_cnt = _VMEvent_getEventPickupCnt();
    if (evt_pickup_cnt >= PIKA_EVENT_PICKUP_MAX) {
        return;
    }
    PikaObj* event_listener;
    uintptr_t event_id;
    Arg* event_data;
    int signal;
    int head;
    if (PIKA_RES_OK == __eventListener_popEvent(&event_listener, &event_id,
                                                &event_data, &signal, &head)) {
        g_PikaVMState.event_pickup_cnt++;
        pika_debug("pickup_info: %s", info);
        pika_debug("pickup_cnt: %d", g_PikaVMState.event_pickup_cnt);
        Arg* res = NULL;
        if (NULL != event_data) {
            res =
                __eventListener_runEvent(event_listener, event_id, event_data);
        } else {
            event_data = arg_newInt(signal);
            res =
                __eventListener_runEvent(event_listener, event_id, event_data);
            arg_deinit(event_data);
            event_data = NULL;
        }
        // only on muti thread, keep the res
        if (_VM_is_first_lock()) {
            if (NULL == event_data) {
                // from signal
                g_PikaVMState.sq.res[head] = res;
            } else {
                // from event
                g_PikaVMState.cq.res[head] = res;
            }
        } else {
            if (NULL != res) {
                arg_deinit(res);
            }
        }
        g_PikaVMState.event_pickup_cnt--;
    }
#endif
}

VM_SIGNAL_CTRL VMSignal_getCtrl(void) {
    return g_PikaVMState.signal_ctrl;
}

void pika_vm_exit(void) {
    g_PikaVMState.signal_ctrl = VM_SIGNAL_CTRL_EXIT;
}

void pika_vm_exit_await(void) {
    pika_vm_exit();
    while (1) {
        pika_platform_thread_yield();
        if (_VMEvent_getVMCnt() == 0) {
            return;
        }
    }
}

void pika_vmSignal_setCtrlClear(void) {
    g_PikaVMState.signal_ctrl = VM_SIGNAL_CTRL_NONE;
}

/* head declare start */
static VMParameters* __pikaVM_runByteCodeFrameWithState(
    PikaObj* self,
    VMParameters* locals,
    VMParameters* globals,
    ByteCodeFrame* bytecode_frame,
    uint16_t pc,
    PikaVMThread* vm_thread,
    pika_bool in_repl);

VMParameters* _pikaVM_runByteCodeFrameWithState(PikaObj* self, VMParameters* locals,
    VMParameters* globals, ByteCodeFrame* bytecode_frame, uint16_t pc, PikaVMThread* vm_thread);

/* head declare end */

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
        ByteCodeFrame* method_bytecodeFrame =
            methodArg_getBytecodeFrame(aMethod);
        uintptr_t insturctArray_start = (uintptr_t)instructArray_getByOffset(
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

InstructUnit* byteCodeFrame_findInstructUnit(ByteCodeFrame* bcframe,
                                             int32_t iPcStart,
                                             enum InstructIndex index,
                                             int32_t* iOffset_p,
                                             pika_bool bIsForward) {
    /* find instruct unit */
    int instructArray_size = instructArray_getSize(&(bcframe->instruct_array));
    while (1) {
        *iOffset_p += (bIsForward ? -1 : 1) * instructUnit_getSize();
        if (iPcStart + *iOffset_p >= instructArray_size) {
            return NULL;
        }
        if (iPcStart + *iOffset_p < 0) {
            return NULL;
        }
        InstructUnit* unit = instructArray_getByOffset(
            &(bcframe->instruct_array), iPcStart + *iOffset_p);
        if (index == instructUnit_getInstructIndex(unit)) {
            return unit;
        }
    }
}

InstructUnit* byteCodeFrame_findInsForward(ByteCodeFrame* bcframe,
                                           int32_t pc_start,
                                           enum InstructIndex index,
                                           int32_t* p_offset) {
    return byteCodeFrame_findInstructUnit(bcframe, pc_start, index, p_offset,
                                          pika_true);
}

Hash byteCodeFrame_getNameHash(ByteCodeFrame* bcframe) {
    if (0 == bcframe->name_hash) {
        bcframe->name_hash = hash_time33(bcframe->name);
    }
    return bcframe->name_hash;
}

InstructUnit* byteCodeFrame_findInsUnitBackward(ByteCodeFrame* bcframe,
                                                int32_t pc_start,
                                                enum InstructIndex index,
                                                int32_t* p_offset) {
    return byteCodeFrame_findInstructUnit(bcframe, pc_start, index, p_offset,
                                          pika_false);
}

char* _find_super_class_name(ByteCodeFrame* bcframe, int32_t pc_start) {
    /* find super class */
    int32_t offset = 0;
    char* super_class_name = NULL;
    byteCodeFrame_findInsForward(bcframe, pc_start, PIKA_INS(CLS), &offset);
    InstructUnit* unit_run = byteCodeFrame_findInsUnitBackward(
        bcframe, pc_start, PIKA_INS(RUN), &offset);
    super_class_name = constPool_getByOffset(
        &(bcframe->const_pool), instructUnit_getConstPoolIndex(unit_run));
    return super_class_name;
}

PikaObj* New_builtins_object(Args* args);

#if PIKA_INSTRUCT_EXTENSION_ENABLE
const VMInstructionSet VM_default_instruction_set = {
#define __INS_OPCODE
    .instructions =
        (const VMInstruction[]){
#include "__instruction_table.h"
        },
    .count = __INSTRUCTION_CNT,
    .op_idx_start = PIKA_INS(NON),
    .op_idx_end = PIKA_INS(NON) + __INSTRUCTION_CNT - 1,
};

#ifndef PIKA_INSTRUCT_SIGNATURE_DICT
#define PIKA_INSTRUCT_SIGNATURE_DICT 0
#endif

typedef struct VMInstructionSetItem VMInstructionSetItem;
struct VMInstructionSetItem {
    VMInstructionSetItem* next;
    const VMInstructionSet* ins_set;
};

static struct {
    const VMInstructionSetItem default_ins_set;
    VMInstructionSetItem* list;
    VMInstructionSetItem* recent;
#if PIKA_INSTRUCT_SIGNATURE_DICT_COUNT > 0
    const uint16_t signature_dict[PIKA_INSTRUCT_SIGNATURE_DICT_COUNT];
#endif
} g_PikaVMInsSet = {
    .default_ins_set =
        {
            .ins_set = &VM_default_instruction_set,
            .next = NULL,
        },
    .list = (VMInstructionSetItem*)&g_PikaVMInsSet.default_ins_set,
    .recent = (VMInstructionSetItem*)&g_PikaVMInsSet.default_ins_set,
#if PIKA_INSTRUCT_SIGNATURE_DICT_COUNT > 0
    .signature_dict = {PIKA_INSTRUCT_SIGNATURE_DICT},
#endif
};

pika_bool pikaVM_registerInstructionSet(VMInstructionSet* ins_set) {
    pika_assert(NULL != ins_set);

#if PIKA_INSTRUCT_SIGNATURE_DICT_COUNT > 0
    uint16_t signature = ins_set->signature;

    pika_bool ins_set_valid = pika_false;
    for (int n = 0;
         n < sizeof(g_PikaVMInsSet.signature_dict) / sizeof(uint16_t); n++) {
        if (g_PikaVMInsSet.signature_dict[n] == signature) {
            ins_set_valid = pika_true;
            break;
        }
    }
    if (!ins_set_valid) {
        return pika_false;
    }
#endif

    /* check whether the target instruction set exists or not */
    VMInstructionSetItem* list_item = g_PikaVMInsSet.list;
    do {
        if (list_item->ins_set->signature == signature) {
            return pika_true; /* already exist */
        }

        list_item = list_item->next;
    } while (NULL != list_item->next);

    VMInstructionSetItem* item =
        pika_platform_malloc(sizeof(VMInstructionSetItem));
    if (NULL == item) {
        return pika_false;
    }
    item->ins_set = ins_set;
    item->next = NULL;

    /* add item to the tail of VM.list */
    list_item->next = item;

    return pika_true;
}

static const VMInstruction* instructUnit_getInstruct(
    enum InstructIndex ins_idx) {
    VMInstructionSetItem* item = g_PikaVMInsSet.recent;

    if ((ins_idx >= item->ins_set->op_idx_start) &&
        (ins_idx <= item->ins_set->op_idx_end)) {
        return &(
            item->ins_set->instructions[ins_idx - item->ins_set->op_idx_start]);
    }

    /* search list */
    item = g_PikaVMInsSet.list;
    do {
        if ((ins_idx >= item->ins_set->op_idx_start) &&
            (ins_idx <= item->ins_set->op_idx_end)) {
            g_PikaVMInsSet.recent = item;
            return &(item->ins_set
                         ->instructions[ins_idx - item->ins_set->op_idx_start]);
        }
        item = item->next;
    } while (NULL != item->next);

    return NULL;
}

static enum InstructIndex __find_ins_idx_in_ins_set(
    char* ins_str,
    const VMInstructionSet* set) {
    const VMInstruction* ins = set->instructions;
    uint_fast16_t count = set->count;

    do {
        if (0 == strncmp(ins_str, ins->op_str, ins->op_str_len)) {
            return (enum InstructIndex)ins->op_idx;
        }
        ins++;
    } while (--count);
    return __INSTRUCTION_UNKNOWN;
}

enum InstructIndex pikaVM_getInstructFromAsm(char* ins_str) {
    enum InstructIndex ins_idx =
        __find_ins_idx_in_ins_set(ins_str, g_PikaVMInsSet.recent->ins_set);

    if (__INSTRUCTION_UNKNOWN == ins_idx) {
        VMInstructionSetItem* item = g_PikaVMInsSet.list;

        do {
            ins_idx = __find_ins_idx_in_ins_set(ins_str, item->ins_set);
            if (__INSTRUCTION_UNKNOWN != ins_idx) {
                g_PikaVMInsSet.recent = item;
                return ins_idx;
            }
            item = item->next;
        } while (NULL != item->next);

        return PIKA_INS(NON);
    }

    return ins_idx;
}

#else

pika_bool pikaVM_registerInstructionSet(VMInstructionSet* ins_set) {
    return pika_false;
}

enum InstructIndex pikaVM_getInstructFromAsm(char* ins_str) {
#define __INS_COMPARE
#include "__instruction_table.h"
    return NON;
}

const VM_instruct_handler VM_instruct_handler_table[__INSTRUCTION_CNT] = {
#define __INS_TABLE
#include "__instruction_table.h"
};
#endif

extern volatile PikaObj* __pikaMain;
static enum shellCTRL __obj_shellLineHandler_debug(PikaObj* self,
                                                   char* input_line,
                                                   struct ShellConfig* config) {
    /* continue */
    if (strEqu("c", input_line)) {
        return SHELL_CTRL_EXIT;
    }
    /* next */
    if (strEqu("n", input_line)) {
        return SHELL_CTRL_EXIT;
    }
    /* launch shell */
    if (strEqu("sh", input_line)) {
        /* exit pika shell */
        pikaScriptShell((PikaObj*)__pikaMain);
        return SHELL_CTRL_CONTINUE;
    }
    /* quit */
    if (strEqu("q", input_line)) {
        obj_setInt(self, "enable", 0);
        return SHELL_CTRL_EXIT;
    }
    /* print */
    if (strIsStartWith(input_line, "p ")) {
        char* path = input_line + 2;
        Arg* asm_buff = arg_newStr("print(");
        asm_buff = arg_strAppend(asm_buff, path);
        asm_buff = arg_strAppend(asm_buff, ")\n");
        pikaVM_run_ex_cfg cfg = {0};
        cfg.globals = config->globals;
        cfg.in_repl = pika_true;
        pikaVM_run_ex(config->locals, arg_getStr(asm_buff), &cfg);
        arg_deinit(asm_buff);
        return SHELL_CTRL_CONTINUE;
    }
    pikaVM_run_ex_cfg cfg = {0};
    cfg.globals = config->globals;
    cfg.in_repl = pika_true;
    pikaVM_run_ex(config->locals, input_line, &cfg);
    return SHELL_CTRL_CONTINUE;
}

void pika_debug_set_trace(PikaObj* self) {
    if (!obj_getInt(self, "enable")) {
        return;
    }
    char* name = "stdin";
    pika_assert(NULL != self->vmFrame);
    if (NULL != self->vmFrame->bytecode_frame->name) {
        name = self->vmFrame->bytecode_frame->name;
    }
    pika_platform_printf("%s:%d\n", name, self->vmFrame->pc);
    struct ShellConfig cfg = {
        .prefix = "(Pdb-pika) ",
        .handler = __obj_shellLineHandler_debug,
        .fn_getchar = __platform_getchar,
        .locals = self->vmFrame->locals,
        .globals = self->vmFrame->globals,
    };
    _do_pikaScriptShell(self, &cfg);
    shConfig_deinit(&cfg);
}

static int pikaVM_runInstructUnit(PikaObj* self,
                                  PikaVMFrame* vm,
                                  InstructUnit* ins_unit) {
    enum InstructIndex instruct = instructUnit_getInstructIndex(ins_unit);
    arg_newReg(ret_reg, PIKA_ARG_BUFF_SIZE);
    Arg* return_arg = &ret_reg;
    // char invode_deepth1_str[2] = {0};
    int32_t pc_next = vm->pc + instructUnit_getSize();
    char* data = PikaVMFrame_getConstWithInstructUnit(vm, ins_unit);
    /* run instruct */
    pika_assert(NULL != vm->vm_thread);
    if (vm_frame_check_break_point(vm)) {
        pika_debug_set_trace(self);
    }

#if PIKA_INSTRUCT_EXTENSION_ENABLE
    const VMInstruction* ins = instructUnit_getInstruct(instruct);
    if (NULL == ins) {
        /* todo: unsupported instruction */
        pika_assert(NULL != ins);
    }
    pika_assert(NULL != ins->handler);

    return_arg = ins->handler(self, vm, data, &ret_reg);
#else
    return_arg = VM_instruct_handler_table[instruct](self, vm, data, &ret_reg);
#endif

    if (vm->vm_thread->error_code != PIKA_RES_OK ||
        VMSignal_getCtrl() == VM_SIGNAL_CTRL_EXIT) {
        /* raise jmp */
        if (vm->vm_thread->try_state == TRY_STATE_INNER) {
            vm->jmp = VM_JMP_RAISE;
        } else {
            /* exit */
            vm->jmp = VM_JMP_EXIT;
        }
    }

#if PIKA_BUILTIN_STRUCT_ENABLE
    int invoke_deepth = vm_frame_get_invoke_deepth_now(vm);
    if (invoke_deepth > 0) {
        PikaObj* oReg = vm->oreg[invoke_deepth - 1];
        if (NULL != oReg && NULL != return_arg) {
            pikaList_append(oReg, return_arg);
            return_arg = NULL;
        }
    }
#endif

    if (NULL != return_arg) {
        stack_pushArg(&(vm->stack), return_arg);
    }
    goto __next_line;
__next_line:
    /* exit */
    if (VM_JMP_EXIT == vm->jmp) {
        pc_next = VM_PC_EXIT;
        goto __exit;
    }
    /* break */
    if (VM_JMP_BREAK == vm->jmp) {
        pc_next = vm->pc + vm_frame_get_break_addr_offset(vm);
        goto __exit;
    }
    /* continue */
    if (VM_JMP_CONTINUE == vm->jmp) {
        pc_next = vm->pc + vm_frame_get_continue_addr_offset(vm);
        goto __exit;
    }
/* raise */
#if !PIKA_NANO_ENABLE
    if (VM_JMP_RAISE == vm->jmp) {
        int offset = vm_frame_get_raise_addr_offset(vm);
        if (0 == offset) {
            /* can not found end of try, return */
            pc_next = VM_PC_EXIT;
            vm->vm_thread->try_result = TRY_RESULT_RAISE;
            goto __exit;
        }
        pc_next = vm->pc + offset;
        vm->vm_thread->try_result = TRY_RESULT_NONE;
        goto __exit;
    }
#endif
    /* static jmp */
    if (vm->jmp != 0) {
        pc_next = vm->pc + vm_frame_get_jmp_addr_offset(vm);
        goto __exit;
    }
    /* not jmp */
    pc_next = vm->pc + instructUnit_getSize();

    /* jump to next line */
    if (vm->vm_thread->error_code != 0) {
        while (1) {
            if (pc_next >= (int)PikaVMFrame_getInstructArraySize(vm)) {
                pc_next = VM_PC_EXIT;
                goto __exit;
            }
            InstructUnit* ins_next = instructArray_getByOffset(
                &vm->bytecode_frame->instruct_array, pc_next);
            if (instructUnit_getIsNewLine(ins_next)) {
                goto __exit;
            }
            pc_next = pc_next + instructUnit_getSize();
        }
    }

    goto __exit;
__exit:
    vm->jmp = 0;
    /* reach the end */
    if (pc_next >= (int)PikaVMFrame_getInstructArraySize(vm)) {
        return VM_PC_EXIT;
    }
    return pc_next;
}

VMParameters* pikaVM_runAsm(PikaObj* self, char* pikaAsm) {
    ByteCodeFrame bytecode_frame;
    byteCodeFrame_init(&bytecode_frame);
    byteCodeFrame_appendFromAsm(&bytecode_frame, pikaAsm);
    VMParameters* res = pikaVM_runByteCodeFrame(self, &bytecode_frame);
    byteCodeFrame_deinit(&bytecode_frame);
    return res;
}

static ByteCodeFrame* _cache_bytecodeframe(PikaObj* self) {
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame* res = NULL;
    if (!obj_isArgExist(self, "@bcn")) {
        /* start form @bc0 */
        obj_setInt(self, "@bcn", 0);
    }
    int bcn = obj_getInt(self, "@bcn");
    char bcn_str[] = "@bcx";
    bcn_str[3] = '0' + bcn;
    /* load bytecode to heap */
    args_setHeapStruct(self->list, bcn_str, bytecode_frame_stack,
                       byteCodeFrame_deinit);
    /* get bytecode_ptr from heap */
    res = args_getHeapStruct(self->list, bcn_str);
    obj_setInt(self, "@bcn", bcn + 1);
    return res;
}

static ByteCodeFrame* _cache_bcf_fn(PikaObj* self, char* py_lines) {
    /* cache 'def' and 'class' to heap */
    if ((NULL == strstr(py_lines, "def ")) &&
        (NULL == strstr(py_lines, "class "))) {
        return NULL;
    }
    return _cache_bytecodeframe(self);
}

static char* _get_data_from_bytecode2(uint8_t* bytecode,
                                      enum InstructIndex ins1,
                                      enum InstructIndex ins2) {
    ByteCodeFrame bf = {0};
    char* res = NULL;
    byteCodeFrame_init(&bf);
    byteCodeFrame_loadByteCode(&bf, bytecode);
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(&bf.instruct_array);
        if (NULL == ins_unit) {
            goto __exit;
        }
        enum InstructIndex ins = instructUnit_getInstructIndex(ins_unit);
        if (ins == ins1 || ins == ins2) {
            res = constPool_getByOffset(&bf.const_pool,
                                        ins_unit->const_pool_index);
            goto __exit;
        }
        instructArray_getNext(&bf.instruct_array);
    }
__exit:
    byteCodeFrame_deinit(&bf);
    return res;
}

static ByteCodeFrame* _cache_bcf_fn_bc(PikaObj* self, uint8_t* bytecode) {
    /* save 'def' and 'class' to heap */
    if (NULL ==
        _get_data_from_bytecode2(bytecode, PIKA_INS(DEF), PIKA_INS(CLS))) {
        return NULL;
    }
    return _cache_bytecodeframe(self);
}

VMParameters* pikaVM_run_ex(PikaObj* self,
                            char* py_lines,
                            pikaVM_run_ex_cfg* cfg) {
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame* bytecode_frame_p = NULL;
    uint8_t is_use_heap_bytecode = 0;
    PikaObj* globals = self;
    if (NULL != cfg->globals) {
        globals = cfg->globals;
    }
    /*
     * the first obj_run, cache bytecode to heap, to support 'def' and
     * 'class'
     */
    bytecode_frame_p = _cache_bcf_fn(self, py_lines);
    if (NULL == bytecode_frame_p) {
        is_use_heap_bytecode = 0;
        /* get bytecode_ptr from stack */
        bytecode_frame_p = &bytecode_frame_stack;
    }

    /* load or generate byte code frame */
    /* generate byte code */
    byteCodeFrame_init(bytecode_frame_p);
    if (PIKA_RES_OK != pika_lines2Bytes(bytecode_frame_p, py_lines)) {
        obj_setSysOut(self, PIKA_ERR_STRING_SYNTAX_ERROR);
        globals = NULL;
        goto __exit;
    }
    /* run byteCode */
    if (NULL != cfg->module_name) {
        byteCodeFrame_setName(bytecode_frame_p, cfg->module_name);
    }
    globals = _pikaVM_runByteCodeFrameGlobals(self, globals, bytecode_frame_p,
                                              cfg->in_repl);
    goto __exit;
__exit:
    if (!is_use_heap_bytecode) {
        byteCodeFrame_deinit(&bytecode_frame_stack);
    }
    return globals;
}

VMParameters* pikaVM_runByteCode_ex(PikaObj* self,
                                    uint8_t* bytecode,
                                    pikaVM_runBytecode_ex_cfg* cfg) {
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame* bytecode_frame_p = NULL;
    uint8_t is_use_heap_bytecode = 1;
    /*
     * the first obj_run, cache bytecode to heap, to support 'def' and
     * 'class'
     */
    bytecode_frame_p = _cache_bcf_fn_bc(self, bytecode);
    if (NULL == bytecode_frame_p) {
        is_use_heap_bytecode = 0;
        /* get bytecode_ptr from stack */
        bytecode_frame_p = &bytecode_frame_stack;
        /* no def/class ins, no need cache bytecode */
        cfg->is_const_bytecode = pika_true;
    }

    /* load or generate byte code frame */
    /* load bytecode */
    _do_byteCodeFrame_loadByteCode(bytecode_frame_p, bytecode, cfg->name,
                                   cfg->is_const_bytecode);

    /* run byteCode */

    cfg->globals = _pikaVM_runByteCodeFrameWithState(
        self, cfg->locals, cfg->globals, bytecode_frame_p, 0, cfg->vm_thread);
    goto __exit;
__exit:
    if (!is_use_heap_bytecode) {
        byteCodeFrame_deinit(&bytecode_frame_stack);
    }
    return cfg->globals;
}

VMParameters* pikaVM_runByteCodeFile(PikaObj* self, char* filename) {
    Args buffs = {0};
    Arg* file_arg = arg_loadFile(NULL, filename);
    pika_assert(NULL != file_arg);
    if (NULL == file_arg) {
        pika_platform_printf("Error: can not open file '%s'\n", filename);
        return NULL;
    }
    uint8_t* lines = arg_getBytes(file_arg);
    /* clear the void line */
    VMParameters* res = pikaVM_runByteCodeInconstant(self, lines);
    arg_deinit(file_arg);
    strsDeinit(&buffs);
    return res;
}

VMParameters* pikaVM_runSingleFile(PikaObj* self, char* filename) {
    Args buffs = {0};
    Arg* file_arg = arg_loadFile(NULL, filename);
    if (NULL == file_arg) {
        pika_platform_printf("FileNotFoundError: %s\n", filename);
        return NULL;
    }
    char* lines = (char*)arg_getBytes(file_arg);
    lines = strsFilePreProcess(&buffs, lines);
    /* clear the void line */
    pikaVM_run_ex_cfg cfg = {0};
    cfg.in_repl = pika_false;
    char* module_name = strsPathGetFileName(&buffs, filename);
    module_name = strsPopToken(&buffs, &module_name, '.');
    cfg.module_name = module_name;
    VMParameters* res = pikaVM_run_ex(self, lines, &cfg);
    arg_deinit(file_arg);
    strsDeinit(&buffs);
    return res;
}

VMParameters* pikaVM_run(PikaObj* self, char* py_lines) {
    pikaVM_run_ex_cfg cfg = {0};
    cfg.in_repl = pika_false;
    return pikaVM_run_ex(self, py_lines, &cfg);
}

VMParameters* pikaVM_runByteCode(PikaObj* self, const uint8_t* bytecode) {
    pika_assert(NULL != self);
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.locals = self;
    cfg.globals = self;
    cfg.name = NULL;
    cfg.vm_thread = &vm_thread;
    cfg.is_const_bytecode = pika_true;
    return pikaVM_runByteCode_ex(self, (uint8_t*)bytecode, &cfg);
}

Arg* pikaVM_runByteCodeReturn(PikaObj* self,
                              const uint8_t* bytecode,
                              char* returnName) {
    pikaVM_runByteCode(self, bytecode);
    Arg* ret = args_getArg(self->list, returnName);
    if (NULL == ret) {
        return NULL;
    }
    ret = arg_copy(ret);
    /* set gc root to avoid be free */
    arg_setObjFlag(ret, OBJ_FLAG_GC_ROOT);
    obj_removeArg(self, returnName);
    return ret;
}

Arg* pikaVM_runByteCode_exReturn(PikaObj* self,
                                 VMParameters* locals,
                                 VMParameters* globals,
                                 uint8_t* bytecode,
                                 PikaVMThread* vm_thread,
                                 pika_bool is_const_bytecode,
                                 char* return_name) {
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.locals = locals;
    cfg.globals = globals;
    cfg.vm_thread = vm_thread;
    cfg.is_const_bytecode = is_const_bytecode;
    pikaVM_runByteCode_ex(self, bytecode, &cfg);
    Arg* ret = args_getArg(self->list, return_name);
    if (NULL == ret) {
        return NULL;
    }
    ret = arg_copy(ret);
    /* set gc root to avoid be free */
    arg_setObjFlag(ret, OBJ_FLAG_GC_ROOT);
    obj_removeArg(self, return_name);
    return ret;
}

VMParameters* pikaVM_runByteCodeInconstant(PikaObj* self, uint8_t* bytecode) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    pikaVM_runBytecode_ex_cfg cfg = {0};
    cfg.locals = self;
    cfg.globals = self;
    cfg.vm_thread = &vm_thread;
    cfg.is_const_bytecode = pika_false;
    return pikaVM_runByteCode_ex(self, (uint8_t*)bytecode, &cfg);
}

void constPool_update(ConstPool* self) {
    self->content_start = (void*)arg_getContent(self->arg_buff);
}

void constPool_init(ConstPool* self) {
    self->arg_buff = NULL;
    self->content_start = NULL;
    self->content_offset_now = 0;
    self->size = 1;
    self->output_redirect_fun = NULL;
    self->output_f = NULL;
}

void constPool_deinit(ConstPool* self) {
    if (NULL != self->arg_buff) {
        arg_deinit(self->arg_buff);
    }
}

void constPool_append(ConstPool* self, char* content) {
    if (NULL == self->arg_buff) {
        self->arg_buff = arg_newStr("");
    }
    uint16_t size = strGetSize(content) + 1;
    if (NULL == self->output_redirect_fun) {
        self->arg_buff = arg_append(self->arg_buff, content, size);
    } else {
        self->output_redirect_fun(self, content);
    };
    constPool_update(self);
    self->size += size;
}

char* constPool_getNow(ConstPool* self) {
    if (self->content_offset_now >= self->size) {
        /* is the end */
        return NULL;
    }
    return (char*)((uintptr_t)constPool_getStart(self) +
                   (uintptr_t)(self->content_offset_now));
}

uint16_t constPool_getOffsetByData(ConstPool* self, char* data) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    uint16_t offset_out = 65535;
    if (self->arg_buff == NULL) {
        goto __exit;
    }
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto __exit;
        }
        if (strEqu(data, constPool_getNow(self))) {
            offset_out = self->content_offset_now;
            goto __exit;
        }
    }
__exit:
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return offset_out;
}

char* constPool_getNext(ConstPool* self) {
    self->content_offset_now += strGetSize(constPool_getNow(self)) + 1;
    return constPool_getNow(self);
}

char* constPool_getByIndex(ConstPool* self, uint16_t index) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    for (uint16_t i = 0; i < index; i++) {
        constPool_getNext(self);
    }
    char* res = constPool_getNow(self);
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return res;
}

void constPool_print(ConstPool* self) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto __exit;
        }
        uint16_t offset = self->content_offset_now;
        pika_platform_printf("%d: %s\r\n", offset, constPool_getNow(self));
    }
__exit:
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return;
}

void byteCodeFrame_init(ByteCodeFrame* self) {
    /* init to support append,
       if only load static bytecode,
       can not init */
    constPool_init(&(self->const_pool));
    instructArray_init(&(self->instruct_array));
    self->name = NULL;
    self->label_pc = -1;
}

extern const char magic_code_pyo[4];
void _do_byteCodeFrame_loadByteCode(ByteCodeFrame* self,
                                    uint8_t* bytes,
                                    char* name,
                                    pika_bool is_const) {
    if (bytes[0] == magic_code_pyo[0] && bytes[1] == magic_code_pyo[1] &&
        bytes[2] == magic_code_pyo[2] && bytes[3] == magic_code_pyo[3]) {
        /* load from file, found magic code, skip head */
        bytes = bytes + sizeof(magic_code_pyo) + sizeof(uint32_t);
    }
    uint32_t* ins_size_p = (uint32_t*)bytes;
    void* ins_start_p = (uint32_t*)((uintptr_t)bytes + sizeof(*ins_size_p));
    uint32_t* const_size_p =
        (uint32_t*)((uintptr_t)ins_start_p + (uintptr_t)(*ins_size_p));
    self->instruct_array.size = *ins_size_p;
    self->instruct_array.content_start = ins_start_p;
    self->const_pool.size = *const_size_p;
    self->const_pool.content_start =
        (char*)((uintptr_t)const_size_p + sizeof(*const_size_p));
    byteCodeFrame_setName(self, name);
    if (!is_const) {
        pika_assert(NULL == self->instruct_array.arg_buff);
        pika_assert(NULL == self->instruct_array.arg_buff);
        self->instruct_array.arg_buff = arg_newBytes(ins_start_p, *ins_size_p);
        self->const_pool.arg_buff =
            arg_newBytes(self->const_pool.content_start, *const_size_p);
        self->instruct_array.content_start =
            arg_getBytes(self->instruct_array.arg_buff);
        self->const_pool.content_start =
            arg_getBytes(self->const_pool.arg_buff);
    }
    pika_assert(NULL != self->const_pool.content_start);
}

void byteCodeFrame_setName(ByteCodeFrame* self, char* name) {
    if (name != NULL && self->name == NULL) {
        self->name = pika_platform_malloc(strGetSize(name) + 1);
        pika_platform_memcpy(self->name, name, strGetSize(name) + 1);
    }
}

void byteCodeFrame_loadByteCode(ByteCodeFrame* self, uint8_t* bytes) {
    _do_byteCodeFrame_loadByteCode(self, bytes, NULL, pika_true);
}

void byteCodeFrame_deinit(ByteCodeFrame* self) {
    constPool_deinit(&(self->const_pool));
    instructArray_deinit(&(self->instruct_array));
    if (NULL != self->name) {
        pika_platform_free(self->name);
    }
}

void instructArray_init(InstructArray* self) {
    self->arg_buff = NULL;
    self->content_start = NULL;
    self->size = 0;
    self->content_offset_now = 0;
    self->output_redirect_fun = NULL;
    self->output_f = NULL;
}

void instructArray_deinit(InstructArray* self) {
    if (NULL != self->arg_buff) {
        arg_deinit(self->arg_buff);
    }
}

void instructArray_append(InstructArray* self, InstructUnit* ins_unit) {
    if (NULL == self->arg_buff) {
        self->arg_buff = arg_newNone();
    }
    if (NULL == self->output_redirect_fun) {
        self->arg_buff =
            arg_append(self->arg_buff, ins_unit, instructUnit_getSize());
    } else {
        self->output_redirect_fun(self, ins_unit);
    };
    instructArray_update(self);
    self->size += instructUnit_getSize();
}

void instructUnit_init(InstructUnit* ins_unit) {
    ins_unit->depth = 0;
    ins_unit->const_pool_index = 0;
    ins_unit->isNewLine_instruct = 0;
}

void instructArray_update(InstructArray* self) {
    self->content_start = (void*)arg_getContent(self->arg_buff);
}

InstructUnit* instructArray_getNow(InstructArray* self) {
    if (self->content_offset_now >= self->size) {
        /* is the end */
        return NULL;
    }
    return (InstructUnit*)((uintptr_t)instructArray_getStart(self) +
                           (uintptr_t)(self->content_offset_now));
}

InstructUnit* instructArray_getNext(InstructArray* self) {
    self->content_offset_now += instructUnit_getSize();
    return instructArray_getNow(self);
}

#if PIKA_INSTRUCT_EXTENSION_ENABLE

static const char* __find_ins_str_in_ins_set(enum InstructIndex op_idx,
                                             const VMInstructionSet* set) {
    const VMInstruction* ins = set->instructions;
    uint_fast16_t count = set->count;

    do {
        if (ins->op_idx == op_idx) {
            return ins->op_str;
        }
        ins++;
    } while (--count);
    return NULL;
}

static char* instructUnit_getInstructStr(InstructUnit* self) {
    enum InstructIndex op_idx = instructUnit_getInstructIndex(self);

    const char* ins_str =
        __find_ins_str_in_ins_set(op_idx, g_PikaVMInsSet.recent->ins_set);
    if (NULL != ins_str) {
        return (char*)ins_str;
    }
    VMInstructionSetItem* item = g_PikaVMInsSet.list;
    do {
        ins_str = __find_ins_str_in_ins_set(op_idx, item->ins_set);
        if (NULL != ins_str) {
            g_PikaVMInsSet.recent = item;
            return (char*)ins_str;
        }
        item = item->next;
    } while (NULL != item->next);
    return "NON";
}
#else
static char* instructUnit_getInstructStr(InstructUnit* self) {
#define __INS_GET_INS_STR
#include "__instruction_table.h"
    return "NON";
}
#endif

void instructUnit_print(InstructUnit* self) {
    if (instructUnit_getIsNewLine(self)) {
        pika_platform_printf("B%d\r\n", instructUnit_getBlockDeepth(self));
    }
    pika_platform_printf("%d %s #%d\r\n", instructUnit_getInvokeDeepth(self),
                         instructUnit_getInstructStr(self),
                         self->const_pool_index);
}

static void instructUnit_printWithConst(InstructUnit* self,
                                        ConstPool* const_pool) {
    // if (instructUnit_getIsNewLine(self)) {
    //     pika_platform_printf("B%d\r\n",
    //     instructUnit_getBlockDeepth(self));
    // }
    pika_platform_printf(
        "%s %s \t\t(#%d)\r\n", instructUnit_getInstructStr(self),
        constPool_getByOffset(const_pool, self->const_pool_index),
        self->const_pool_index);
}

void instructArray_printWithConst(InstructArray* self, ConstPool* const_pool) {
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(self);
        if (NULL == ins_unit) {
            goto __exit;
        }
        instructUnit_printWithConst(ins_unit, const_pool);
        instructArray_getNext(self);
    }
__exit:
    self->content_offset_now = offset_befor;
    return;
}

void instructArray_print(InstructArray* self) {
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(self);
        if (NULL == ins_unit) {
            goto __exit;
        }
        instructUnit_print(ins_unit);
        instructArray_getNext(self);
    }
__exit:
    self->content_offset_now = offset_befor;
    return;
}

void instructArray_printAsArray(InstructArray* self) {
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    uint8_t line_num = 12;
    uint16_t g_i = 0;
    uint8_t* ins_size_p = (uint8_t*)&self->size;
    pika_platform_printf("0x%02x, ", *(ins_size_p));
    pika_platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)1));
    pika_platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)2));
    pika_platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)3));
    pika_platform_printf("/* instruct array size */\n");
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(self);
        if (NULL == ins_unit) {
            goto __exit;
        }
        for (int i = 0; i < (int)instructUnit_getSize(); i++) {
            g_i++;
            pika_platform_printf("0x%02x, ",
                                 *((uint8_t*)ins_unit + (uintptr_t)i));
            if (g_i % line_num == 0) {
                pika_platform_printf("\n");
            }
        }
        instructArray_getNext(self);
    }
__exit:
    pika_platform_printf("/* instruct array */\n");
    self->content_offset_now = offset_befor;
    return;
}

size_t byteCodeFrame_getSize(ByteCodeFrame* bf) {
    return bf->const_pool.size + bf->instruct_array.size;
}

void byteCodeFrame_print(ByteCodeFrame* self) {
    constPool_print(&(self->const_pool));
    instructArray_printWithConst(&(self->instruct_array), &(self->const_pool));
    pika_platform_printf("---------------\r\n");
    pika_platform_printf("byte code size: %d\r\n",
                         self->const_pool.size + self->instruct_array.size);
}

static VMParameters* __pikaVM_runByteCodeFrameWithState(
    PikaObj* self,
    VMParameters* locals,
    VMParameters* globals,
    ByteCodeFrame* bytecode_frame,
    uint16_t pc,
    PikaVMThread* vm_thread,
    pika_bool in_repl) {
    pika_assert(NULL != vm_thread);
    int size = bytecode_frame->instruct_array.size;
    /* locals is the local scope */

    if (g_PikaVMState.vm_cnt == 0) {
        pika_vmSignal_setCtrlClear();
    }
    PikaVMFrame* vm =
        vm_frame_create(locals, globals, bytecode_frame, pc, vm_thread);
    vm->in_repl = in_repl;
    g_PikaVMState.vm_cnt++;
    while (vm->pc < size) {
        if (vm->pc == VM_PC_EXIT) {
            break;
        }
        InstructUnit* this_ins_unit = PikaVMFrame_getInstructNow(vm);
        uint8_t is_new_line = instructUnit_getIsNewLine(this_ins_unit);
        if (is_new_line) {
            vm_frame_solve_unused_stack(vm);
            stack_reset(&(vm->stack));
            vm->vm_thread->error_code = 0;
            vm->vm_thread->line_error_code = 0;
        }
        self->vmFrame = vm;
        vm->pc = pikaVM_runInstructUnit(self, vm, this_ins_unit);
        vm->ins_cnt++;
#if PIKA_INSTRUCT_HOOK_ENABLE
        if (vm->ins_cnt % PIKA_INSTRUCT_HOOK_PERIOD == 0) {
            pika_hook_instruct();
        }
#endif
        if (vm->ins_cnt % PIKA_INSTRUCT_YIELD_PERIOD == 0) {
            _pikaVM_yield();
        }
        if (0 != vm->vm_thread->error_code) {
            vm->vm_thread->line_error_code = vm->vm_thread->error_code;
            InstructUnit* head_ins_unit = this_ins_unit;
            /* get first ins of a line */
            while (1) {
                if (instructUnit_getIsNewLine(head_ins_unit)) {
                    break;
                }
                head_ins_unit--;
            }
            if (vm->vm_thread->try_state) {
                vm->vm_thread->try_error_code = vm->vm_thread->error_code;
            }
            /* print inses of a line */
            if (!vm->vm_thread->try_state) {
                while (1) {
                    if (head_ins_unit != this_ins_unit) {
                        pika_platform_printf("   ");
                    } else {
                        pika_platform_printf(" -> ");
                    }
                    instructUnit_printWithConst(head_ins_unit,
                                                &(bytecode_frame->const_pool));
                    head_ins_unit++;
                    if (head_ins_unit > this_ins_unit) {
                        break;
                    }
                }
            }
            pika_platform_error_handle();
        }
    }
    vm_frame_solve_unused_stack(vm);
    stack_deinit(&(vm->stack));
    g_PikaVMState.vm_cnt--;
    VMParameters* result = locals;
    pikaFree(vm, sizeof(PikaVMFrame));
    self->vmFrame = NULL;
    return result;
}

pika_bool pika_debug_check_break(char* module_name, int pc_break) {
    Hash h = hash_time33(module_name);
    for (int i = 0; i < g_PikaVMState.break_point_cnt; i++) {
        if (g_PikaVMState.break_module_hash[i] == h &&
            g_PikaVMState.break_point_pc[i] == pc_break) {
            return pika_true;
        }
    }
    return pika_false;
}

pika_bool pika_debug_check_break_hash(Hash module_hash, int pc_break) {
    for (int i = 0; i < g_PikaVMState.break_point_cnt; i++) {
        if (g_PikaVMState.break_module_hash[i] == module_hash &&
            g_PikaVMState.break_point_pc[i] == pc_break) {
            return pika_true;
        }
    }
    return pika_false;
}

PIKA_RES pika_debug_set_break(char* module_name, int pc_break) {
    if (pika_debug_check_break(module_name, pc_break)) {
        return PIKA_RES_OK;
    }
    if (g_PikaVMState.break_point_cnt >= PIKA_DEBUG_BREAK_POINT_MAX) {
        return PIKA_RES_ERR_RUNTIME_ERROR;
    }
    Hash h = hash_time33(module_name);
    g_PikaVMState.break_module_hash[g_PikaVMState.break_point_cnt] = h;
    g_PikaVMState.break_point_pc[g_PikaVMState.break_point_cnt] = pc_break;
    g_PikaVMState.break_point_cnt++;
    return PIKA_RES_OK;
}

PIKA_RES pika_debug_reset_break(char* module_name, int pc_break) {
    if (!pika_debug_check_break(module_name, pc_break)) {
        return PIKA_RES_OK;
    }
    Hash h = hash_time33(module_name);
    for (int i = 0; i < g_PikaVMState.break_point_cnt; i++) {
        if (g_PikaVMState.break_module_hash[i] == h &&
            g_PikaVMState.break_point_pc[i] == pc_break) {
            // Move subsequent break points one position forward
            for (int j = i; j < g_PikaVMState.break_point_cnt - 1; j++) {
                g_PikaVMState.break_module_hash[j] =
                    g_PikaVMState.break_module_hash[j + 1];
                g_PikaVMState.break_point_pc[j] =
                    g_PikaVMState.break_point_pc[j + 1];
            }
            // Decrease the count of break points
            g_PikaVMState.break_point_cnt--;
            return PIKA_RES_OK;
        }
    }
    return PIKA_RES_ERR_RUNTIME_ERROR;
}

VMParameters* _pikaVM_runByteCodeFrameWithState(PikaObj* self, VMParameters* locals,
    VMParameters* globals, ByteCodeFrame* bytecode_frame, uint16_t pc, PikaVMThread* vm_thread)
{
    return __pikaVM_runByteCodeFrameWithState(
        self, locals, globals, bytecode_frame, pc, vm_thread, pika_false);
}

VMParameters* _pikaVM_runByteCodeFrame(PikaObj* self,
                                       ByteCodeFrame* byteCode_frame,
                                       pika_bool in_repl) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    return __pikaVM_runByteCodeFrameWithState(self, self, self, byteCode_frame,
                                              0, &vm_thread, in_repl);
}

VMParameters* _pikaVM_runByteCodeFrameGlobals(PikaObj* self,
                                              PikaObj* globals,
                                              ByteCodeFrame* byteCode_frame,
                                              pika_bool in_repl) {
    PikaVMThread vm_thread = {.try_state = TRY_STATE_NONE,
                              .try_result = TRY_RESULT_NONE};
    return __pikaVM_runByteCodeFrameWithState(
        self, self, globals, byteCode_frame, 0, &vm_thread, in_repl);
}

VMParameters* pikaVM_runByteCodeFrame(PikaObj* self,
                                      ByteCodeFrame* byteCode_frame) {
    return _pikaVM_runByteCodeFrame(self, byteCode_frame, pika_false);
}

void constPool_printAsArray(ConstPool* self) {
    uint8_t* const_size_str = (uint8_t*)&(self->size);
    pika_platform_printf("0x%02x, ", *(const_size_str));
    pika_platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)1));
    pika_platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)2));
    pika_platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)3));
    pika_platform_printf("/* const pool size */\n");
    uint16_t ptr_befor = self->content_offset_now;
    uint8_t line_num = 12;
    uint16_t g_i = 0;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    pika_platform_printf("0x00, ");
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto __exit;
        }
        char* data_each = constPool_getNow(self);
        /* todo start */
        size_t len = strlen(data_each);
        for (uint32_t i = 0; i < len + 1; i++) {
            pika_platform_printf("0x%02x, ", *(data_each + (uintptr_t)i));
            g_i++;
            if (g_i % line_num == 0) {
                pika_platform_printf("\n");
            }
        }
        /* todo end */
    }
__exit:
    /* retore ptr_now */
    pika_platform_printf("/* const pool */\n");
    self->content_offset_now = ptr_befor;
    return;
}

void byteCodeFrame_printAsArray(ByteCodeFrame* self) {
    pika_platform_printf("const uint8_t bytes[] = {\n");
    instructArray_printAsArray(&(self->instruct_array));
    constPool_printAsArray(&(self->const_pool));
    pika_platform_printf("};\n");
    pika_platform_printf("pikaVM_runByteCode(self, (uint8_t*)bytes);\n");
}

PikaObj* pikaVM_runFile(PikaObj* self, char* file_name) {
    Args buffs = {0};
    char* module_name = strsPathGetFileName(&buffs, file_name);
    strPopLastToken(module_name, '.');
    char* pwd = strsPathGetFolder(&buffs, file_name);
    pika_platform_printf("(pikascript) pika compiler:\r\n");
    PikaMaker* maker = New_PikaMaker();
    pikaMaker_setPWD(maker, pwd);
    pikaMaker_compileModuleWithDepends(maker, module_name);
    _do_pikaMaker_linkCompiledModules(maker, "pikaModules_cache.py.a",
                                      pika_false);
    pikaMaker_deinit(maker);
    pika_platform_printf("(pikascript) all succeed.\r\n\r\n");

    pikaMemMaxReset();
    char* libfile_path =
        strsPathJoin(&buffs, pwd, "pikascript-api/pikaModules_cache.py.a");
    if (PIKA_RES_OK == obj_linkLibraryFile(self, libfile_path)) {
        obj_runModule(self, module_name);
    }
    strsDeinit(&buffs);
    return self;
}

void _pikaVM_yield(void) {
#if PIKA_EVENT_ENABLE
    if (!pika_GIL_isInit()) {
        _VMEvent_pickupEvent();
    }
#endif
    /*
     * [Warning!] Can not use pika_GIL_ENTER() and pika_GIL_EXIT() here,
     * because yield() is called not atomically, and it will cause data
     * race.
     */
    // pika_GIL_EXIT();
    // pika_GIL_ENTER();
}
