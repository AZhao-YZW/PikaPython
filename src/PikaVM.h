/*
 * This file is part of the PikaScript project.
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
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PIKA_VM__H
#define __PIKA_VM__H
#include "PikaObj.h"
#include "dataQueue.h"
#include "dataQueueObj.h"
#include "dataStack.h"
#if PIKA_SETJMP_ENABLE
#include <setjmp.h>
#endif

#define pika_GIL_EXIT       vm_gil_exit
#define pika_GIL_ENTER      vm_gil_enter
#define _VM_lock_init       vm_gil_init
#define _VM_is_first_lock   vm_gil_is_first_lock
#define byteCodeFrame_init  bc_frame_init
#define byteCodeFrame_appendFromAsm bc_frame_append_from_asm
#define byteCodeFrame_deinit bc_frame_deinit

void pika_debug_set_trace(PikaObj *self);

typedef enum VM_SIGNAL_CTRL {
    VM_SIGNAL_CTRL_NONE = 0,
    VM_SIGNAL_CTRL_EXIT,
} VM_SIGNAL_CTRL;

typedef union _EventDataType {
    int signal;
    Arg* arg;
} EventDataType;

typedef struct PikaArgEventQueue {
    uint32_t id[PIKA_EVENT_LIST_SIZE];
    EventDataType data[PIKA_EVENT_LIST_SIZE];
    PikaEventListener* listener[PIKA_EVENT_LIST_SIZE];
    Arg* res[PIKA_EVENT_LIST_SIZE];
    int head;
    int tail;
} PikaEventQueue;

typedef struct VMState VMState;
struct VMState {
    VM_SIGNAL_CTRL signal_ctrl;
    int vm_cnt;
#if PIKA_EVENT_ENABLE
    PikaEventQueue cq;
    PikaEventQueue sq;
    int event_pickup_cnt;
    pika_platform_thread_t* event_thread;
    pika_bool event_thread_exit;
    pika_bool event_thread_exit_done;
#endif
#if PIKA_DEBUG_BREAK_POINT_MAX > 0
    Hash break_module_hash[PIKA_DEBUG_BREAK_POINT_MAX];
    uint32_t break_point_pc[PIKA_DEBUG_BREAK_POINT_MAX];
    int break_point_cnt;
#endif
};

typedef struct {
    PikaObj* globals;
    pika_bool in_repl;
    char* module_name;
} pikaVM_run_ex_cfg;

typedef struct {
    VMParameters* locals;
    VMParameters* globals;
    char* name;
    PikaVMThread* vm_thread;
    pika_bool is_const_bytecode;
} pikaVM_runBytecode_ex_cfg;

VMParameters* pikaVM_run(PikaObj* self, char* pyLine);
VMParameters* pikaVM_runAsm(PikaObj* self, char* pikaAsm);
VMParameters* _pikaVM_runByteCodeFrame(PikaObj* self, ByteCodeFrame *bc_frame, pika_bool in_repl);
VMParameters* _pikaVM_runByteCodeFrameGlobals(PikaObj* self, PikaObj* globals,
                                              ByteCodeFrame *bc_frame, pika_bool in_repl);
VMParameters* pikaVM_runByteCodeFrame(PikaObj* self, ByteCodeFrame *bc_frame);

ByteCodeFrame *bc_frame_append_from_asm(ByteCodeFrame *bf, char* pikaAsm);
void bc_frame_init(ByteCodeFrame *bf);
void bc_frame_deinit(ByteCodeFrame *bf);
void bc_frame_print(ByteCodeFrame *self);
void bc_frame_load_bytecode_default(ByteCodeFrame *self, uint8_t* bytes);
void bc_frame_print_as_array(ByteCodeFrame *self);
VMParameters* pikaVM_runByteCode(PikaObj* self, const uint8_t* bytecode);
VMParameters* pikaVM_runByteCodeInconstant(PikaObj* self, uint8_t* bytecode);
Arg* pikaVM_runByteCodeReturn(PikaObj* self, const uint8_t* bytecode, char* returnName);
Arg* pikaVM_runByteCode_exReturn(PikaObj* self, VMParameters* locals, VMParameters* globals,
                                 uint8_t* bytecode, PikaVMThread* vm_thread,
                                 pika_bool is_const_bytecode, char* return_name);
InstructUnit *inst_array_get_now(InstructArray *self);
InstructUnit *inst_array_get_next(InstructArray *self);
VMParameters* pikaVM_runSingleFile(PikaObj* self, char* filename);
VMParameters* pikaVM_runByteCodeFile(PikaObj* self, char* filename);
Arg* obj_runMethodArg(PikaObj* self, PikaObj* method_args_obj, Arg* method_arg);
PikaObj* pikaVM_runFile(PikaObj* self, char* file_name);

VMParameters* pikaVM_runByteCode_ex(PikaObj* self, uint8_t* bytecode, pikaVM_runBytecode_ex_cfg* cfg);

Arg* _vm_get(PikaVMFrame *vm, PikaObj* self, Arg* key, Arg* obj);
void pika_vm_exit(void);
void pika_vm_exit_await(void);
PIKA_RES event_listener_push_event(PikaEventListener* lisener, uintptr_t eventId, Arg* eventData);
PIKA_RES event_listener_push_signal(PikaEventListener* lisener, uintptr_t eventId, int signal);
int vm_event_get_vm_cnt(void);
void vm_event_pickup_event(char* info);
int vm_gil_init(void);
int vm_gil_is_first_lock(void);
PIKA_RES pika_debug_set_break(char* module_name, int pc_break);
PIKA_RES pika_debug_reset_break(char* module_name, int pc_break);
pika_bool pika_debug_check_break_hash(Hash module_hash, int pc_break);

#define _VMEvent_pickupEvent() vm_event_pickup_event(__FILE__)
#define PIKA_INS(__INS_NAME) _##PIKA_VM_INS_##__INS_NAME

VMParameters* pikaVM_run_ex(PikaObj* self, char* py_lines, pikaVM_run_ex_cfg* cfg);

#endif

#ifdef __cplusplus
}
#endif
