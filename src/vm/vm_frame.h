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
#ifndef __VM_FRAME_H__
#define __VM_FRAME_H__

#include "dataArgs.h"
#include "PikaVM.h"
#include "PikaObj.h"
#include "const_pool.h"
#include "inst.h"

typedef struct {
    int8_t n_positional;
    int8_t n_positional_got;
    int8_t n_default;
    int8_t n_arg;
    int8_t i_arg;
    int8_t n_input;
    pika_bool is_vars;
    pika_bool is_keys;
    pika_bool is_default;
    ArgType method_type;
    PikaTuple* tuple;
    PikaDict* kw;
    PikaDict* kw_keys;
    char *var_tuple_name;
    char *kw_dict_name;
    char *type_list;
} FunctionArgsInfo;

static inline char *vm_frame_get_const_with_inst_unit(PikaVMFrame *vm, InstructUnit *ins_unit)
{
    return const_pool_get_by_offset(&(vm->bytecode_frame->const_pool),
                                    inst_unit_get_const_pool_index(ins_unit));
}

static inline uint32_t vm_frame_get_inst_array_size(PikaVMFrame *vm)
{
    return inst_array_get_size(&(vm->bytecode_frame->instruct_array));
}

static inline InstructUnit *vm_frame_get_inst_unit_with_offset(PikaVMFrame *vm, int offset)
{
    return inst_array_get_by_offset(&(vm->bytecode_frame->instruct_array), vm->pc + offset);
}

static inline InstructUnit *vm_frame_get_inst_now(PikaVMFrame *vm)
{
    return inst_array_get_by_offset(&(vm->bytecode_frame->instruct_array), vm->pc);
}

void vm_frame_set_error_code(PikaVMFrame *vm, int8_t error_code);
void vm_frame_set_sys_out(PikaVMFrame *vm, char *fmt, ...);
enum InstructIndex vm_frame_get_inst_with_offset(PikaVMFrame *vm, int32_t offset);
int vm_frame_get_blk_deepth_now(PikaVMFrame *vm);
int vm_frame_get_invoke_deepth_now(PikaVMFrame *vm);
pika_bool vm_frame_check_break_point(PikaVMFrame *vm);
int32_t vm_frame_get_jmp_addr_offset(PikaVMFrame *vm);
int32_t vm_frame_get_break_addr_offset(PikaVMFrame *vm);
int32_t vm_frame_get_continue_addr_offset(PikaVMFrame *vm);
int vm_frame_load_args_from_method_arg(PikaVMFrame *vm, PikaObj *oMethodHost, Args* aLoclas,
                                       Arg* aMethod, char *sMethodName, char *sProxyName, int iNumUsed);
uint32_t vm_frame_get_input_arg_num(PikaVMFrame *vm);
void vm_frame_solve_unused_stack(PikaVMFrame *vm);
PikaVMFrame *vm_frame_create(VMParameters* locals, VMParameters* globals, ByteCodeFrame *bytecode_frame,
                             int32_t pc, PikaVMThread* vm_thread);

#if !PIKA_NANO_ENABLE
char *vm_frame_get_const_with_offset(PikaVMFrame *vm, int32_t offset);
int32_t vm_frame_get_raise_addr_offset(PikaVMFrame *vm);
#endif

#endif