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
#ifndef __INST_H__
#define __INST_H__

#include "dataArgs.h"
#include "PikaVM.h"
#include "const_pool.h"

typedef Arg* (*VM_instruct_handler)(PikaObj *self, PikaVMFrame *vm, char *data, Arg* arg_ret_reg);
typedef struct VMInstruction {
    VM_instruct_handler handler;
    const char *op_str;
    uint16_t op_idx;
    uint16_t op_str_len : 4;
    uint16_t : 12;
} VMInstruction;

typedef struct VMInstructionSet {
    const struct VMInstruction* instructions;
    uint16_t count;
    uint16_t signature;
    uint16_t op_idx_start;
    uint16_t op_idx_end;
} VMInstructionSet;

static inline int inst_unit_get_blk_deepth(InstructUnit *self)
{
    return self->depth & 0x0F;
}

static inline void inst_unit_set_blk_deepth(InstructUnit *self, int val)
{
    self->depth |= (0x0F & val);
}

static inline int inst_unit_get_invoke_deepth(InstructUnit *self)
{
    return self->depth >> 4;
}

static inline void inst_unit_set_invoke_deepth(InstructUnit *self, int val)
{
    self->depth |= ((0x0F & val) << 4);
}

static inline enum InstructIndex inst_unit_get_inst_index(InstructUnit *self)
{
    return (enum InstructIndex)(self->isNewLine_instruct & 0x7F);
}

static inline void inst_unit_set_inst_index(InstructUnit *self, int val)
{
    self->isNewLine_instruct |= (0x7F & val);
}

static inline int inst_unit_get_const_pool_index(InstructUnit *self)
{
    return self->const_pool_index;
}

static inline void inst_unit_set_const_pool_index(InstructUnit *self, int val)
{
    self->const_pool_index = val;
}

static inline int inst_unit_get_is_new_line(InstructUnit *self)
{
    return self->isNewLine_instruct >> 7;
}

static inline void inst_unit_set_is_new_line(InstructUnit *self, int val)
{
    self->isNewLine_instruct |= ((0x01 & val) << 7);
}

static inline size_t inst_unit_get_size(void)
{
    return (size_t)sizeof(InstructUnit);
}

static inline InstructUnit *inst_array_get_start(InstructArray *self)
{
    return (InstructUnit *)self->content_start;
}

static inline uint32_t inst_array_get_size(InstructArray *self)
{
    return self->size;
}

static inline InstructUnit *inst_array_get_by_offset(InstructArray *self, int offset)
{
    return (InstructUnit *)((uintptr_t)inst_array_get_start(self) + (uintptr_t)offset);
}

const VMInstruction* inst_unit_get_inst(enum InstructIndex ins_idx);
void inst_unit_print_with_const_pool(InstructUnit *self, ConstPool *const_pool);
void inst_array_print_with_const_pool(InstructArray *self, ConstPool *const_pool);
void inst_array_print_as_array(InstructArray *self);
void inst_array_init(InstructArray *ins_array);
void inst_array_deinit(InstructArray *ins_array);
void inst_array_append(InstructArray *ins_array, InstructUnit *ins_unit);
void inst_unit_init(InstructUnit *ins_unit);
void inst_unit_print(InstructUnit *self);
void inst_array_print(InstructArray *self);
enum InstructIndex inst_get_index_from_asm(char *line);

#endif